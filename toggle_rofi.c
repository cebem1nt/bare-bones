/*
 * I have a bash script that adds toggle behaviour to any rofi using applet.
 * It does the following:
 * 
 * - Takes in a cli to execute.
 *   - If there is a toggle_rofi with another cli, kill it, run given one
 *   - Else if there is such cli running, just kill it, end
 *   - Else just run given cli
 *
 * We will assume that this scipt is being runned by with full path
 */

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/limits.h>
#include <string.h>
#include <unistd.h>

#define error_exit(...)                   \
    do {                                  \
        fprintf(stderr, __VA_ARGS__);     \
        exit(EXIT_FAILURE);               \
    } while (0)

#define perror_exit(msg) do  {            \
    perror(msg);                          \
    exit(1);                              \
} while(0)

typedef struct {
    size_t len, cap;
    char*  buf;
} dbuf_t;

static void read_growing(int fd, dbuf_t* d) 
{
    ssize_t n;

    while ((n = read(fd, d->buf + d->len, d->cap - d->len)) > 0) {
        d->len += n;

        if (d->len == d->cap) {
            d->cap *= 2;
            if (!(d->buf = realloc(d->buf, d->cap)))
                error_exit("No memory!");
        }
    }
}

bool is_cli_running(DIR* proc_dp, char* program, char** argv) 
{
    struct dirent*  ent;
    char   cmd[PATH_MAX];
    int    fd;

    dbuf_t dbuf = {
        .len = 0,
        .cap = BUFSIZ,
        .buf = NULL
    };
    
    if (!(dbuf.buf = malloc(dbuf.cap)))
        error_exit("No memory!");

    while ((ent = readdir(proc_dp)) != NULL) {
        const char* fname = ent->d_name;
        size_t len = 0;

        if (!isdigit(fname[0])) // Skip, not a pid
            continue;

        snprintf(cmd, PATH_MAX, "/proc/%s/cmdline", fname);
        
        printf("Filename: %s\n", fname);
        printf("Cmdline: %s\n", cmd);    

        if ((fd = open(cmd, O_RDONLY)) == -1)
            perror_exit("open");
        
        read_growing(fd, &dbuf);

        for (size_t i = 0; i < dbuf.len;) {
            char* arg = &dbuf.buf[i];
            i += strlen(arg) + 1;
            
            printf("%s\n", arg);
        }

        exit(1);
    }

    return false;
}

int main(int argc, char** argv) 
{
    if (argc <= 1)
        error_exit("A CLI expected, but nothing given\n");

    char* program = *(argv++);
    char  presolved[PATH_MAX];
    DIR*  proc_dp;

    realpath(program, presolved);

    if ((proc_dp = opendir("/proc/")) == NULL)
        perror_exit("opendir");

    is_cli_running(proc_dp, program, argv);    
}