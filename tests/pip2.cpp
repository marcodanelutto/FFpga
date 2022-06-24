#include <iostream>
#include <ff/ff.hpp>

using namespace std;
using namespace ff;

#include "fpganodo.hpp"

class gen : public ff_node {
private:
  int n; 	// size	of the vectors
  int m; 	// tasks in the stream
  int max; 	// max value of the vector items

public:
  gen(int n, int m, int max) : n(n) , m(m) , max(max) {}

  void * svc(void * t) {

    size_t size_in_bytes = n * sizeof(int);

    for(int i=0; i<m; i++) {			// for as many items in the stream
      int * a = (int *) malloc(size_in_bytes);  // allocate memory for the task
      int * b = (int *) malloc(size_in_bytes);
      int * c = (int *) malloc(size_in_bytes);
      int * size = (int *)malloc(sizeof(int));
      for(int j=0; j<n; j++) {			// fill in dummy values
        a[j] = rand() % max;
        b[j] = rand() % max;
      }
      *size = n;

      FTask * task = (FTask *) malloc(sizeof(FTask));	// create task 
      task->in.push_back({a, size_in_bytes});		
      task->in.push_back({b, size_in_bytes});
      task->out.push_back({c, size_in_bytes});
      task->scalars.push_back({size, sizeof(int)});

      ff_send_out((void *) task);			// eventually send it out
      cout << "GEN: sent " << i << " size was " << sizeof(task) << " " << sizeof(FTask)<<  endl; 
    }
    cout << "GEN: going to finish" << endl; 
    return(EOS);
  }
};

// 
//  this is the drain, collapsing the stream (video print)
//
class drain : public ff_node {
public:
  int svc_init() {
    cout << "DRAIN: started" << endl;
    return(0);
  }
  void * svc(void * t) {
    FTask * task = (FTask *) t;
    cout << "DRAIN got task" << endl; 
    cout << "DRAIN" << endl;

    int * a = (int *)(task->in[0].first);
    int * b = (int *)(task->in[1].first);
    int * c = (int *)(task->out[0].first);

    for(int i=0; i<task->in[0].second; i++) cout << a[i] << " ";
    cout << endl ; 
    for(int i=0; i<task->in[1].second; i++) cout << b[i] << " ";
    cout << endl ; 
    if(c != NULL) {
      for(int i=0; i<task->out[0].second; i++) cout << c[i] << " ";
      cout << endl ;
    }
    return(GO_ON);
  }
};

// 
// main program : sets up the pipeline with the FPGA node in the middle
//
int main(int argc, char * argv[]) {

  string kernelName = "krnl_vadd";
  string bitStream  = "krnl_vadd.xclbin"; 
  if(argc != 1) {
    kernelName = argv[1];
    bitStream = argv[2]; 
  }
  cout << "Exec " << bitStream << endl; 


  int n = 4;
  int m = 8;
  int max = 16;
  size_t size_in_bytes = n * sizeof(int);

  // 
  // this is the descriptor of the tasks to be managed on the FPGA
  // in. items are those we need to copy task -> FPGA
  // out items are those we need to copy bask FPGA -> task
  //
  FTask task_description;
  task_description.in.push_back({nullptr, size_in_bytes});
  task_description.in.push_back({nullptr, size_in_bytes});
  task_description.out.push_back({nullptr, size_in_bytes});
  task_description.scalars.push_back({nullptr, sizeof(int)});
  FNode s2(kernelName, bitStream, task_description);

  ff_pipeline p;				// create pipeline
  p.add_stage(new gen(n, m, max));		// add "classic" source stage
  p.add_stage(s2);	// add FPGA stage (FPGA not found -> this fails)
  p.add_stage(new drain{});			// add "classic" drain stage

  p.run_and_wait_end();				// just run it

  return(0);
}
