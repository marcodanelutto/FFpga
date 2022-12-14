#pragma once
#include <vector>
#include <utility>


typedef struct __task_fpga {
    std::vector<std::pair<void *, size_t>> in;      // vector of (host_ptr, size_in_bytes) for IN device buffers
    std::vector<std::pair<void *, size_t>> out;     // vector of (host_ptr, size_in_bytes) for OUT device buffers
    std::vector<std::pair<void *, size_t>> scalars; // vector of (value, size_in_bytes) for scalar values
} FTask;


struct FTaskElement
{
    void * ptr;
    size_t size;
    cl::Buffer * buff;

    FTaskElement(void * ptr, size_t size, cl::Buffer * buff)
    : ptr(ptr)
    , size(size)
    , buff(buff)
    {}
};

struct FTaskCL
{
    std::vector<FTaskElement> in;      // vector of (host_ptr, size_in_bytes) for IN device buffers
    std::vector<FTaskElement> out;     // vector of (host_ptr, size_in_bytes) for OUT device buffers
    std::vector<FTaskElement> scalars; // vector of (value, size_in_bytes) for scalar values

    std::vector<cl::Event> write_event;
    std::vector<cl::Event> kernel_event;
    std::vector<cl::Event> read_event;

    void addIn(size_t size)
    {
        in.emplace_back(nullptr, size, nullptr);
    }

    void addOut(size_t size)
    {
        out.emplace_back(nullptr, size, nullptr);
    }

    void addScalar(size_t size)
    {
        scalars.emplace_back(nullptr, size, nullptr);
    }

    void addIn(void * ptr, size_t size)
    {
        in.emplace_back(ptr, size, nullptr);
    }

    void addOut(void * ptr, size_t size)
    {
        out.emplace_back(ptr, size, nullptr);
    }

    void addScalar(void * ptr, size_t size)
    {
        scalars.emplace_back(ptr, size, nullptr);
    }
};