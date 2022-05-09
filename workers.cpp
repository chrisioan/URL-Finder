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

#define NP_DIR "named_pipes/" // NamedPipes' directory
#define OUT_DIR "out/"        // output files' directory
#define OUT_EXT ".out"        // output files' extension

// File Descriptors
int fd_np, fd_read, fd_write;

void cleanup(int signum)
{
    // Close Named Pipe
    close(fd_np);
    // Close Read File
    if (fd_read > 0)
        close(fd_read);
    // Close Write File
    if (fd_write > 0)
        close(fd_write);
    // Terminate this Worker
    raise(SIGKILL);
}

using namespace std;

int main(int argc, char *argv[])
{
    static struct sigaction act;

    act.sa_handler = cleanup;
    sigfillset(&(act.sa_mask));
    sigaction(SIGINT, &act, NULL);

    // Map to contain pair<Location, Count>
    map<string, int> locations;
    // Initialize monitored dir's path
    const string path = argv[0];
    // Initialize Named Pipe name
    const string np_name = NP_DIR + to_string(getpid());
    int rsize = 0;
    // To read byte-byte
    char input;
    vector<char> mybuffer, url;

    // Open the Named Pipe for read & write
    if ((fd_np = open(np_name.c_str(), O_RDWR)) < 0)
    {
        // open() failed
        perror("fifo open error");
        exit(6);
    }

    while (1)
    {
        while ((rsize = read(fd_np, &input, 1)) > 0)
        {
            if (input == '\0')
                break;
            mybuffer.push_back(input);
        }
        if (rsize < 0)
        {
            // read() failed
            perror("Error in Reading");
            exit(6);
        }

        string read_file = path;
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
        // Open read file
        if ((fd_read = open(read_file.c_str(), O_RDONLY, 0666)) == -1)
        {
            // open() failed
            perror("open call");
            exit(6);
        }
        // Create output file
        if ((fd_write = open(write_file.c_str(), O_WRONLY | O_CREAT, 0666)) == -1)
        {
            // open() failed
            if (errno != EEXIST)
            {
                // errno not equal to file exists
                perror("open call");
                exit(6);
            }
        }
        // Clear Buffer (Vector)
        mybuffer.clear();
        // rsize = 1 (byte-byte)
        while ((rsize = read(fd_read, &input, 1)) > 0)
        {
            // Found newline
            if (input == '\n')
            {
                for (int i = 0; i < int(mybuffer.size()); i++)
                {
                    // Enough room for URL
                    if ((mybuffer.size() - i) > 7)
                    {
                        // Starts with http://
                        if (mybuffer.at(i) == 'h' && mybuffer.at(i + 1) == 't' && mybuffer.at(i + 2) == 't' && mybuffer.at(i + 3) == 'p' && mybuffer.at(i + 4) == ':' && mybuffer.at(i + 5) == '/' && mybuffer.at(i + 6) == '/')
                        {
                            // Iterate to extract the URL's Location
                            for (int w = i + 7; w < int(mybuffer.size()); w++)
                            {
                                // Found the end of URL's Location
                                if (mybuffer.at(w) == '/' || mybuffer.at(w) == '\0' || mybuffer.at(w) == '\n' || mybuffer.at(w) == ' ')
                                {
                                    string loc = "";
                                    // Iterate URL's Location to find 'www.' (if exists)
                                    for (int j = 0; j < int(url.size()); j++)
                                    {
                                        // Enough room for URL
                                        if ((url.size() - j) > 4)
                                        {
                                            // Starts with www.
                                            if (url.at(j) == 'w' && url.at(j + 1) == 'w' && url.at(j + 2) == 'w' && url.at(j + 3) == '.')
                                                // Remove www. from URL's Location
                                                url.erase(url.begin() + j, url.begin() + j + 3);
                                            else
                                                loc += url.at(j);
                                        }
                                        else
                                            loc += url.at(j);
                                    }
                                    // Check if URL's Location exists in map
                                    auto it = locations.find(loc);
                                    // Exists in map
                                    if (it != locations.end())
                                        // Increase Counter
                                        it->second += 1;
                                    // Doesn't exist in map
                                    else
                                        // Insert and set Counter to 1
                                        locations.insert(pair<string, int>(loc, 1));
                                    // Empty the URL buffer
                                    url.clear();
                                    break;
                                }
                                // Didn't find end yet - insert char
                                else
                                    url.push_back(mybuffer.at(w));
                            }
                            continue;
                        }
                        // Doesn't start with http://
                        else
                            continue;
                    }
                    // Not enough room for URL
                    else
                        break;
                }
                // Empty the buffer
                mybuffer.clear();
            }
            // Still on the same line
            else
            {
                // Add byte/char to buffer
                mybuffer.push_back(input);
            }
        }
        if (rsize < 0)
        {
            // read() failed
            perror("Error in Reading");
            exit(6);
        }
        // Write everything to .out file
        for (auto curr = locations.begin(); curr != locations.end(); curr++)
        {
            string out = (*curr).first + " " + to_string((*curr).second) + "\n";
            // write
            if (write(fd_write, out.c_str(), out.length()) == -1)
            {
                // write() failed
                perror("Error in Writing");
                exit(7);
            }
        }
        // Empty the map
        locations.clear();
        // Close Read File
        if ((fd_read = close(fd_read)) == -1)
        {
            // close() failed
            perror("Error in Close");
            exit(11);
        }
        // Close Write File
        if ((fd_write = close(fd_write)) == -1)
        {
            // close() failed
            perror("Error in Close");
            exit(11);
        }
        fflush(stdout);
        // Worker is now in state "stopped"
        raise(SIGSTOP);
    }

    return 0;
}