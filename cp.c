#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 256

void 
clean_exit(int fd1, int fd2) {
    perror("cp");
    
    close(fd1);
    close(fd2);

    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv) 
{
    if (argc < 3 || argc > 3) {
        fprintf(stderr, "Usage: cp <src> <dest>");
        return 1;
    }

    int  input_fd, output_fd;
    char buf[BUFFER_SIZE];
    int  n;

    if ( (input_fd = open(argv[1], O_RDONLY)) == -1 ) {
        perror("cp");
        return 1;
    }

    if ( (output_fd = open(argv[2], O_WRONLY | O_CREAT, 0644)) == -1 ) {
        perror("cp");
        close(input_fd);
        return 1;
    }

    while ( (n = read(input_fd, buf, BUFFER_SIZE)) ) {
        if (n == -1)
            clean_exit(input_fd, output_fd);
        
        if (write(output_fd, buf, n) == -1)
            clean_exit(input_fd, output_fd);
    }

    return 0;
}