/*
 * Sort of shell >> function
 * appends stdin to a file
 */

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE 256

void
clean_exit(char* fn_name) 
{
    perror(fn_name);
    exit(EXIT_FAILURE);
}

int 
main(int argc, char** argv) 
{
    if (argc > 2) {
        printf("Usage: append < <input_file> <output>"); // Well, use it that way huh
        return 1;
    }

    int  out_fd;
    int  out_flags = O_RDWR | O_APPEND | O_CREAT;
    int  mode = 0644;
    char buf[BUFFER_SIZE];

    if ((out_fd = open(argv[1], out_flags, mode)) == -1)
        clean_exit("open out");
    
    int  n;

    while ( (n = read(STDIN_FILENO, buf, BUFFER_SIZE)) ) {
        if (n == -1)
            clean_exit("read");
        
        if (write(out_fd, buf, n) == -1)
            clean_exit("write");
    }

    return 0;
}