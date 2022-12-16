#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

int num_threads = 8;
char *buffer;
unsigned int *buf;
pthread_t *threads;

struct thread_data
{
    unsigned int thread_id;
    unsigned int size;
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

void perror(string s)
{
    cout << "Error! " << s << endl;
    exit(0);
}

void print_performance(double size, double start, double end, unsigned int block_count, unsigned int final_xor)
{
    cout << "Number of blocks read: " << block_count << " blocks" << endl;
    cout << "Size of the file read: " << (size / (1024 * 1024)) << " MB" << endl;
    cout << "Time taken: " << (end - start) << " seconds" << endl;
    cout << "Rate at which file was read: " << get_rate(size, start, end) << "MiB/sec" << endl;
    printf("Xor value is %08x", final_xor);
}

void print_performance_w(double size, double start, double end)
{
    cout << "Size of the file that was writte: " << (size / (1024 * 1024)) << " MB" << endl;
    cout << "Time taken: " << (end - start) << " seconds" << endl;
    cout << "Rate at which file was written: " << get_rate(size, start, end) << "MiB/sec" << endl;
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
    cout << "RUN" << endl;

    unsigned int block_size = 0, block_count = 0, size = 0;
    bool read_mode = false, write_mode = false;
    double start, end;
    string file_name = "";
    struct thread_data td[num_threads];

    if (argc != 5)
    {
        perror("Too few arguments!");
    }
    else
    {
        string s = argv[2];
        file_name = argv[1];
        read_mode = ("-r" == s || "-R" == s);
        write_mode = ("-w" == s || "-W" == s);
        block_size = (unsigned int)stoi(argv[3]);
        block_count = (unsigned int)stoi(argv[4]);
        // cout<<"---"<<block_size<<"--"<<block_count<<endl;
    }

    srandom(time(NULL));
    size = block_count * block_size;

    if (read_mode)
    {
        unsigned int no_of_blocks_elapsed = 0, final_xor = 0, size_of_buf;
        unsigned int no_of_elements = (unsigned int)(block_size / sizeof(int) + block_size % sizeof(int));
        size_of_buf = no_of_elements * sizeof(int);
        buf = (unsigned int *)malloc(size_of_buf);
        // cout<<no_of_elements<<" ----- "<<size<<" ----- "<<sizeof(buf)<<"-----"<<(no_of_elements * sizeof( unsigned int))<<endl;
        // memset(buf,0,no_of_elements*sizeof(int));

        start = now();
        ifstream object;
        object.open(file_name, ios::binary);

        if (object.fail())
        {
            cout << "Can't read file " << file_name;
        }
        else
        {
            cout << "Reading " << file_name << " in chunks of " << block_size << " ..... " << endl;
            threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

            if (!threads)
            {
                perror("out of memory for threads!");
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

            // cout<<"----"<<object.gcount()<<endl;
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

        end = now();

        print_performance_w(size, start, end);
    }
    else
    {
        perror("Please check your arguments and specify if its -r/-w");
    }

    cout << "\n";

    return 0;
}