#include <memory>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <ff/ff.hpp>

using namespace std;
using namespace ff;

#define PINNED 0

#if PINNED
#include <fpga/fnode_pinned_overlap.hpp>
#else
#include <fpga/fnode_overlap.hpp>
#endif

#define MAX_BUF_SIZE (32*1024*1024)

const char * filenames[] = {
    "2_OHara_S1_rbcLa_2019_minq7.fastq",
    "bible_genesi",
    "bitstream",
    "6_Swamp_S1_ITS2_2019_minq7.fastq",
    "enwik8",
    "fotoTastiera",
};

#define TEXT_MAX_LEN 256

struct FileZip {
    char filename[TEXT_MAX_LEN];
    char filename_txt[TEXT_MAX_LEN];
    char filename_zip[TEXT_MAX_LEN];
    size_t size_txt;
    size_t size_zip;
    double wall_clock;
    double elapsed;
    
    double compression_ratio() {
        return (double)size_txt / (double)size_zip;
    }

    double throughput() {
        return (double)size_txt / elapsed * (1e6 / (1024.0 * 1024.0));
    }
};

double computeElapsed(timeval ts, timeval te)
{
     double retVal = 0.0;
     retVal = (te.tv_sec-ts.tv_sec)*1000000.0+(te.tv_usec-ts.tv_usec);
     return retVal;
}

class gen : public ff_node {
private:
    int n;

public:
    void * svc(void * t) {
        

        for (auto * filename : filenames) {
            timeval time_start_read, time_end_read;
            gettimeofday(&time_start_read,NULL);

            FileZip * file = (FileZip *)malloc(sizeof(FileZip));
            snprintf(file->filename, TEXT_MAX_LEN, "%s", filename);
            snprintf(file->filename_txt, TEXT_MAX_LEN, "txt/%s.txt", filename);
            
            // checks that the file exists
            FILE * fd = fopen(file->filename_txt, "rb"); 
            if (fd == NULL) {
                cout << "Error opening file" << endl; 
                exit(-1);
            }
            // gets the file size
            fseek(fd, 0L, SEEK_END); 
            file->size_txt = ftell(fd); 
            rewind(fd);

            // WARNING: capped the size of the file to MAX_BUF_SIZE
            if (file->size_txt > MAX_BUF_SIZE) {
                file->size_txt = MAX_BUF_SIZE;
            }   
            
            // allocates task
            int ret = 0;
            char * txt; ret |= posix_memalign((void **)&txt, 4096, file->size_txt);
            char * zip; ret |= posix_memalign((void **)&zip, 4096, file->size_txt);
            int * zipsize; ret |= posix_memalign((void **)&zipsize, 4096, sizeof(int));
            int * txtsize; ret |= posix_memalign((void **)&txtsize, 4096, sizeof(int));

            if (ret != 0) {
                std::cerr << "ERROR: allocating aligned memory!" << std::endl;
                return nullptr;
            }

            // reads the file (up to MAX_BUF_SIZE)
            fread((void *)txt, 1, file->size_txt, fd); 
            fclose(fd);

            // sets the filesize            
            *txtsize = file->size_txt;
            
            // creates the task
            FTaskCL * task = new FTaskCL();
            task->addIn(txt, file->size_txt);
            task->addOut(zip, file->size_txt);
            task->addOut(zipsize, sizeof(int));
            task->addScalar(txtsize, sizeof(int));
            task->setUserData((void *)file);

            gettimeofday(&time_end_read,NULL);

            file->wall_clock = computeElapsed(time_start_read, time_end_read);

            ff_send_out((void *) task);
        }

        return(EOS);
    }
};

class drain : public ff_node {
private:
    bool pass; 
    double total_time;
public:
    drain()
    : pass(true)
    , total_time(0)
    {}

    void svc_end(void) {
        printf("\n\ntotal deflating elapsed time : %lf usec\n", total_time);
        std::cout << "CONVERTER TEST " << (pass ? "PASSED" : "FAILED") << std::endl;
    }

    void * svc(void * t) {
        timeval time_start_write, time_end_write;
        gettimeofday(&time_start_write,NULL);

        FTaskCL * task = (FTaskCL *)t;

        char * zip = (char *)(task->out[0].ptr);
        int * zipsize = (int *)(task->out[1].ptr);

        FileZip * file = (FileZip *)(task->getUserData());
        snprintf(file->filename_zip, TEXT_MAX_LEN, "zip/%s.zip", file->filename);
        file->size_zip = *zipsize;

        // cout << "Drain got a result of " << file->size_zip << " bytes " << endl; 
        FILE * fd = fopen(file->filename_zip, "w");
        if(fd == NULL) {
            cout << "Error opening outfile" << endl;
            exit(-1);
        }
        fwrite((void *)zip, 1, file->size_zip, fd); 
        fclose(fd);
        gettimeofday(&time_end_write,NULL);
        file->wall_clock += computeElapsed(time_start_write, time_end_write);

        file->elapsed = task->kernel_all_time_us();
        file->wall_clock += task->all_time_us();
        total_time += file->wall_clock;

        printf("%s: Input File Size = %d [B]\n", file->filename_txt, file->size_txt);
        printf("%s: Output File Size = %d [B]\n", file->filename_zip, file->size_zip);
        printf("Compression Ratio CR = %5.2f\n", file->compression_ratio());

        printf("Deflating elapsed time : %lf usec\n", file->elapsed);
        printf("Overall deflating elapsed time : %lf usec\n", file->wall_clock);

        printf("Throughput = %lf [MB/sec]\n", file->throughput());

        free(task->in[0].ptr);
        free(task->out[0].ptr);
        free(task->out[1].ptr);
        free(task->user_data);

        return(GO_ON);
    }
};

int main(int argc, char * argv[])
{
    argc--;
    argv++;

    int nworkers = 1;
    string bitStream  = "DeflateKernel.xclbin";
    string kernelName = "DeflateKernel:{DeflateKernel_1}";

    if (argc > 0) nworkers = atoi(argv[0]);
    if (argc > 1) bitStream = string(argv[1]);
    if (argc > 2) kernelName = string(argv[2]);

    cout << "Exec kernel " << kernelName << " with " << bitStream << endl; 


    ff_farm farm;
    farm.add_emitter(new gen()); 

    FDevice * device = new FDevice(bitStream, kernelName);
    std::vector<ff_node *> w;

#if PINNED
    for(int i = 0; i < nworkers; ++i) {
        fnode_pinned_overlap * worker = new fnode_pinned_overlap(device);
        w.push_back(worker->stage());
    }
#else
    size_t size_in_bytes = MAX_BUF_SIZE;

    FTaskCL task_description;
    task_description.addIn(size_in_bytes);
    task_description.addOut(size_in_bytes);
    task_description.addOut(sizeof(int));
    task_description.addScalar(sizeof(int));
    for(int i = 0; i < nworkers; ++i) {
        fnode_overlap * worker = new fnode_overlap(device, task_description, 1);
        w.push_back(worker->stage());
    }
#endif
    farm.add_workers(w);
    farm.cleanup_workers();
    
    farm.add_collector(new drain());

    farm.run_and_wait_end();

    cout << "Time Spent: " << farm.ffTime() << "ms" << endl;

    return(0);
}