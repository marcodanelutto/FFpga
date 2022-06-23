#pragma once
#include <vector>
#include <utility>


typedef struct __task_fpga {
    std::vector<std::pair<void *, size_t>> in;      // vector of (host_ptr, size_in_bytes) for IN device buffers
    std::vector<std::pair<void *, size_t>> out;     // vector of (host_ptr, size_in_bytes) for OUT device buffers
    std::vector<std::pair<void *, size_t>> scalars; // vector of (value, size_in_bytes) for scalar values
} FTask;
