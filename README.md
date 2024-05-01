# URL Finder

## Table of contents
* [About The Project](#about-the-project)
* [Requirements](#requirements)
* [How To Run](#how-to-run)
* [General Notes](#general-notes)
* [Sniffer](#sniffer)
* [Workers](#workers)
* [Finder](#finder)
<br/><br/>

## About The Project
The purpose of this project is to become familiar with the creation of processes using system calls like fork/exec, inter-process communication via pipes and named pipes, the use of low-level I/O, signal handling, and the creation of shell scripts.
<br></br>

For the **first part** of the project which includes **Sniffer** and **Workers**, using `inotifywait`, we will monitor file changes in a directory. The purpose is to open those files and search URLs using low-level I/O. The files are text files that can contain plain text and URLs. The search is limited to URLs using the **http** protocol, specifically in the form of `http://...`. Each URL starts with http://, and ends with a space character.

For each detected URL, the location information must be extracted, excluding "www". Check the following link: https://www.techopedia.com/definition/1352/uniform-resource-locator-url

For example, for the URL of the website of our department http://www.di.uoa.gr/ we have `di.uoa.gr` as the location.

During file reading, the `worker` creates a new file in which it records all detected locations along with their frequency of appearance. For example, if the added file contains **3 URLs** with the location **di.uoa.gr**, the worker's output file will have a line like **di.uoa.gr 3**, and similarly, one line for each other location.

If the file read by the worker is named `<filename>`, then the file it creates is named `<filename>.out`.
<br></br>

For the **second part** of the project, οur goal is to create a shell script, **finder.sh**, that will accept one or more Top Level Domains (TLDs) as arguments and search for these TLDs across all **.out** files. Specifically, the script should determine the total number of occurrences of each TLD across the set of output files we created.

Our files from the previous part are in the following format:
```
    location     num_of_appearances
```
For example, if we give `com` as the argument for the TLD, the result will be the sum of `num_of_appearances` where the locations **end with** `com`.
<br/><br/>

## Requirements
* Make
  ```sh
  sudo apt install make
  ```
* Compiler with support for C++11 or newer
  ```sh
  sudo apt install g++
  ```
* Inotify-tools
  ```sh
  sudo apt install inotify-tools
  ```
<br/><br/>

## How To Run 
When on the root directory of the project:
```sh
make
```
* For **Sniffer**, navigate to **build/release** and execute:
```sh
./sniffer [-p path]
```
Where **-p path** is an optional parameter, used to indicate the path of the directory we want to monitor.

* For **Finder**, navigate to **src** and execute:
```sh
./finder.sh <list_of_tlds>
```
Where **list_of_tlds** is one or more Top Level Domain (TLD) that we want to search in all .out files.

---
* Examples:
```sh
./sniffer -p Work
```
```sh
./sniffer
```
```sh
./finder.sh com
```
```sh
./finder.sh com net org
```
<br/><br/>

## General Notes
1. If the parameter [-p path] is provided, then the path must exist before the program is executed - it is not created.
2. Each named pipe created in the "named_pipes" folder carries the name of the process that created it.
3. All **.out** files are created inside a folder called **out** which will be located in root directory.
<br/><br/>

## Sniffer
Source code can be found here: [sniffer.cpp](https://github.com/chrisioan/URL-Finder/blob/main/src/sniffer.cpp)

At the beginning of the program, a "sigaction" named "act" is created with a handler called "cleanup" and the flag "SA_RESTART", so that when the Manager/Sniffer receives a SIGCHLD signal from a worker, it will unblock from the read call it makes on the communication channel with the Listener, allowing it to refresh its information about available workers and retry the read operation. The signal set is initialized and filled, then the SIGCHLD and SIGINT signals are detected. In the "cleanup" signal handler, a check is made to determine if the received signal is SIGINT so that, before the manager is terminated, it can kill all other processes and perform the necessary cleanup of files, pipes, named pipes, etc. Thus, it closes the read and write ends of the pipe between Manager and Listener (if they are not already closed), sends a SIGKILL signal to the Listener process, and then sends SIGCONT followed by SIGINT to all other processes. Finally, a SIGKILL signal is sent to the Manager itself to terminate. The use of SIGCONT is to "unblock" any Workers that are in a "stopped" state so that they can receive the subsequent SIGINT signal to perform their own cleanup.

Next, the program's execution arguments are checked, and the given path is obtained (otherwise, the current directory is used). A pipe is created for communication between the Manager and the Listener, and the Listener process is created via fork(). The Manager closes the write end since it only reads, while the Listener closes the read end. Additionally, the output of the Listener process is connected to the pipe with the dup2 call, and finally, inotifywait is executed using execlp with the appropriate parameters (the "body" of the Listener process is replaced with that of inotifywait).

Subsequently, there is an infinite loop (while (1)), within which files detected and written to the pipe by the Listener are continuously read and assigned to a worker (if one is available); otherwise, a new worker is created to perform the operation mentioned in the prompt. In another while loop, one character/byte at a time is read from the pipe into "mybuffer," which is a vector of chars. When a newline '\n' is detected, the buffer's contents are traversed one by one to check for "CREATE" or "MOVED_TO" to extract the filename. The buffer is cleared, the (second) while loop is exited, and a '\n' is added at the end of the filename so that we know where it ends.

At this point, workers who have changed status (become "stopped," triggering the SIGCHLD signal to the Manager) are obtained with **waitpid()** and are added to the "workers_queue" (this queue holds the PIDs of Worker processes). Then, it's checked if there are available workers (by checking if the queue is not empty). If yes, the first worker in the queue is popped, as it will be assigned the "filename" obtained earlier. The worker's named pipe (which should have been created earlier) is opened for Read & Write, the filename is written into it, and a SIGCONT signal is sent from the Manager to the Worker to continue/"unblock." If no worker is available, a new one must be created via **fork()**.

In case **fork()** returns 0 (indicating the child/worker process), a new named pipe is created, named after the process ID, opened for Read & Write, the filename is written into the newly created named pipe, and **execlp** is called, providing the monitored path as an argument to **workers.cpp**. The Manager does nothing and returns to the beginning of the while(1) loop.
<br/><br/>

## Workers
Source code can be found here: [workers.cpp](https://github.com/chrisioan/URL-Finder/blob/main/src/workers.cpp)

Just as in the Manager, the Worker has a "sigaction" called "act" and detects the SIGINT signal sent by the Manager, which triggers the necessary cleanup in the signal handler:
1. Close the Named Pipe.
2. Close the Read and Write files if they are open.
3. The Worker process terminates itself by raising a SIGKILL signal.

Initially, the Named Pipe is opened, and its file descriptor is obtained. The same logic as in the Manager follows. Within an infinite loop (while(1)), the entire operation of the Worker takes place. Once again, one character/byte at a time is read and added to the buffer until a newline is detected (hence the addition of '\n' at the end of the "filename" previously). By traversing the buffer's contents, the appropriate strings are created, which hold the name of the file from which we will read and the name of the file that will be created and written with the locations and their occurrence counts. I remind you that I assume the output files will all be written to the "out" folder, which I include in the .tar.gz, so the "write_file" holds something like "out/<filename>.out."

Next, the file is opened for reading, and its file descriptor is obtained. One character at a time is read into the buffer until a newline or a space is found to "split" the file into words. At this point, it's checked if the size of "mybuffer" is greater than 7 (the length of "http://"), indicating a possible URL. This is then verified by checking if it starts with "http://", suggesting it's a URL.

Using the same logic, it's checked if 'www.' is present, which is removed by setting the start of the for loop accordingly (to skip those positions holding those specific characters). The "loc" string is formed to represent the location, as spaces, newlines, and slashes indicate its end. Once the "loc" is finally obtained, it's checked if it exists in the map named "locations." If it does, the counter is increased by one; otherwise, it's added as a new key with a value of 1.

Finally, by traversing the "locations" map, all the necessary information is written to the output file. The map is cleared, the files are closed, and the Worker process sends a SIGSTOP signal to itself to enter a "stopped" state. When it receives a SIGCONT signal from the Manager, it will repeat the entire process within the while(1) loop.
<br/><br/>

## Finder
Source code can be found here: [finder.sh](https://github.com/chrisioan/URL-Finder/blob/main/src/finder.sh)

The number of arguments is stored in a variable called 'argc,' while the names of files in the 'out/' folder are stored in 'files,' obtained by executing the 'ls out' command. Each argument/TLD is traversed individually and formatted (with a period '.' at the beginning and a space ' ' at the end). The 'count' variable is reset to zero at the start of each new TLD search.

In each file (retrieved earlier with 'ls ../out'), the 'match' variable contains the result of the 'grep' command, with [pattern] being the content of the TLD ("$tld") and [file] being the current file to search, with the path from the current directory to it ("../out/$file"). In other words, 'match' holds all the lines from the given file that contain the pattern/TLD.

Finally, the contents of 'match' are traversed, and if any of them is an integer, it is added to the 'count' variable. Checking whether an item is an integer is done using Regular Expression (=~). The '^' indicates the start of the string, '$' indicates the end, and '+' means there can be additional occurrences of the previous pattern. Thus, this approach checks if the string contains only digits (indicating it's an integer).
<br/><br/>
