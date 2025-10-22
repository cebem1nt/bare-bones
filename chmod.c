#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

void
err_exit(const char* reason) 
{
    perror(reason);
    exit(EXIT_FAILURE);
}

int
main(int argc, char** argv) 
{
    if (argc != 3) {
        fprintf(stderr, "Usage: chmod <mode> <file>\n");
        return 1;
    }

    char*  file = argv[2];
    mode_t mode;

    if (sscanf(argv[1], "%o", &mode) < 0) {
        fprintf(stderr, "Incorrect filemode input %s\n", argv[3]);
        return 1;
    }

    if (chmod(file, mode) == -1)
        err_exit("chmod");

    return 0;
}