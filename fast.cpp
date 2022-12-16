#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

/* GeneraL Variables */
int num_threads = 8;
unsigned int *buf;
char *buffer;

/* Threads */
pthread_t *threads;

struct thread_data
{
    unsigned int thread_id;
    unsigned int size;
    unsigned int xor_result;
};

/* Functions */
double now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void perror(string s)
{
    cout << "Error: " << s << endl;
    exit(0);
}

void print_performance(double start, double end, unsigned int final_xor)
{
    cout << "======================= Metrics ======================" << endl;
    cout << "Time taken: " << (end - start) << " seconds" << endl;
    printf("Xor value: %08x", final_xor);
    cout << endl
         << "======================================================" << endl;
}

void *xorbuf(void *arg)
{
    struct thread_data *args;
    args = (struct thread_data *)arg;

    long tid = args->thread_id;
    long size = args->size;
    unsigned int result = 0;

    for (int i = tid; i < size; i += num_threads)
    {
        // if(buf[i]!=0) cout<<buf[i]<<" thread "<<tid<<" "<<i<<endl;
        result ^= buf[i];
    }

    args->xor_result = result;

    pthread_exit(NULL);
}

unsigned int multithreaded_xor(unsigned int no_of_elements, struct thread_data td[])
{
    unsigned int final_xor = 0;

    for (int i = 0; i < num_threads; i++)
    {
        td[i].size = no_of_elements;
        td[i].thread_id = i;
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
    unsigned int block_size = 16777216, final_xor = 0;
    bool read_mode = false, write_mode = false;
    double start, end;
    string file_name = "";
    struct thread_data td[num_threads];

    if (argc != 2)
    {
        perror("Please check your arguments.");
        return 0;
    }
    else
    {
        file_name = argv[1];
    }

    srandom(time(NULL));

    unsigned int no_of_elements = (unsigned int)(block_size / sizeof(int) + block_size % sizeof(int));
    unsigned int size_of_buf = no_of_elements * sizeof(int);
    buf = (unsigned int *)malloc(size_of_buf);
    // cout<<no_of_elements<<" ----- "<<size<<" ----- "<<sizeof(buf)<<"-----"<<(no_of_elements * sizeof( unsigned int))<<endl;
    // memset(buf,0,no_of_elements*sizeof(int));

    start = now();

    ifstream object;
    object.open(file_name, ios::binary);

    if (object.fail())
    {
        cout << "Issue reading file " << file_name << endl;
    }
    else
    {
        threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

        if (!threads)
        {
            perror("Out of memory.");
        }

        while (object.read((char *)buf, size_of_buf))
        {
            final_xor ^= multithreaded_xor(no_of_elements, td);
        }

        if (object.gcount() < block_size && object.gcount() > 0)
        {
            final_xor ^= multithreaded_xor(object.gcount() / sizeof(unsigned int), td);
        }

        // cout<<"----"<<object.gcount()<<endl;

        object.close();

        end = now();

        print_performance(start, end, final_xor);
    }

    return 0;
}