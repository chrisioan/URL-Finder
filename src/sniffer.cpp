//  Manager/Sniffer
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

#define NP_DIR "../../named_pipes/"             /* NamedPipes' directory */

pid_t pid_l;                                    /* Listener's PID */
int p[2];                                       /* Pipe */

void cleanup(int signum)
{
    if (signum == SIGINT)                       /* SIGINT signal */
    {
        if (p[0] > 1)
            close(p[0]);                        /* Close Pipe's READ */
        if (p[1] > 1)
            close(p[1]);                        /* Close Pipe's WRITE */

        kill(pid_l, SIGKILL);                   /* Send SIGKILL to Listener - Terminate it */

        kill(0, SIGCONT);                       /* Send SIGCONT signal to every process in the process group of
                                                    Manager/Sniffer so that "stopped" processes can get the signal SIGINT */
        kill(0, SIGINT);                        /* Send SIGINT signal so that every process
                                                    can catch it and make their own cleanup */
        raise(SIGKILL);                         /* Terminate Manager/Sniffer */
    }
}

using namespace std;

int main(int argc, char *argv[])
{
    static struct sigaction act;
    act.sa_handler = cleanup;
    act.sa_flags = SA_RESTART;                  /* read() shall restart instead of fail if signal (SIGCHLD) */
    sigfillset(&(act.sa_mask));                     /* is given to manager by worker */
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    if ((argc != 1) && (argc != 3))             /* Check if program is executed correctly */
    {
        printf("Usage: ./sniffer [-p path]\n");
        exit(1);
    }

    string path = ".";                          /* Default path is current directory */

    if (argc == 3)                              /* Optional parameter given */
    {
        if (strcmp(argv[1], "-p") != 0)
        {
            printf("Usage: ./sniffer [-p path]\n");
            exit(1);
        }
        path = argv[2];                         /* Update path */
    }

    if (path.back() != '/')                     /* '/' must be the last char */
        path += "/";

    if (pipe(p) == -1)                          /* Create a pipe so that Sniffer / Manager */
    {                                               /* can communicate with Listener */
        perror("pipe call");                    /* pipe() failed */
        exit(2);
    }

    switch (pid_l = fork())                     /* Create child process for listener */
    {
    case -1:                                    /* fork() failed */
        perror("fork call");
        exit(3);
    case 0:                                     /* Fork succeeded - LISTENER */
        close(p[0]);                            /* LISTENER is writing */
        if (dup2(p[1], STDOUT_FILENO) == -1)    /* Link LISTENER's stdout with the pipe */
        {
            perror("dup2 call");                /* dup2() failed */
            exit(4);
        }
        /* Use 'inotifywait' to monitor the changes in files stored in a directory under 'path' */
        if (execlp("inotifywait", "inotifywait", path.c_str(), "-m", "-e", "create", "-e", "moved_to", NULL) == -1)
        {
            perror("execlp call");              /* execlp() failed */
            exit(5);
        }
    default:                                    /* Parent process - SNIFFER / MANAGER */
        close(p[1]);                            /* Parent is reading */
    }

    pid_t pid;
    string np_name;
    queue<pid_t> workers_queue;
    int _size = 0, fd_np;
    char input;                                 /* To read byte-byte */
    vector<char> mybuffer;

    while (1)
    {
        mybuffer.clear();                       /* Clear buffer */
        string filename = "";
        while ((_size = read(p[0], &input, 1)) > 0)
        {
            if (input == '\n')                  /* Found newline */
            {
                for (int i = 0; i < int(mybuffer.size()); i++)
                {
                    /* Enough space for filename */
                    if((mybuffer.size() - i) > 6)
                        /* Starts with CREATE */
                        if(mybuffer.at(i) == 'C' && mybuffer.at(i+1) == 'R' && mybuffer.at(i+2) == 'E' && mybuffer.at(i+3) == 'A' && mybuffer.at(i+4) == 'T' && mybuffer.at(i+5) == 'E')
                            /* Iterate to extract filename */
                            for (int w = i + 7; w < int(mybuffer.size()); w++)
                                filename += mybuffer.at(w);
                    /* Enough space for filename */
                    if((mybuffer.size() - i) > 8)
                        /* Starts with MOVED_TO */
                        if(mybuffer.at(i) == 'M' && mybuffer.at(i+1) == 'O' && mybuffer.at(i+2) == 'V' && mybuffer.at(i+3) == 'E' && mybuffer.at(i+4) == 'D' && mybuffer.at(i+5) == '_' && mybuffer.at(i+6) == 'T' && mybuffer.at(i+7) == 'O') 
                            /* Iterate to extract filename */
                            for (int w = i + 9; w < int(mybuffer.size()); w++)
                                filename += mybuffer.at(w);
                }
                mybuffer.clear();               /* Clear buffer */
                break;
            }
            else                                /* Still on the same line */
                mybuffer.push_back(input);      /* Add byte/char to buffer */
        }
        if (_size < 0)
        {
            perror("Error in Reading");         /* read() failed */
            exit(6);
        }
        filename += "\n";

        /* Catch all workers in status "stopped" */
        while ((pid = waitpid(-1, NULL, WNOHANG | WUNTRACED)) > 0)
            workers_queue.push(pid);            /* add worker to queue */

        if (!workers_queue.empty())             /* There is an available worker */
        {
            pid = workers_queue.front();        /* Get worker's pid from queue */
            workers_queue.pop();                /* Remove it from available workers */
            np_name = NP_DIR + to_string(pid);  /* Assign Named Pipe name */
            /* Open the Named Pipe for read & write */
            if ((fd_np = open(np_name.c_str(), O_RDWR)) < 0)
            {
                perror("fifo open error");      /* open() failed */
                exit(8);
            }
            /* Write the filename into the Named Pipe */
            if ((_size = write(fd_np, filename.c_str(), filename.length())) == -1)
            {
                perror("Error in Writing");     /* write() failed */
                exit(7);
            }
            if (kill(pid, SIGCONT) == -1)       /* Continue Worker with given PID */
            {
                perror("kill call");            /* kill() failed */
                exit(10);
            }
        }
        else                                    /* All workers are occupied */
        {
            switch (pid = fork())               /* Create new Worker process */
            {
            case -1:                            /* fork() failed */
                perror("fork call");
                exit(3);
            case 0:                             /* Fork succeeded - New WORKER created */
                /* Initialize Named Pipe name */
                np_name = NP_DIR + to_string(getpid());
                /* Create named pipe called after the PID */
                if (mkfifo(np_name.c_str(), 0666) == -1)
                {
                    if (errno != EEXIST)        /* mkfifo() failed */
                    {
                        /* errno not equal to file exists */
                        perror("receiver: mkfifo");
                        exit(9);
                    }
                }
                /* Open the Named Pipe for read & write */
                if ((fd_np = open(np_name.c_str(), O_RDWR)) < 0)
                {
                    perror("fifo open error");  /* open() failed */
                    exit(8);
                }
                /* Write the filename into the Named Pipe */
                if ((_size = write(fd_np, filename.c_str(), filename.length())) == -1)
                {
                    perror("Error in Writing"); /* write() failed */
                    exit(7);
                }
                /* execlp() failed */
                if (execlp("./workers", path.c_str(), NULL) == -1)
                {
                    perror("execlp call");      /* execlp() failed */
                    exit(5);
                }
            default:                            /* Parent process - SNIFFER / MANAGER */
                break;
            }
        }
    }

    return 0;
}