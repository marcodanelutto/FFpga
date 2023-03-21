#pragma once

#include "ff/ff.hpp"
#include "ftask.hpp"

namespace FNodeTaskInternals
{
class FNodeEnqueuer : public ff::ff_node
{
private:
    FDevice & device;
    std::string kernel_name;
    cl::CommandQueue queue;

    bool chain_tasks;
    FTask * previous_task;

public:
    FNodeEnqueuer(FDevice & device, std::string kernel_name, bool chain_tasks = false)
    : device(device)
    , kernel_name(kernel_name)
    , queue(device.createCommandQueue())
    , chain_tasks(chain_tasks)
    , previous_task(nullptr)
    {}

    void * svc (void * t)
    {
        FTask * task = (FTask *)t;
        cl::Kernel kernel = device.createKernel(kernel_name);
        task->enqueue(device, kernel, queue, previous_task);

        if (chain_tasks) {
            previous_task = task;
        }

        return t;
    }
};

class FNodeReader : public ff::ff_node {
public:
    void * svc (void * t)
    {
        FTask * task = (FTask *)t;
        task->wait();
        return t;
    }
};
}

class FNodeTask : public ff::ff_pipeline {
public:
    FNodeTask(FDevice & device, std::string kernel_name, bool chain_tasks = false)
    {
        add_stage(new FNodeTaskInternals::FNodeEnqueuer(device, kernel_name, chain_tasks));
        add_stage(new FNodeTaskInternals::FNodeReader());
        cleanup_nodes();
    }
};
