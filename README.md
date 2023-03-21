# FFpga
Experimental FastFlow node called FNodeTask to offload computation of Vitis HLS kernels on Alveo FPGAs.

### FNodeTask
The FNodeTask can be used in any place where you need an ff_node/ff_node_t.
It offloads tasks to pre-compiled kernels on an Alveo FPGA

## Sample code
The `vadd_cus` program executes a VectorAdd (vadd) kernel composed of multiple Compute Units (CUs).

### Compile `vadd` kernel
```
cd kernels/vadd_cus
make all TARGET=hw # sw_emu or hw_emu
```

### Compile `vadd_cus` host
```
cd test
make vadd_cus
```

### Run
You can use tests contained in the `Makefile` as follows:

```
$ make test_pipe    # runs a pipeline containing FNodeTask
$ make test_farm    # runs a farm of pipeline containing FNodeTask
```

By executing `vadd_cus` without parameters it will show:

```
$ ./vadd_cus
Usage:
        ./vadd_cus [file.xclbin] [kernel_name] [num_workers] [chain_tasks] [vec_elems] [vec_nums]

Example:
        ./vadd_cus vadd.xclbin vadd 2 0 $((1 << 10)) $((1 << 6))
```

