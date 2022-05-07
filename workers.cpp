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
    int rsize = 0, fd;
    char read_buf[1000];
    // Initialize Named Pipe name
    const string np_name = NP_DIR + to_string(getpid());

    // Open the Named Pipe for read & write
    if ((fd = open(np_name.c_str(), O_RDWR)) < 0)
    {
        // open() failed
        perror("fifo open error");
        exit(6);
    }

    while (1)
    {
        // Clear read_buf
        memset(read_buf, 0, 1000);
        if ((rsize = read(fd, read_buf, 1000)) < 0)
        {
            // read() failed
            perror("Error in Reading");
            exit(6);
        }
        printf("Message Received: %.*s\n", rsize, read_buf);
        fflush(stdout);
        raise(SIGSTOP);
    }

    return 0;
}