HOSTCXX = g++
HOSTCXXFLAGS = -std=c++17 -I$(XILINX_VIVADO)/include -I$(XILINX_XRT)/include/ -I .. -O3
HOSTLDFLAGS = -L$(XILINX_XRT)/lib/  -pthread -lrt -lstdc++

%:	%.cpp
	$(HOSTCXX) $(HOSTCXXFLAGS) $(HOSTLDFLAGS) -o $@ $< /usr/lib/x86_64-linux-gnu/libOpenCL.so -g -I. 

clean:
	rm -f *.out main pip pip2 pip3
