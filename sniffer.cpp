// Manager
#include <iostream>
#include <queue>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

using namespace std;

int main(int argc, char *argv[])
{
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

    // Check if program is executed correctly
    if ((argc != 1) && (argc != 3))
    {
        printf("Usage: ./sniffer [-p path]\n");
        exit(1);
    }

    int p[2];
    pid_t pid;

    // Create a pipe so that Sniffer / Manager
    // can communicate with Listener
    if (pipe(p) == -1)
    {
        // pipe() failed
        perror("pipe call");
        exit(2);
    }

    // Create child process for listener
    switch (pid = fork())
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
        if ((execlp("inotifywait", "inotifywait", path.c_str(), "-m", "-e", "create", "-e", "moved_to", NULL)) == -1)
        {
            // execlp() failed
            perror("execlp call");
            exit(5);
        }
        break;
    // Parent process - SNIFFER / MANAGER
    default:
        // Parent is reading
        close(p[1]);
    }

    

    return 0;
}