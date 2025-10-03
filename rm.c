#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

void 
rm(char* path) 
{
    if (remove(path) == -1) {
        perror("rm");
        exit(EXIT_FAILURE);
    }
}

int
main(int argc, char** argv) 
{
    if (argc < 2) {
        fprintf(stderr, "Arguments were expected, but got nothing.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
       rm(argv[i]);
    }

    return 0;
}