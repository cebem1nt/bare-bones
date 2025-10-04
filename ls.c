#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

void 
ls(char* path)
{
    struct dirent* dirp;
    DIR* dp;

    if ( (dp = opendir(path)) == NULL ) {
        perror("ls");
        exit(1);
    }

    while ( (dirp = readdir(dp)) != NULL ) {
        printf("%s\n", dirp->d_name);
    }

    closedir(dp);
}

int 
main(int argc, char** argv) 
{
    if (argc == 1) {
        char* home = getenv("HOME");
        ls(home);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        char* dir = argv[i];

        printf("\n%s:\n", dir); ls(dir);
    }

    return 0;
}