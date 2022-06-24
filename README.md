# FFpga
offloading from FastFlow nodes to Vitis kernels on Alveo FPGAs

### ff_node_fpga
The ff_node_fpga can be used in any place where you need an ff_node/ff_node_t. It offloads tasks to pre-compiled kernels on an Alveo FPGA 

## Sample code (so far)
### pip2 
pip2 kernelname bitstream executes a vadd like kernel on a range of vector pairs randomly generated in a three stage pipeline
