HOSTCXX = g++
HOSTCXXFLAGS = -std=c++17 -I$(XILINX_VIVADO)/include -I$(XILINX_XRT)/include/ -I .. -O3
HOSTLDFLAGS = -L$(XILINX_XRT)/lib/  -pthread -lrt -lstdc++

%:	%.cpp
	$(HOSTCXX) $(HOSTCXXFLAGS) $(HOSTLDFLAGS) -o $@ $< /usr/lib/x86_64-linux-gnu/libOpenCL.so -g -I. 

test:
	$(HOSTCXX) -I ../fpga  -o hls_test hls_stream.test.cpp 

clean:
	rm -f *.out main pip pip1 hls_test pip2 pip3

run2hw:
	./pip2 krnl_vadd ../../vadd-hw/krnl_vadd.xclbin
