// Workers
#include <iostream>
#include <queue>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>

#define NP_DIR "named_pipes/" // NP = NamedPipes

using namespace std;

int main(int argc, char *argv[])
{
    // Initialize monitored dir's path
    const string path = argv[0];
    // Initialize Named Pipe name
    const string np_name = NP_DIR + to_string(getpid());
    int rsize = 0, rsize2 = 0, fd_np, fd_read, fd_write;
    char read_buf[1000], read_buf2[1000];

    // Open the Named Pipe for read & write
    if ((fd_np = open(np_name.c_str(), O_RDWR)) < 0)
    {
        // open() failed
        perror("fifo open error");
        exit(6);
    }

    while (1)
    {
        // Clear read_buf
        memset(read_buf, 0, 1000);
        if ((rsize = read(fd_np, read_buf, 1000)) < 0)
        {
            // read() failed
            perror("Error in Reading");
            exit(6);
        }
        string read_file = path;
        string write_file = "out/";
        for(int i = 0; i < rsize; i++)
        {
            if(read_buf[i] != '\n')
            {
                read_file += read_buf[i];
                write_file += read_buf[i];
            }
            else
                write_file += ".out";
        }
        // Open read file
        if((fd_read = open(read_file.c_str(), O_RDONLY, 0666)) == -1)
        {
            // open() failed
            perror("open call");
            exit(6);

        }
        // Create output file
        if((fd_write = open(write_file.c_str(), O_WRONLY | O_CREAT, 0666)) == -1)
        {
            // open() failed
            if(errno != EEXIST)
            {
                // errno not equal to file exists
                perror("open call");
                exit(6);
            }
        }
        // Clear read_buf
        memset(read_buf2, 0, 1000);
        while((rsize2 = read(fd_read, read_buf2, 1000)) > 0)
        {
            printf("%.*s", rsize2, read_buf2);
        }
        //printf("Message Received: %.*s\n", rsize, read_buf);
        fflush(stdout);
        raise(SIGSTOP);
    }

    return 0;
}