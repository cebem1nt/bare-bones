/*
 * I assume that linux kernel literally maps opened file descriptors to
 * the /proc/.../fd/ . So when you use fd = open(file_name ...) the kernel
 * actually makes a symbolic link to file_name in the directory /proc/<pid>/fd/ !
 * So what we just have to readlink of each file based on given pid
 */

#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>

#define MAX_FD_LENGTH 256
#define MAX_FILE_PATH 1024
#define MAX_TIME_LEN 20

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

void
read_fd_link(char* link, char* dest) 
{
    size_t len;
    len = readlink(link, dest, MAX_FILE_PATH - 1);
    if (len == -1)
        err_exit("readlink");
    
    dest[len] = '\0';
}

char*
uid_to_name(uid_t uid) 
{
    struct passwd* p = getpwuid(uid);
    return p->pw_name;
}

void format_time(time_t t, char* buf, size_t len) 
{
    struct tm tm;

    if (localtime_r(&t, &tm) == NULL) {
        buf[0] = '\0';
        return;
    }

    strftime(buf, len, "%m-%d %H:%M:%S", &tm);
}

int
main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: pfiles <pid>\n");
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
        char   file_path[MAX_FILE_PATH];
        char   time_buf[MAX_TIME_LEN];

        char*  proc_fd = dirp->d_name;
        struct stat st;

        if (strcmp(proc_fd, ".") == 0 || strcmp(proc_fd, "..") == 0)
            continue;

        char* link = malloc(proc_path_size + MAX_FD_LENGTH);
        if (!link)
            err_exit("malloc");

        strcpy(link, proc_path);
        strcat(link, proc_fd);

        read_fd_link(link, file_path);
        if (stat(link, &st) == -1)
            err_exit("stat");
        
        char* owner = uid_to_name(st.st_uid);
        format_time(st.st_mtim.tv_sec, time_buf, MAX_TIME_LEN);

        printf("%s \t %s %s - %s\n", proc_fd, time_buf, owner, file_path);
        free(link);
    }

    return 0;
}
