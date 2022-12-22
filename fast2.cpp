#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

int num_threads = 4;
unsigned int *buf;
pthread_t *threads;

struct thread_data
{
    unsigned int start;
    unsigned int end;
    unsigned int xor_result;
};

double now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

double get_rate(double size, double start, double end)
{
    return size / ((end - start) * 1024 * 1024);
}

void print_error(string s)
{
    cout << "Error! " << s << endl;
    exit(EXIT_FAILURE);
}
void *xorbuf(void *arg)
{
    struct thread_data *args;
    args = (struct thread_data *)arg;
    long start = args->start;
    long end = args->end;

    unsigned int result = 0;
    for (int i = start; i <= end; i++)
    {
        // if(buf[i]!=0) cout<<buf[i]<<" thread "<<tid<<" "<<i<<endl;
        result ^= buf[i];
    }
    args->xor_result = result;
    pthread_exit(NULL);
    return NULL;
}

unsigned int multithreaded_xor(unsigned int no_of_elements, struct thread_data td[], int indices[])
{
    unsigned int final_xor = 0;
    for (int i = 0; i < num_threads; i++)
    {
        td[i].start = indices[i];
        td[i].end = indices[i+1];
        pthread_create(&threads[i], NULL, xorbuf, (void *)&td[i]);
    }
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    for (int i = 0; i < num_threads; i++)
    {
        final_xor = final_xor ^ td[i].xor_result;
    }
    return final_xor;
}

int main(int argc, char *argv[])
{
    unsigned int block_size = 16777216, final_xor = 0;  //16MB of buffer
    double start, end;
    string file_name = "";
    struct thread_data td[num_threads];

    if (argc != 2)
        print_error("Check arguments!");

    else
    {
        file_name = argv[1];
    }

    srand(time(NULL));

    int indices[num_threads+1];
    
    unsigned int no_of_elements = (unsigned int)((block_size + sizeof(int) - 1) / sizeof(int));
    for (int i = 0; i < num_threads+1; i++)
    {
        indices[i] = (no_of_elements/num_threads)*i;
        cout<<indices[i]<<endl;
    }
    unsigned int size_of_buf = no_of_elements * sizeof(int);
    buf = (unsigned int *)malloc(size_of_buf);
    // memset(buf,0,no_of_elements*sizeof(int));
    start = now();
    ifstream object;
    object.open(file_name, ios::binary);
    if (object.fail())
        print_error("Cannot read file!");
    else
    {
        threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
        if (!threads)
            perror("out of memory for threads!");

        while (object.read((char *)buf, size_of_buf))
        {
            final_xor ^= multithreaded_xor(no_of_elements, td, indices);
        }
        if (object.gcount() < block_size && object.gcount() > 0)
        {
            final_xor ^= multithreaded_xor(object.gcount() / sizeof(unsigned int), td, indices);
        }
        end = now();
        cout << "Time taken: " << (end - start) << " seconds" << endl << 
                "Block size: " << block_size << " bytes" << endl;
        printf("Xor value is %08x", final_xor);
    }

    cout << "\n";
    return 0;
}