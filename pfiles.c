/*
 * I assume that linux kernel literally maps opened file descriptors to
 * the /proc/.../fd/ . So when you use fd = open(file_name ...) the kernel
 * actually makes a symbolic link to file_name in the directory /proc/<pid>/fd/ !
 * So what we just have to readlink of each file based on given pid
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FD_LENGTH 256
#define MAX_FILE_PATH 1024

size_t
calc_size(char* pid) 
{
    return strlen("/proc/") + strlen("/fd/") + strlen(pid) + 1;
}

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
        printf("Usage: pfiles <pid>");
        return 1;
    }

    char*  pid = argv[1];
    size_t proc_path_size = calc_size(pid);
    char*  proc_path = malloc(proc_path_size);

    if (proc_path == NULL) {
        fprintf(stderr, "To big pid isn't it?\n");
        return 1;
    }

    strcpy(proc_path, "/proc/");
    strcat(proc_path, pid);
    strcat(proc_path, "/fd/");

    struct dirent* dirp;
    DIR* dp;

    if ((dp = opendir(proc_path)) == NULL)
        err_exit("opendir");

    while ((dirp = readdir(dp)) != NULL) {
        char* fd = dirp->d_name;

        if (strcmp(fd, ".") == 0 || strcmp(fd, "..") == 0)
            continue;

        char*  link = malloc(proc_path_size + MAX_FD_LENGTH);
        char   file_path[MAX_FILE_PATH];
        size_t file_path_len;

        strcpy(link, proc_path);
        strcat(link, fd);

        file_path_len = readlink(link, file_path, MAX_FILE_PATH - 1);

        if (file_path_len == -1)
            err_exit("readlink");
        
        file_path[file_path_len] = '\0';
        printf("%s \t %s\n", fd, file_path);
        free(link);
    }

    return 0;
}