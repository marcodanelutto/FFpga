KERNCXX = v++ 
KERNPLAT = --platform /opt/xilinx/platforms/xilinx_u50_gen3x16_xdma_201920_3/xilinx_u50_gen3x16_xdma_201920_3.xpfm
TARGET = sw_emu

%.xo: 	%.cpp
	$(KERNCXX) -t $(TARGET) $(KERNPLAT) -o $@ -c -k $* $<
kernels: 
	$(KERNCXX) -t $(TARGET) $(KERNPLAT) -o krnl_vsub.xo -c -k krnl_vsub krnl_vsub.cpp

%.xclbin:	%.xo
	$(KERNCXX) -t $(TARGET)  $(KERNPLAT) --link $< -o $@ --save-temps ### --config ./connectivity.cfg 

clean:
	rm -Rf *log *summary *info _x *.xo
