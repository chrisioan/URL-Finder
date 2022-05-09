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

#define NP_DIR "named_pipes/" // NamedPipes' directory

// Listener's PID
pid_t pid_l;
// Pipe
int p[2];

void cleanup(int signum)
{
    // SIGINT signal
    if (signum == SIGINT)
    {
        // Close the pipe
        if (p[0] > 1)
            close(p[0]);
        if (p[1] > 1)
            close(p[1]);

        // Send SIGKILL to Listener - Terminate it
        kill(pid_l, SIGKILL);

        // Send SIGCONT signal to every process in the process group of
        // Manager/Sniffer so that "stopped" processes can get the signal SIGINT
        kill(0, SIGCONT);

        // Send SIGINT signal so that every process can catch it and
        // make their own cleanup
        kill(0, SIGINT);

        // Terminate Manager/Sniffer
        raise(SIGKILL);
    }
}

using namespace std;

int main(int argc, char *argv[])
{
    static struct sigaction act;
    // read() shall restart instead of fail if
    // signal (SIGCHLD) is given to manager by worker
    act.sa_handler = cleanup;
    act.sa_flags = SA_RESTART;
    sigfillset(&(act.sa_mask));
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGINT, &act, NULL);

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
        // '/' must be the last char
        if (path.back() != '/')
            path += "/";
    }

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

    while (1)
    {
        // Clear read_buf
        memset(read_buf, 0, 1000);
        if ((rsize = read(p[0], read_buf, 1000)) < 0)
        {
            perror("Error in Reading");
            exit(6);
        }
        char *filename = strtok(read_buf, " ");
        filename = strtok(NULL, " ");
        filename = strtok(NULL, " ");

        // Catch all workers in status "stopped"
        while ((pid = waitpid(-1, NULL, WNOHANG | WUNTRACED)) > 0)
            // add worker to queue
            workers_queue.push(pid);

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

    return 0;
}