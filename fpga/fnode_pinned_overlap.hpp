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

    cl::Kernel kernel;
    cl::CommandQueue write_queue;
    cl::CommandQueue kernel_queue;
    cl::CommandQueue read_queue;

    cl_int err;

public:

    fnode_sender(std::string bs, std::string kn)
    : bs(bs)
    , kn(kn)
    {}

    int svc_init()
    {
        d = new FDevice(bs, kn);
        kernel = d->createKernelInstance();
        write_queue = d->createQueueInstance();
        kernel_queue = d->createQueueInstance();
        read_queue = d->createQueueInstance();

        return 0;
    }

    void * svc (void * t)
    {
        FTaskCL * task = (FTaskCL *)t;
        std::cout << "Receiving task" << std::endl;

        size_t argi = 0;
        // pin host memory buffers
        std::vector<cl::Memory> in_buffers;
        for (FTaskElement & p : task->in) {
            cl::Buffer b = cl::Buffer(d->getContext(),
                                      (CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY),
                                      p.size, p.ptr,
                                      &err);
            if (err != CL_SUCCESS) {
                std::cerr << "ERROR: create buffer for in_buffers" << std::endl;
                return nullptr;
            }
            in_buffers.push_back(b);
            kernel.setArg(argi++, b);
        }

        std::vector<cl::Memory> out_buffers;
        for (FTaskElement & p : task->out) {
            cl::Buffer b = cl::Buffer(d->getContext(),
                                      (CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY),
                                      p.size, p.ptr,
                                      &err);
            if (err != CL_SUCCESS) {
                std::cerr << "ERROR: create buffer for out_buffers" << std::endl;
                return nullptr;
            }
            out_buffers.push_back(b);
            kernel.setArg(argi++, b);
        }

        for (FTaskElement & p : task->scalars) {
            kernel.setArg(argi++, *((int *)p.ptr));
        }

        // migrate input buffers
        std::vector<cl::Event> write_event(1);
        err = write_queue.enqueueMigrateMemObjects(in_buffers, 0, nullptr, &write_event[0]);
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: enqueueMigrateMemObjects for in_buffers" << std::endl;
            return nullptr;
        }
        write_queue.flush();

        std::vector<cl::Event> kernel_event(1);
        err = kernel_queue.enqueueTask(kernel, &write_event, &kernel_event[0]);
        if (err != CL_SUCCESS) return nullptr;
        kernel_queue.flush();

        std::vector<cl::Event> read_event(1);
        err = read_queue.enqueueMigrateMemObjects(out_buffers, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_event, &read_event[0]);
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: enqueueMigrateMemObjects for out_buffers" << std::endl;
            return nullptr;
        }
        read_queue.flush();

        task->write_event.push_back(write_event[0]);
        task->kernel_event.push_back(kernel_event[0]);
        task->read_event.push_back(read_event[0]);

        return t;
    }

    void svc_end()
    {}
};

class fnode_receiver : public ff_node
{
private:

public:

    fnode_receiver()
    {}

    int svc_init(void)
    {
        return 0;
    }

    double event_time_us(cl::Event & e)
    {
        cl_ulong start;
        cl_ulong end;
        e.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
        e.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
        return (end - start) / 1000.0;
    }

    void * svc (void * t)
    {
        FTaskCL * task = (FTaskCL *)t;
        std::cout << "Waiting for task..." << std::endl;
        task->read_event[0].wait();

        std::cout << "TASK: ("
                  << event_time_us(task->write_event[0])  << "us, "
                  << event_time_us(task->kernel_event[0]) << "us, "
                  << event_time_us(task->read_event[0])   << "us) "
                  << std::endl;

        return t;
    }

    void svc_end() {
    }
};


class fnode_pinned_overlap {
private:

    ff_pipeline * p;
    fnode_sender * sender;
    fnode_receiver * receiver;

    void prepare(std::string bs, std::string kn)
    {
        sender = new fnode_sender(bs, kn);
        receiver = new fnode_receiver();

        p = new ff_pipeline();
        p->add_stage(sender);
        p->add_stage(receiver);
    }

public:
    fnode_pinned_overlap()
    {
        prepare("krnl_vadd.xclbin", "krnl_vadd");
    }

    fnode_pinned_overlap(std::string bs, std::string kn)
    {
        prepare(bs, kn);
    }

    ff_pipeline * stage() {
        return p;
    }
};
