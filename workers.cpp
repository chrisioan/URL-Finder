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

#define NP_DIR "named_pipes/" // NP = NamedPipes

using namespace std;

int main(int argc, char *argv[])
{
    // Map to contain URL Locations with Number of times shown
    map<string, int> locations;
    // Initialize monitored dir's path
    const string path = argv[0];
    // Initialize Named Pipe name
    const string np_name = NP_DIR + to_string(getpid());
    int rsize = 0, rsize2 = 0, fd_np, fd_read, fd_write;
    char read_buf[1000], read_buf2[1000];
    // To read byte-byte
    char input;
    // Keeps 1 line each time
    vector<char> mybuffer;
    vector<char> url;

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
        // Clear Map
        locations.clear();
        string read_file = path;
        string write_file = "out/";
        for (int i = 0; i < rsize; i++)
        {
            if (read_buf[i] != '\n')
            {
                read_file += read_buf[i];
                write_file += read_buf[i];
            }
            else
                write_file += ".out";
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
        // Clear read_buf
        memset(read_buf2, 0, 1000);
        // Clear Vector
        mybuffer.clear();
        // rsize2 = 1 (byte-byte)
        while ((rsize2 = read(fd_read, &input, 1)) > 0)
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
                                if (mybuffer.at(w) == '/' || mybuffer.at(w) == '\0' || mybuffer.at(w) == '\n')
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
                                    if(it != locations.end())
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
        if (rsize2 < 0)
        {
            // read() failed
            perror("Error in Reading");
            exit(6);
        }

        for(auto it = locations.begin(); it != locations.end(); ++it)
        {
            string out = "";
            out = (*it).first;
            out += " ";
            out += (*it).second;
            out += "\n";
            // write
            
            printf("%s", out.c_str());
        }

        // Write everything to .out file
        // fd_write
        // Empty the map
        locations.clear();
        fflush(stdout);
        raise(SIGSTOP);
    }

    return 0;
}