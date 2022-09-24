#pragma once

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1

#include <CL/cl2.hpp>
#include <iostream>
#include <fstream>
#include <iostream>
#include <string>
// pre unique_ptr
#include <memory>
#include "task.hpp"

// per fastflow
#include <ff/ff.hpp>

#define OCL_CHECK(error, call) \
call; \
if (error != CL_SUCCESS) { \
    printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
    exit(EXIT_FAILURE); \
}


class FDevice {

private:
    std::string bs;
    std::string kn;

    cl_int err;
    cl::Device device;
    cl::Context context;


public:

    FDevice(std::string bs, std::string kn)
    : bs(bs)
    , kn(kn)
    {
        bool found_device = false;
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        for(size_t i = 0; i < platforms.size(); ++i) {
            cl::Platform platform = platforms[i];
            std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
            if (platformName == "Xilinx") {
                std::vector<cl::Device> devices;
                platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
                if (devices.size()){
                    device = devices[0];
                    found_device = true;
                    break;
                }
            }
        }

        if (found_device) {
            std::cerr << "MDLOG: Device found" << std::endl;
        } else {
            std::cerr << "MDLOG: Device NOT found" << std::endl;
            exit(-1);
        }

        context = cl::Context(device, NULL, NULL, NULL, &err);
        if(err != CL_SUCCESS) {
            std::cerr << "MDLOG: Context CANNOT be created" << std::endl;
            exit(-2);
        }
        std::cerr << "MDLOG: got context " << std::endl;
    }

    cl::Context getContext()
    {
        return context;
    }


    cl::Kernel createKernelInstance()
    {
        FILE* fp;
        if ((fp = fopen(bs.c_str(), "r")) == nullptr) {
            printf("ERROR: %s xclbin not available please build\n", bs.c_str());
            exit(-1);
        }

        // Load xclbin
        std::ifstream bin_file(bs, std::ifstream::binary);
        bin_file.seekg (0, bin_file.end);
        unsigned nb = bin_file.tellg();
        bin_file.seekg (0, bin_file.beg);
        char *buf = new char [nb];
        bin_file.read(buf, nb);

    // Creating Program from Binary File
        cl::Program::Binaries bins;
        bins.push_back({buf,nb});

        std::vector<cl::Device> devices;
        devices.push_back(device);

        cl::Program program(context, devices, bins, NULL, &err);
        if(err != CL_SUCCESS) {
            std::cout << "Error loading program code from xclbin" << std::endl;
            exit(-2);
        }

        cl::Kernel krnl = cl::Kernel(program, kn.c_str(), &err);
        if(err != CL_SUCCESS) {
            std::cout << "Error creating kernel from xclbin" << std::endl;
            exit(-3);
        }
        return krnl;
    }

    cl::CommandQueue createQueueInstance()
    {
        cl::CommandQueue q = cl::CommandQueue(context, device, /*CL_QUEUE_PROFILING_ENABLE |*/ CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
        if (err != CL_SUCCESS) {
            std::cerr << "Error creating CommandQueue" << std::endl;
            exit(-1);
        }
        return q;
    }
};
