// Workers
#include <iostream>
#include <queue>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

int main(int argc, char *argv[])
{
    // na xrisimopoiw to pid kathe diergasias paidi/worker ws onoma tou named_pipe
    string p_dir = "named_pipes/";
    if(mkfifo((p_dir + "bla").c_str(), 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("receiver: mkfifo");
            exit(6);
        }
    }

    return 0;
}