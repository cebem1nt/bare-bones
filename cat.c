#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 256

void
err_exit(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

int 
main(int argc, char** argv) 
{
    if (argc != 2) {
        printf("Usage: cat <file>\n");
        return 1;
    }

    int  fd, n;
    char buf[BUF_SIZE];

    if ((fd = open(argv[1], O_RDONLY)) == -1)
        err_exit("cat");

    while ((n = read(fd, buf, BUF_SIZE)) != 0) {
        if (n == -1)
            err_exit("read");
        
        if (write(STDOUT_FILENO, buf, n) == -1)
            err_exit("write");
    }
    
    putchar('\n');
    return 0;
}