// Manager
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

// volatile sig_atomic_t sigint_flag = 0;
// int sigint_flag = 0;
//  int flag = 0;

void sigchld_handler(int signum)
{
    // printf("\nCatching: signum=%d\n", signum);
    // printf("Catching: returning\n");
    // flag = 1;
}

void sigint_handler(int signum)
{
    // leaks
    /* if (kill(pid, SIGKILL) == -1)
    {
        // kill() failed
        perror("kill call");
        exit(10);
    } */
    // sigint_flag = 1;
}

using namespace std;

int main(int argc, char *argv[])
{
    // Check if program is executed correctly
    if ((argc != 1) && (argc != 3))
    {
        printf("Usage: ./sniffer [-p path]\n");
        exit(1);
    }

    // Default path is current directory
    string path = ".";

    // Optional parameter given
    if (argc == 3)
    {
        if (strcmp(argv[1], "-p") != 0)
        {
            printf("Usage: ./sniffer [-p path]\n");
            exit(1);
        }
        // Update path
        path = argv[2];
    }

    int p[2];
    pid_t pid_l;

    // Create a pipe so that Sniffer / Manager
    // can communicate with Listener
    if (pipe(p) == -1)
    {
        // pipe() failed
        perror("pipe call");
        exit(2);
    }

    // Create child process for listener
    switch (pid_l = fork())
    {
    // fork() failed
    case -1:
        perror("fork call");
        exit(3);
    // Fork succeeded - LISTENER
    case 0:
        // LISTENER is writing
        close(p[0]);
        // Link LISTENER's stdout with the pipe
        if (dup2(p[1], STDOUT_FILENO) == -1)
        {
            // dup2() failed
            perror("dup2 call");
            exit(4);
        }
        // Use 'inotifywait' to monitor the changes in files
        // stored in a directory under 'path'
        if (execlp("inotifywait", "inotifywait", path.c_str(), "-m", "-e", "create", "-e", "moved_to", NULL) == -1)
        {
            // execlp() failed
            perror("execlp call");
            exit(5);
        }
    // Parent process - SNIFFER / MANAGER
    default:
        // Parent is reading
        close(p[1]);
    }

    pid_t pid;
    string np_name;
    queue<pid_t> workers_queue;
    int rsize = 0, wsize = 0, fd;
    char read_buf[1000], write_buf[1000];

    static struct sigaction act, act2;
    act.sa_handler = sigchld_handler;
    // read() shall restart instead of fail if
    // signal (SIGCHLD) is given to manager by worker
    act.sa_flags = SA_RESTART;
    sigfillset(&(act.sa_mask));
    sigaction(SIGCHLD, &act, NULL);

    act2.sa_handler = sigint_handler;
    sigfillset(&(act2.sa_mask));
    sigaction(SIGINT, &act2, NULL);

    while (1)
    {
        // Clear read_buf
        memset(read_buf, 0, 1000);
        // sleep(5);
        if ((rsize = read(p[0], read_buf, 1000)) < 0)
        {
            perror("Error in Reading");
            exit(6);
        }
        // strtok_r instead?
        char *filename = strtok(read_buf, " ");
        filename = strtok(NULL, " ");
        filename = strtok(NULL, " ");

        // Catch all workers in status "stopped"
        while ((pid = waitpid(-1, NULL, WNOHANG | WUNTRACED)) > 0)
        {
            // add worker to queue
            workers_queue.push(pid);
            // flag=0;
        }

        // There is an available worker
        if (!workers_queue.empty())
        {
            // Get worker's pid from queue
            pid = workers_queue.front();
            // Remove it from available workers
            workers_queue.pop();
            // Assign Named Pipe name
            np_name = NP_DIR + to_string(pid);
            // Open the Named Pipe for read & write
            if ((fd = open(np_name.c_str(), O_RDWR)) < 0)
            {
                // open() failed
                perror("fifo open error");
                exit(8);
            }
            // Clear write_buf
            memset(write_buf, 0, 1000);
            // Copy the filename to write_buf
            strcpy(write_buf, filename);
            if ((wsize = write(fd, write_buf, 1000)) == -1)
            {
                // write() failed
                perror("Error in Writing");
                exit(7);
            }
            // Continue Worker with given PID
            if (kill(pid, SIGCONT) == -1)
            {
                // kill() failed
                perror("kill call");
                exit(10);
            }
        }
        // All workers are occupied
        else
        {
            // Create new Worker process
            switch (pid = fork())
            {
            // fork() failed
            case -1:
                perror("fork call");
                exit(3);
            // Fork succeeded - New WORKER created
            case 0:
                // Initialize Named Pipe name
                np_name = NP_DIR + to_string(getpid());
                // Create named pipe called after the PID
                if (mkfifo(np_name.c_str(), 0666) == -1)
                {
                    // mkfifo() failed
                    if (errno != EEXIST)
                    {
                        // errno not equal to file exists
                        perror("receiver: mkfifo");
                        exit(9);
                    }
                }
                // Open the Named Pipe for read & write
                if ((fd = open(np_name.c_str(), O_RDWR)) < 0)
                {
                    // open() failed
                    perror("fifo open error");
                    exit(8);
                }
                // Clear write_buf
                memset(write_buf, 0, 1000);
                // Copy the filename to write_buf
                strcpy(write_buf, filename);
                if ((wsize = write(fd, write_buf, 1000)) == -1)
                {
                    // write() failed
                    perror("Error in Writing");
                    exit(7);
                }
                // execlp() failed
                if (execlp("./workers", path.c_str(), NULL) == -1)
                {
                    // execlp() failed
                    perror("execlp call");
                    exit(5);
                }
            // Parent process - SNIFFER / MANAGER
            default:
                break;
            }
        }
    }
    waitpid(pid_l, NULL, 0);

    return 0;
}