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
    cl::Program program;


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

        context = createContext();
        program = createProgram();
    }

    cl::Context createContext()
    {
        cl::Context _context = cl::Context(device, NULL, NULL, NULL, &err);
        if(err != CL_SUCCESS) {
            std::cerr << "MDLOG: Context CANNOT be created" << std::endl;
            exit(-2);
        }
        std::cerr << "MDLOG: got context " << std::endl;

        return _context;
    }

    cl::Context getContext()
    {
        return context;
    }

    cl::Program createProgram()
    {
        std::cout << "INFO: Reading " << bs << std::endl;
        FILE* fp;
        if ((fp = fopen(bs.c_str(), "r")) == nullptr) {
            printf("ERROR: %s xclbin not available please build\n", bs.c_str());
            exit(EXIT_FAILURE);
        }
        // Loading XCL Bin into char buffer
        std::cout << "Loading: '" << bs.c_str() << "'\n";
        std::ifstream bin_file(bs.c_str(), std::ifstream::binary);
        bin_file.seekg(0, bin_file.end);
        auto nb = bin_file.tellg();
        bin_file.seekg(0, bin_file.beg);
        std::vector<unsigned char> buf;
        buf.resize(nb);
        bin_file.read(reinterpret_cast<char*>(buf.data()), nb);

        // Creating Program from Binary File
        cl::Program::Binaries bins{{buf.data(), buf.size()}};

        std::vector<cl::Device> devices{device};
        // devices.push_back(device);

        cl::Program program = cl::Program(context, devices, bins, nullptr, &err);
        if(err != CL_SUCCESS) {
            std::cout << "Error loading program code from xclbin" << std::endl;
            exit(-2);
        }
        return program;
    }


    cl::Kernel createKernelInstance()
    {
        cl::Kernel krnl = cl::Kernel(program, kn.c_str(), &err);
        if(err != CL_SUCCESS) {
            std::cout << "Error creating kernel from xclbin" << std::endl;
            exit(-3);
        }
        return krnl;
    }

    cl::CommandQueue createQueueInstance()
    {
        cl::CommandQueue q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
        if (err != CL_SUCCESS) {
            std::cerr << "Error creating CommandQueue" << std::endl;
            exit(-1);
        }
        return q;
    }
};
