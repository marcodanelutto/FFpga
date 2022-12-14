#pragma once

#include <iostream>
#include <ff/ff.hpp>

#include <fpga/fdevice.hpp>
#include <fpga/task.hpp> 

#include <utils/utimer.cpp>

using namespace ff;


class fnode_sender : public ff_node
{
private:

    std::string bs;
    std::string kn;
    FDevice * d;
    FTaskCL task_description;

    std::vector<cl::Kernel> kernels;
    std::vector< std::vector<cl::Buffer *> > in_buffers;
    std::vector< std::vector<cl::Buffer *> > out_buffers;

    std::vector<cl::CommandQueue> in_queues; // same cardinality of in_buffers (each queue is assigned on a logical in_buffer)
    std::vector<cl::CommandQueue> kernel_queues; // same cardinality of par variable
    std::vector<cl::CommandQueue> out_queues; // same cardinality of out_buffers (each queue is assigned on a logical out_buffer)

    std::vector< std::vector<cl::Event> > out_events;

    size_t par; // number of buffer of the n-buffering technique to overlap transfer with computation
    size_t it;

    cl_int err;

    void _prepare() {
        for (size_t i = 0; i < par; i++) {
            kernels.push_back(d->createKernelInstance());
        }

        int argi = 0;

        for (auto & p : task_description.in) {
            std::vector<cl::Buffer *> buffs;
            buffs.reserve(par);

            for (size_t i = 0; i < par; ++i) {
                cl::Buffer * b = new cl::Buffer(d->getContext(), CL_MEM_READ_ONLY, p.size, nullptr, &err);
                if (err != CL_SUCCESS) {
                    std::cerr << "Error in creating IN buffer with size: " << p.size << std::endl;
                    exit(-1);
                }
                buffs.push_back(b);

                kernels[i].setArg(argi, *b);
            }
            argi++;

            in_buffers.push_back(buffs);
        }

        for (auto & p : task_description.out) {
            std::vector<cl::Buffer *> buffs;
            buffs.reserve(par);

            for (size_t i = 0; i < par; ++i) {
                cl::Buffer * b = new cl::Buffer(d->getContext(), CL_MEM_WRITE_ONLY, p.size, nullptr, &err);
                if (err != CL_SUCCESS) {
                    std::cerr << "Error in creating OUT buffer with size: " << p.size << std::endl;
                    exit(-2);
                }
                buffs.push_back(b);

                kernels[i].setArg(argi, *b);
            }
            argi++;

            out_buffers.push_back(buffs);
        }

        for (size_t i = 0; i < task_description.in.size(); ++i) {
            in_queues.push_back(d->createQueueInstance());
        }

        for (size_t i = 0; i < par; ++i) {
            kernel_queues.push_back(d->createQueueInstance());
        }

        for (size_t i = 0; i < task_description.out.size(); ++i) {
            out_queues.push_back(d->createQueueInstance());
        }

        out_events = std::vector< std::vector<cl::Event> >(par, std::vector<cl::Event>());
    }

public:

    fnode_sender(std::string bs, std::string kn, FTaskCL task_description, size_t par = 2)
    : bs(bs)
    , kn(kn)
    , task_description(task_description)
    , par(par)
    , it(0)
    {
        d = new FDevice(bs, kn);
        _prepare();
    }

    fnode_sender(FDevice * d, FTaskCL task_description, size_t par = 2)
    : d(d)
    , task_description(task_description)
    , par(par)
    , it(0)
    {
        d = new FDevice(bs, kn);
        _prepare();
    }

    void * svc (void * t)
    {
        FTaskCL * task = (FTaskCL *)t;

        const size_t i = it % par;
        if (it >= par) {
            cl::Event::waitForEvents(out_events[i]);
        }
        it++;

        std::vector<cl::Event> write_events(task->in.size());
        int argi = 0;
        for (FTaskElement & p : task->in) {
            err = in_queues[argi].enqueueWriteBuffer(*(in_buffers[argi][i]),
                                                     CL_FALSE,
                                                     0, p.size, p.ptr,
                                                     nullptr,
                                                     &write_events[argi]);
            if (err != CL_SUCCESS) {
                std::cerr << "ERROR: enqueueWriteBuffer()" << std::endl;
                return nullptr;
            }

            argi++;
        }

        argi += task->out.size();
        for (FTaskElement & p : task->scalars) {
            kernels[i].setArg(argi++, *( (int *)p.ptr) );
        }

        cl::Event kernel_event;
        err = kernel_queues[i].enqueueTask(kernels[i], &write_events, &kernel_event); // TODO: check write_events if has to be passed by reference
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: enqueueTask()" << std::endl;
            return nullptr;
        }


        std::vector<cl::Event> wait_events;
        wait_events.push_back(kernel_event);
        std::vector<cl::Event> read_events(task->out.size());
        argi = 0;
        for (auto & p : task->out) {
            err = out_queues[argi].enqueueReadBuffer(*(out_buffers[argi][i]), CL_FALSE, 0, p.size, p.ptr, &wait_events, &(read_events[argi]));
            if (err != CL_SUCCESS) {
                std::cerr << "ERROR: enqueueReadBuffer()" << std::endl;
                return nullptr;
            }
            argi++;
        }

        out_events[i] = read_events;

        task->write_event = write_events;
        task->kernel_event.push_back(kernel_event);
        task->read_event = read_events;

        return t;
    }

    void svc_end()
    {
        for (auto buffs : in_buffers) {
            for (auto p : buffs) {
                delete p;
            }
        }

        for (auto buffs : out_buffers) {
            for (auto p : buffs) {
                delete p;
            }
        }
    }
};

class fnode_receiver : public ff_node
{
private:

public:

    fnode_receiver()
    {}

    int svc_init(void)
    {
        total_time = 0;
        return 0;
    }

    void * svc (void * t)
    {
        FTaskCL * task = (FTaskCL *)t;
        if (t == nullptr) {
            std::cerr << "ERROR: fnode_receiver task = NULL" << std::endl;
        }

        cl::Event::waitForEvents(task->read_event);

        // auto write_time  = task->write_all_time_us();
        // auto kernel_time = task->kernel_all_time_us();
        // auto read_time   = task->read_all_time_us();

        // total_time += write_time + kernel_time + read_time;

        // std::cout << "TASK: ("
        //           << write_time  << "us, "
        //           << kernel_time << "us, "
        //           << read_time   << "us) "
        //           << std::endl;
        return t;
    }
};


class fnode_overlap {
private:

    FTaskCL task_description;
    size_t par;

    ff_pipeline * p;
    fnode_sender * sender;
    fnode_receiver * receiver;

    void prepare(std::string bs, std::string kn)
    {
        sender = new fnode_sender(bs, kn, task_description, par);
        receiver = new fnode_receiver();

        p = new ff_pipeline();
        p->add_stage(sender);
        p->add_stage(receiver);
    }

public:
        fnode_overlap(FTaskCL task_description, size_t par = 2)
            : task_description(task_description)
            , par(par)
        {
            prepare("krnl_vadd.xclbin", "krnl_vadd");
        }

        fnode_overlap(std::string bs, std::string kn, FTaskCL task_description, size_t par = 2)
            : task_description(task_description)
            , par(par)
        {
            prepare(bs, kn);
        }

        ff_pipeline * stage() {
            return p;
        }
};
