#pragma once

#include <iostream>
#include <ff/ff.hpp>

const bool mdlog = false; 

#include <fpga/hostinsterfaceutils.hpp>
#include <fpga/task.hpp> 

#include <utils/utimer.cpp>


using namespace std; 
using namespace ff;

class ff_node_fpga : public ff::ff_node {
private:
  FTask task_description;
  FpgaDevice * d;
  bool loadKernel; 
  string kernelName; 
  string bitStream; 
public:

  ff_node_fpga(FTask task_description) : kernelName("krnl_vadd"), bitStream("krnl_vadd.xclbin"), task_description(task_description), loadKernel(true) {}
  ff_node_fpga(string kn, string bs, FTask task_description) : 
	  kernelName(kn), bitStream(bs), task_description(task_description), loadKernel(true) {}
  ff_node_fpga(string kn, string bs, FTask task_description, bool lk) : 
	  kernelName(kn), bitStream(bs), task_description(task_description), loadKernel(lk) {}
  
  int svc_init(void) {
    utimer svcinit("SVC INIT");
    d = new FpgaDevice{};
    if(!d->DeviceExists()) 
      return(-1);
    if(mdlog) std::cerr << "MDLOG: device found!" << std::endl;
    
    if(loadKernel) {
      if(!d->CreateKernel(bitStream, kernelName))
        return(-1);
      if(mdlog) std::cerr << "MDLOG: kernel created!" << std::endl;
    }
    
    if(!d->setupBuffers(task_description))
      return(-1);
    if(mdlog) std::cerr << "MDLOG: bufferes created!" << std::endl;
    return(0);
  }

  void svc_end() {
    utimer svcend("SVC END");
    if(d->finalize())
      cout << "Errore nella finalize " << endl;
    else
      cout << "Finalizzazione terminata con successo "<< endl;

    d->cleanup();
    return;
 }
  
  void * svc (void * t) {


//XRT allocates memory space in 3K boundary for internal memory management. If the host memory pointer is not aligned to a page boundary, XRT performs extra memcpy to make it aligned.

    FTask * task = (FTask *) t;
    {
      utimer svctask("task");
      if(!d->computeTask(task))
        return(NULL);
    
      if(!d->waitTask())
        return(NULL);
      if(mdlog) std::cerr << "MDLOG:  task computed!" << std::endl;
    }


    return t; 
  }
};
