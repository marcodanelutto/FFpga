HOSTCXX = g++
HOSTCXXFLAGS = -std=c++14 -I$(XILINX_VIVADO)/include -I$(XILINX_XRT)/include/ -I .. -I ../fpga -O3
HOSTLDFLAGS = -L$(XILINX_XRT)/lib/  -pthread -lrt -lstdc++

FFINCLUDE = -I ~/fastflow -DTRACE_FASTFLOW

%:	%.cpp
	$(HOSTCXX) $(HOSTCXXFLAGS) $(HOSTLDFLAGS) $(FFINCLUDE) -o $@ $< /usr/lib/x86_64-linux-gnu/libOpenCL.so -I. 


test_pipe: vadd_cus
	./vadd_cus ../kernels/vadd_cus/vadd.xclbin vadd 0 0 1024 64

test_farm: vadd_cus
	./vadd_cus ../kernels/vadd_cus/vadd.xclbin vadd 8 0 1024 64

test_pipe2: vadd_pipe
	./vadd_pipe ../kernels/vadd_cus/vadd.xclbin vadd 0 0 1024 64

test_farm2: vadd_pipe
	./vadd_pipe ../kernels/vadd_cus/vadd.xclbin vadd 8 0 1024 64

clean:
	rm -f *.out test_ftask

cleanall:
	$(RM) -rf sd_host *.o *.xo _x .run .Xil *.xclbin *.ltx *.log *.info *compile_summary* vitis_analyzer* *link_summary* *.o main emconfig.json device_trace_0.csv *.xclbin.package_summary opencl_trace.csv xrt.run_summary summary.csv .ipcache waveform_prefs.json

