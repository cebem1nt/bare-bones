#include <stdio.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: ln <file> <link name>");
        return 1;
    }

    int is_symlink = 0;

    if (strcmp(argv[1], "-s") == 0) {
        is_symlink = 1;
    }

    if (is_symlink) {
        if (symlink(argv[2], argv[3]) == -1) {
            perror("ln -s");
            return 1;
        }
    }

    else if (link(argv[1], argv[2]) == -1) {
        perror("ln");
        return 1;
    }

    return 0;
}