#include <stdio.h>
#include <sys/stat.h>

int
main(int argc, char** argv) 
{
    if (argc != 2) {
        printf("%04o\n", umask(0));
        return 0;
    }

    // The umask is a shell command as well as cd. 
    // So this one won't change anything in the shell
    mode_t mask;
    
    if (sscanf(argv[1], "%o", &mask) == -1) {
        fprintf(stderr, "umask nvalid mask %s\n", argv[1]);
        return 1;
    }

    umask(mask);
    return 0;
}