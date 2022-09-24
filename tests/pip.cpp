#include <iostream>
#include <sstream>
#include <ff/ff.hpp>

using namespace std;
using namespace ff;

// #include <fpga/fpganode.hpp>
#include <fpga/fnode_overlap.hpp>


void printTask(FTaskCL * task, string sender)
{
    int * a = (int *)(task->in[0].ptr);
    int * b = (int *)(task->in[1].ptr);
    int * c = (int *)(task->out[0].ptr);

    std::ostringstream os;

    os << sender << endl;
    os << "a = {";
    for (int i = 0; i < task->in[0].size / sizeof(int); i++) os << a[i] << " ";
    os << "}" << endl;
    os << "b = {";
    for (int i = 0; i < task->in[1].size / sizeof(int); i++) os << b[i] << " ";
    os << "}" << endl;
    os << "c = {";
    for (int i = 0; i < task->out[0].size / sizeof(int); i++) os << c[i] << " ";
    os << "}" << endl;
    cout << os.str();
}

class gen : public ff_node {
private:
    int n;    // size of the vectors
    int m;    // tasks in the stream
    int max;  // max value of the vector items

public:
    gen(int n, int m, int max)
    : n(n)
    , m(m)
    , max(max)
    {}

    void * svc(void * t)
    {
        size_t size_in_bytes = n * sizeof(int);

        for(int i = 0; i < m; i++) {
            int * a = (int *) malloc(size_in_bytes);
            int * b = (int *) malloc(size_in_bytes);
            int * c = (int *) malloc(size_in_bytes);
            int * size = (int *) malloc(sizeof(int));

            for(int j = 0; j < n; j++) {
                a[j] = rand() % max;
                b[j] = rand() % max;
            }
            *size = n;

            FTaskCL * task = new FTaskCL();
            task->addIn(a, size_in_bytes);
            task->addIn(b, size_in_bytes);
            task->addOut(c, size_in_bytes);
            task->addScalar(size, sizeof(int));

            printTask(task, "GEN");

            ff_send_out((void *) task);
        }

        return(EOS);
    }
};

//
//  this is the drain, collapsing the stream (video print)
//
class drain : public ff_node {
public:
    int svc_init()
    {
        return(0);
    }

    void * svc(void * t)
    {
        if (t) {
            FTaskCL * task = (FTaskCL *) t;
            printTask(task, "DRAIN");
        }
        return(GO_ON);
    }
};

//
// main program : sets up the pipeline with the FPGA node in the middle
//
int main(int argc, char * argv[])
{

    string kernelName = "krnl_vadd";
    string bitStream  = "krnl_vadd.xclbin";
    if(argc > 1) {
        kernelName = argv[1];
        bitStream = argv[2];
    }
    cout << "Exec kernel " << kernelName << " with "  << bitStream << endl;


    // Initial configuration
    int n = 4;
    int m = 8;
    int max = 16;

    // Define a Task for the FPGA computation
    size_t size_in_bytes = n * sizeof(int);

    FTaskCL task_description;
    task_description.addIn(size_in_bytes);
    task_description.addIn(size_in_bytes);
    task_description.addOut(size_in_bytes);
    task_description.addScalar(sizeof(int));

    ff_pipeline p;
    // add "classic" generator stage
    p.add_stage(new gen(n, m, max));

    // Create a FPGA node and place it into the pipeline
    fnode_overlap fnp(bitStream, kernelName, task_description);
    p.add_stage(fnp.stage());

    // add "classic" drain stage
    p.add_stage(new drain{});

    p.run_and_wait_end();             // just run it

  return(0);
}
