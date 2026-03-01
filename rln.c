/*
 * Picks random line from a file
 */

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>
#include <unistd.h>

#define MAX_LINE 4096

void 
abandon(const char* reason) 
{
    perror(reason);
    exit(EXIT_FAILURE);
} 

long 
get_random_line(long total_lines) 
{
    size_t random; 
    getrandom(&random, sizeof(random), 0);
    return random % (total_lines + 1);
}

int 
main(int argc, char** argv) 
{

    if (argc != 2) {
        printf("usage: %s <file>", argv[0]);
        return 1;
    }

    char  buf[MAX_LINE];
    int   fd, n;
    off_t fsize;
    long  lines = 0;

    if ((fd = open(argv[1], O_RDONLY)) == -1)
        abandon("open");

    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    while (read(fd, buf, 1) > 0) {
        if (*buf == '\n')
            lines++;
    }

    long picked = get_random_line(lines);
    long current_line = 1;

    lseek(fd, 0, SEEK_SET);

    while (read(fd, buf, 1) > 0) {
        if (current_line > picked)
            break;

        if (current_line == picked)
            write(STDOUT_FILENO, buf, 1);

        if (*buf == '\n')
            current_line++;
    }

    return 0;
}