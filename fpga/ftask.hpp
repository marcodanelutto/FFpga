#pragma once

#include "fdevice.hpp"

#include <iostream>
#include <string>
#include <vector>


using memories_t = std::vector<cl::Memory>;
using buffers_t = std::vector<cl::Buffer>;
using events_t = std::vector<cl::Event>;
using banks_t = std::vector<int>;


// XCL_MEM_DDR_BANK0
// XCL_MEM_DDR_BANK1
// XCL_MEM_DDR_BANK2
// XCL_MEM_DDR_BANK3

#define MAX_HBM_PC_COUNT 32
#define PC_NAME(n) (n | XCL_MEM_TOPOLOGY)
const int pc_[MAX_HBM_PC_COUNT] = {
    PC_NAME(0),  PC_NAME(1),  PC_NAME(2),  PC_NAME(3),  PC_NAME(4),  PC_NAME(5),  PC_NAME(6),  PC_NAME(7),
    PC_NAME(8),  PC_NAME(9),  PC_NAME(10), PC_NAME(11), PC_NAME(12), PC_NAME(13), PC_NAME(14), PC_NAME(15),
    PC_NAME(16), PC_NAME(17), PC_NAME(18), PC_NAME(19), PC_NAME(20), PC_NAME(21), PC_NAME(22), PC_NAME(23),
    PC_NAME(24), PC_NAME(25), PC_NAME(26), PC_NAME(27), PC_NAME(28), PC_NAME(29), PC_NAME(30), PC_NAME(31)};



struct FTaskElement
{
    void * ptr;
    size_t size;
    banks_t bank_ids;

    FTaskElement(void * ptr, size_t size, banks_t bank_ids)
    : ptr(ptr)
    , size(size)
    , bank_ids(bank_ids)
    {}

    FTaskElement(void * ptr, size_t size, int bank_id)
    : ptr(ptr)
    , size(size)
    , bank_ids({bank_id})
    {}

    FTaskElement(void * ptr, size_t size)
    : ptr(ptr)
    , size(size)
    , bank_ids({})
    {}
};


class FTask
{
private:
    memories_t inputs_d;
    memories_t outputs_d;

    //*************************************************************************
    //
    // Task Executor private functions
    //
    //*************************************************************************

    memories_t create_buffers(FDevice & device, std::vector<FTaskElement> & elems, cl_mem_flags flags)
    {
        cl_int err;

        memories_t buffers;
        for (auto & p : elems) {
            cl::Buffer b;
            if (p.bank_ids.empty()) {
                b = cl::Buffer(device.context_, CL_MEM_USE_HOST_PTR | flags, p.size, p.ptr, &err);
            } else {
                cl_mem_ext_ptr_t ext;
                ext.obj = p.ptr;
                ext.param = 0;
                ext.flags = pc_[p.bank_ids[0]];
                b = cl::Buffer(device.context_, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | flags, p.size, &ext, &err);
            }
            if (err != CL_SUCCESS) {
                std::cerr << "ERROR: failed to create buffer for inputs_d" << std::endl;
                exit(-1);
            }
            buffers.push_back(b);
        }
        return buffers;
    }

    void prepare_buffers(FDevice & device)
    {
        inputs_d = create_buffers(device, inputs, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY);
        outputs_d = create_buffers(device, outputs, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY);
    }

    void migrate_inputs(cl::CommandQueue & queue, FTask * previousTask = nullptr)
    {
        write_events.emplace_back();
        cl_int err = queue.enqueueMigrateMemObjects(inputs_d, 0, (previousTask != nullptr ? &(previousTask->kernel_events) : nullptr), &write_events[0]);
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: failed to migrate inputs_d buffers" << std::endl;
            exit(-3);
        }
    }

    void setKernelArguments(cl::Kernel & kernel)
    {
        size_t argi = 0;
        for (auto & b : inputs_d) {
            kernel.setArg(argi++, b);
        }
        for (auto & b : outputs_d) {
            kernel.setArg(argi++, b);
        }
        for (auto & p : scalars) {
            kernel.setArg(argi++, p.size, p.ptr);
        }
    }

    void enqueueTask(cl::Kernel & kernel, cl::CommandQueue & queue)
    {
        kernel_events.emplace_back();
        cl_int err = queue.enqueueTask(kernel, &write_events, &kernel_events[0]);
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: failed to execute kernel" << std::endl;
            exit(-4);
        }
    }

    void enqueueTask(cl::Kernel & kernel, cl::CommandQueue & queue, FTask * previousTask)
    {
        kernel_events.emplace_back();

        events_t wait_events(write_events);
        if (previousTask != nullptr) wait_events.push_back(previousTask->kernel_events[0]);

        cl_int err = queue.enqueueTask(kernel, &wait_events, &kernel_events[0]);
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: failed to execute kernel" << std::endl;
            exit(-4);
        }
    }

    void migrate_outputs(cl::CommandQueue & queue)
    {
        read_events.emplace_back();
        cl_int err = queue.enqueueMigrateMemObjects(outputs_d, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events, &read_events[0]);
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: failed to migrate outputs_d buffers" << std::endl;
            exit(-5);
        }
    }


public:
    std::vector<FTaskElement> inputs;
    std::vector<FTaskElement> outputs;
    std::vector<FTaskElement> scalars;

    events_t write_events;
    events_t kernel_events;
    events_t read_events;

    FTask() {}

    //*************************************************************************
    //
    // FTaskDescriptor public functions
    //
    //*************************************************************************
    void add_input(size_t size)
    {
        inputs.emplace_back(nullptr, size);
    }

    void add_output(size_t size)
    {
        outputs.emplace_back(nullptr, size);
    }

    void add_input(size_t size, int bank_id)
    {
        inputs.emplace_back(nullptr, size, bank_id);
    }

    void add_output(size_t size, int bank_id)
    {
        outputs.emplace_back(nullptr, size, bank_id);
    }

    void add_input(size_t size, banks_t bank_ids)
    {
        inputs.emplace_back(nullptr, size, bank_ids);
    }

    void add_output(size_t size, banks_t bank_ids)
    {
        outputs.emplace_back(nullptr, size, bank_ids);
    }

    void add_scalar(size_t size)
    {
        scalars.emplace_back(nullptr, size);
    }


    //*************************************************************************
    //
    // FTask public functions
    //
    //*************************************************************************
    void add_input(void * ptr, size_t size)
    {
        inputs.emplace_back(ptr, size);
    }

    void add_output(void * ptr, size_t size)
    {
        outputs.emplace_back(ptr, size);
    }

    void add_scalar(void * ptr, size_t size)
    {
        scalars.emplace_back(ptr, size);
    }

    void add_input(void * ptr, size_t size, int bank_id)
    {
        inputs.emplace_back(ptr, size, bank_id);
    }

    void add_output(void * ptr, size_t size, int bank_id)
    {
        outputs.emplace_back(ptr, size, bank_id);
    }

    void wait()
    {
        cl::Event::waitForEvents(read_events);
    }

    //*************************************************************************
    //
    // Task Executor public functions
    //
    //*************************************************************************

    void enqueue(FDevice & device, cl::Kernel & kernel, cl::CommandQueue & queue, FTask * previousTask = nullptr, bool flush = false)
    {
        prepare_buffers(device);
        setKernelArguments(kernel);

        migrate_inputs(queue);
        enqueueTask(kernel, queue, previousTask);
        migrate_outputs(queue);

        if (flush) queue.flush();
    }
};
