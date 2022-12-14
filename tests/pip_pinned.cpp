#include <memory>
#include <iostream>
#include <sstream>
#include <ff/ff.hpp>
#include <fpga/fnode_pinned_overlap.hpp>

using namespace std;
using namespace ff;

#define PRINT_TASK 0

void printTask(FTaskCL * task, string sender)
{
#if PRINT_TASK
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
#endif
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
            int ret;
            int * a; ret  = posix_memalign((void **)&a, 4096, size_in_bytes);
            int * b; ret |= posix_memalign((void **)&b, 4096, size_in_bytes);
            int * c; ret |= posix_memalign((void **)&c, 4096, size_in_bytes);
            int * size = (int *) malloc(sizeof(int));

            if (ret != 0) {
                std::cerr << "ERROR: allocating aligned memory!" << std::endl;
                return nullptr;
            }

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

int main(int argc, char * argv[])
{
    argc--;
    argv++;

    string bitstream  = "krnl_vadd.xclbin";
    string kernelName = "krnl_vadd";
    int n = 1 << 15;
    int m = 1 << 6;
    int max = 1 << 10;

    int argi = 0;
    if (argc > argi) bitstream = string(argv[argi++]);
    if (argc > argi) kernelName = string(argv[argi++]);
    if (argc > argi) n = atoi(argv[argi++]);
    if (argc > argi) m = atoi(argv[argi++]);
    if (argc > argi) max = atoi(argv[argi++]);

    cout << "Executing " << kernelName << " with " << bitstream << endl;

    ff_pipeline p;
    p.add_stage(new gen(n, m, max));

    // Create a FPGA node and place it into the pipeline
    fnode_pinned_overlap fnp(bitstream, kernelName);
    p.add_stage(fnp.stage());

    p.add_stage(new drain{});
    p.run_and_wait_end();

    std::cout << "Time Spent: " << p.ffTime() << std::endl;

  return(0);
}
