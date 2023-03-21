#pragma once

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1

#include <CL/cl2.hpp>
#include <iostream>
#include <fstream>
#include <string>

#define OCL_CHECK(error, call)                                                                   \
    call;                                                                                        \
    if (error != CL_SUCCESS) {                                                                   \
        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
        exit(EXIT_FAILURE);                                                                      \
    }

auto get_time() { return std::chrono::high_resolution_clock::now(); }
auto time_diff_ms(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); 
}

class FDevice
{
private:
    cl::Platform platform_;
    cl::Program  program_;

    void load_platform()
    {
        cl_int err;

        bool found_platform = false;
        std::vector<cl::Platform> platforms;
        OCL_CHECK(err, err = cl::Platform::get(&platforms));
        for (int i = 0; i < platforms.size(); ++i) {
            cl::Platform platform = platforms[i];
            OCL_CHECK(err, std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err));
            if (platformName == "Xilinx") {
                platform_ = platform;
                found_platform = true;
                break;
            }
        }
        if (!found_platform) {
            std::cerr << "ERROR: failed to find Xilinx platform" << std::endl;
            exit(-1);
        }
    }

    void load_device(int device_id)
    {
        cl_int err;

        bool found_device = false;
        std::vector<cl::Device> devices;
        OCL_CHECK(err, err = platform_.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));

        if (devices.size() == 0) {
            std::cerr << "ERROR: failed to find a Xilinx device" << std::endl;
            exit(-2);
        }

        if (device_id < 0) device_id = 0;
        if (device_id < devices.size()) {
            device_ = devices[device_id];
            found_device = true;
        } else {
            std::cerr << "ERROR: failed to find the Xilinx device[" << device_id << "]" << std::endl;
            exit(-3);
        }
    }

    void load_context()
    {
        cl_int err;
        OCL_CHECK(err, context_ = cl::Context(device_, NULL, NULL, NULL, &err));
    }

    void load_program(std::string bitstream)
    {
        cl_int err;

        std::cout << "Loading " << bitstream << " bitstream...\n" << std::endl;

        FILE * fp;
        if ((fp = fopen(bitstream.c_str(), "r")) == nullptr) {
            std::cerr << "ERROR: " << bitstream << " xclbin not available" << std::endl;
            exit(-4);
        }

        // Load xclbin
        std::ifstream bin_file(bitstream, std::ifstream::binary);
        bin_file.seekg (0, bin_file.end);
        unsigned nb = bin_file.tellg();
        bin_file.seekg (0, bin_file.beg);
        char * buf = new char[nb];
        bin_file.read(buf, nb);

        cl::Program::Binaries bins;
        bins.push_back({buf, nb});

        std::vector<cl::Device> program_devices;
        program_devices.push_back(device_);

        OCL_CHECK(err, program_ = cl::Program(context_, program_devices, bins, NULL, &err));
    }

public:
    cl::Device       device_;
    cl::Context      context_;
    cl::CommandQueue queue_;

    FDevice(std::string bitstream, int device_id = 0)
    {
        load_platform();
        load_device(device_id);
        load_context();
        load_program(bitstream);
        queue_ = createCommandQueue(false, true);
    }

    cl::Kernel createKernel(std::string kernel_name)
    {
        cl_int err;
        cl::Kernel kernel = cl::Kernel(program_, kernel_name.c_str(), &err);
        if (err != CL_SUCCESS) {
            std::cerr << "ERROR: failed to create an instance of " << kernel_name << " kernel" << std::endl;
            exit(-5);
        }
        return kernel;
    }

    cl::CommandQueue createCommandQueue(bool profiling = false, bool out_of_order = false)
    {
        cl_int err;
        cl::CommandQueue queue;
        OCL_CHECK(err, queue = cl::CommandQueue(context_,
                                                device_,
                                                (profiling ? cl::QueueProperties::Profiling : cl::QueueProperties::None)
                                                | (out_of_order ? cl::QueueProperties::OutOfOrder : cl::QueueProperties::None),
                                                &err));
        return queue;
    }

    bool is_emulation()
    {
        char * xcl_mode = getenv("XCL_EMULATION_MODE");
        if (xcl_mode != nullptr) {
            return true;
        }
        return false;
    }

    bool is_sw_emulation()
    {
        char * xcl_mode = getenv("XCL_EMULATION_MODE");
        if ((xcl_mode != nullptr) && !strcmp(xcl_mode, "sw_emu")) {
            return true;
        }
        return false;
    }

    bool is_hw_emulation()
    {
        char * xcl_mode = getenv("XCL_EMULATION_MODE");
        if ((xcl_mode != nullptr) && !strcmp(xcl_mode, "hw_emu")) {
            return true;
        }
        return false;
    }
};
