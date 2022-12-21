#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

/* GeneraL Variables */
int num_threads = 4;
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

double get_rate(double size, double start, double end)
{
    return size / ((end - start) * 1024 * 1024);
}

void print_performance(double size, double start, double end, unsigned int block_count, unsigned int final_xor)
{
    cout << "======================= Metrics ======================" << endl;
    cout << "Number of blocks read: " << block_count << " blocks" << endl;
    cout << "Size of the file read: " << (size / (1024 * 1024)) << " MB" << endl
         << endl;
    cout << "Time taken: " << (end - start) << " seconds" << endl;
    cout << "Rate of file read: " << get_rate(size, start, end) << " MiB/sec" << endl
         << endl;
    printf("Xor value: %08x", final_xor);
    cout << endl
         << "======================================================" << endl;
}

void print_performance_w(double size, double start, double end)
{
    cout << "======================= Metrics ======================" << endl;
    cout << "Size of the file written: " << (size / (1024 * 1024)) << " MB" << endl;
    cout << "Time taken: " << (end - start) << " seconds" << endl;
    cout << "Rate of file written: " << get_rate(size, start, end) << " MiB/sec" << endl;
    cout << "======================================================" << endl;
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
    unsigned int block_size = 0, block_count = 0, size = 0;
    bool read_mode = false, write_mode = false;
    double start, end;
    string file_name = "";
    struct thread_data td[num_threads];

    if (argc != 5)
    {
        perror("Please check your arguments.");
        return 0;
    }
    else
    {
        string s = argv[2];
        file_name = argv[1];
        read_mode = ("-r" == s || "-R" == s);
        write_mode = ("-w" == s || "-W" == s);
        block_size = (unsigned int)stoi(argv[3]);
        block_count = (unsigned int)stoi(argv[4]);
    }

    srandom(time(NULL));

    size = block_count * block_size;

    if (read_mode)
    {
        unsigned int no_of_blocks_elapsed = 0, final_xor = 0, size_of_buf;
        unsigned int no_of_elements = (unsigned int)(block_size / sizeof(int) + block_size % sizeof(int));
        size_of_buf = no_of_elements * sizeof(int);
        buf = (unsigned int *)malloc(size_of_buf);

        start = now();

        ifstream object;
        object.open(file_name, ios::binary);

        if (object.fail())
        {
            cout << "Issue reading file " << file_name;
        }
        else
        {
            cout << "Reading " << file_name << " in chunks of " << block_size << " ..... " << endl;

            threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

            if (!threads)
            {
                perror("Out of memory.");
            }

            while (object.read((char *)buf, size_of_buf))
            {
                final_xor ^= multithreaded_xor(no_of_elements, td);

                if (block_size * ++no_of_blocks_elapsed >= size)
                {
                    break;
                }
            }

            if (object.gcount() < block_size && object.gcount() > 0)
            {
                final_xor ^= multithreaded_xor(object.gcount() / sizeof(unsigned int), td);
            }

            object.close();

            end = now();

            print_performance(size, start, end, block_count, final_xor);
        }
    }
    else if (write_mode)
    {
        start = now();

        ofstream object(file_name);
        buffer = new char[size];

        for (unsigned int i = 0; i < block_count; i++)
        {
            object.write(buffer, size);
        }

        object.close();

        end = now();

        print_performance_w(size, start, end);
    }
    else
    {
        perror("Please specify -r or -w in arguments.");
        return 0;
    }

    return 0;
}