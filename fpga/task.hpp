#pragma once

#include <vector>
#include <utility>

double event_time_us(cl::Event & e)
{
    cl_ulong start;
    cl_ulong end;
    e.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    e.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    return (end - start) / 1000.0;
}

double events_time_us(std::vector<cl::Event> & events)
{
    double time_sum = 0;
    for (auto & e : events) {
        time_sum += event_time_us(e);
    }
    return time_sum;
}


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

    void * user_data;

    std::vector<cl::Event> write_event;
    std::vector<cl::Event> kernel_event;
    std::vector<cl::Event> read_event;

    void setUserData(void * data)
    {
        user_data = data;
    }

    void * getUserData()
    {
        return user_data;
    }

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

    double write_time_us() { return event_time_us(write_event[0]); }
    double write_all_time_us() { return events_time_us(write_event); }
    double kernel_time_us() { return event_time_us(kernel_event[0]); }
    double kernel_all_time_us() { return events_time_us(kernel_event); }
    double read_time_us() { return event_time_us(read_event[0]); }
    double read_all_time_us() { return events_time_us(read_event); }

    // WARNING: this time value might be wrong if transfers occurs in parallel
    double all_time_us() {
        return write_all_time_us() + kernel_all_time_us() + read_all_time_us();
    }
};