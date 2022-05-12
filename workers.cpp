// Workers
#include <iostream>
#include <vector>
#include <queue>
#include <iterator>
#include <map>

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

#define NP_DIR "named_pipes/"                   /* NamedPipes' directory */
#define OUT_DIR "out/"                          /* output files' directory */
#define OUT_EXT ".out"                          /* output files' extension */

int fd_np, fd_read, fd_write;                   /* File Descriptors */

void cleanup(int signum)
{
    close(fd_np);                               /* Close Named Pipe */
    if (fd_read > 0)
        close(fd_read);                         /* Close Read File */
    if (fd_write > 0)
        close(fd_write);                        /* Close Write File */
    raise(SIGKILL);                             /* Terminate this Worker */
}

using namespace std;

int main(int argc, char *argv[])
{
    static struct sigaction act;
    act.sa_handler = cleanup;
    sigfillset(&(act.sa_mask));
    sigaction(SIGINT, &act, NULL);

    map<string, int> locations;                 /* Map to contain pair<Location, Count> */
    const string path = argv[0];                /* Initialize monitored dir's path */
    const string np_name = NP_DIR + to_string(getpid());    /* Initialize Named Pipe name */
    int rsize = 0;
    char input;                                 /* To read byte-byte */
    vector<char> mybuffer;

    /* Open the Named Pipe for read & write */
    if ((fd_np = open(np_name.c_str(), O_RDWR)) < 0)
    {
        perror("fifo open error");              /* open() failed */
        exit(6);
    }

    while (1)
    {
    	mybuffer.clear();			            /* Clear buffer */
        while ((rsize = read(fd_np, &input, 1)) > 0)
        {
            if (input == '\n')
            {
                mybuffer.push_back(input);
                break;
            }
            mybuffer.push_back(input);
        }
        if (rsize < 0)
        {
            perror("Error in Reading");         /* read() failed */
            exit(6);
        }

        string read_file;
        if(strcmp(path.c_str(), "./") == 0)
            read_file = "";
        else
            read_file = path + "";
        fflush(stdout);
        string write_file = OUT_DIR;
        for (int i = 0; i < int(mybuffer.size()); i++)
        {
            if (mybuffer.at(i) != '\n')
            {
                read_file += mybuffer.at(i);
                write_file += mybuffer.at(i);
            }
            else
                write_file += OUT_EXT;
        }
        /* Open read file */
        if ((fd_read = open(read_file.c_str(), O_RDONLY, 0666)) == -1)
        {
            perror("open call");                /* open() failed */
            exit(6);
        }
        /* Create output file */
        if ((fd_write = open(write_file.c_str(), O_WRONLY | O_CREAT, 0666)) == -1)
        {                                       /* open() failed */
            if (errno != EEXIST)
            {
                perror("open call");            /* errno not equal to file exists */
                exit(6);
            }
        }
        while ((rsize = read(fd_read, &input, 1)) > 0)  /* rsize = 1 (byte-byte) */
        {                                       /* Found new 'word' */
            if (input == '\n' || input == '\0' || input == ' ')
            {
            	mybuffer.push_back(input);	    /* So that we can catch the end */
                if (mybuffer.size() > 7)        /* Enough space for URL */
                {                               /* Starts with http:// */
                    if (mybuffer.at(0) == 'h' && mybuffer.at(1) == 't' && mybuffer.at(2) == 't' && mybuffer.at(3) == 'p' && mybuffer.at(4) == ':' && mybuffer.at(5) == '/' && mybuffer.at(6) == '/')
                    {
                        string loc = "";
                        int start;
                        if(mybuffer.size() > 11)
                            if(mybuffer.at(7) == 'w' && mybuffer.at(8) == 'w' && mybuffer.at(9) == 'w' && mybuffer.at(10) == '.')
                                start = 11;
                            else
                                start = 7;
                        else
                            start = 7;
                        /* Iterate to get the URL's Location */
                        for(int i = start; i < int(mybuffer.size()); i++)
                        {                       /* Found the end of URL's Location */
                            if (mybuffer.at(i) == '/' || mybuffer.at(i) == '\0' || mybuffer.at(i) == '\n' || mybuffer.at(i) == ' ')
                            {
                                auto it = locations.find(loc);  /* Check if URL's Location exists in map */
                                if (it != locations.end())  /* Exists in map */
                                    it->second += 1;    /* Increase Counter */
                                else            /* Doesn't exist in map */
                                    locations.insert(pair<string, int>(loc, 1));    /* Insert and set Counter to 1 */
                                break;          /* Stop iterating */
                            }
                            else
                                loc += mybuffer.at(i);
                        }
                    }
                }
                mybuffer.clear();               /* Clear buffer */
            }
            else                                /* Still on the same 'word' */
                mybuffer.push_back(input);      /* Add byte/char to buffer */
        }
        if (rsize < 0)
        {
            perror("Error in Reading");         /* read() failed */
            exit(6);
        }
        mybuffer.clear();                       /* Clear buffer */
        /* Write everything to .out file */
        for (auto curr = locations.begin(); curr != locations.end(); curr++)
        {
            string out = (*curr).first + " " + to_string((*curr).second) + "\n";
            if (write(fd_write, out.c_str(), out.length()) == -1)
            {
                perror("Error in Writing");     /* write() failed */
                exit(7);
            }
        }
        locations.clear();                      /* Clear the map */
        if ((fd_read = close(fd_read)) == -1)   /* Close Read File */
        {
            perror("Error in Close");           /* close() failed */
            exit(11);
        }
        if ((fd_write = close(fd_write)) == -1) /* Close Write File */
        {
            perror("Error in Close");           /* close() failed */
            exit(11);
        }
        fflush(stdout);
        raise(SIGSTOP);                         /* Worker is now in state "stopped" */
    }

    return 0;
}
