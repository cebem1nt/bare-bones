/*
 * I have a bash script that adds toggle behaviour to any rofi using applet.
 * It does the following:
 * 
 * - Takes in a cli to execute.
 *   - If there is a toggle_rofi with another cli, kill it, run given one
 *   - Else if there is such cli running, just kill it, end
 *   - Else just run given cli
 *
 * We will assume that this scipt is being runned using full path
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/limits.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define error_exit(...)                   \
    do {                                  \
        fprintf(stderr, __VA_ARGS__);     \
        exit(EXIT_FAILURE);               \
    } while (0)

#define perror_exit(msg) do  {            \
    perror(msg);                          \
    exit(EXIT_FAILURE);                   \
} while(0)

typedef struct {
    size_t len, cap;
    char*  buf;
} dbuf_t;

static int read_growing(char* fname, dbuf_t* d) 
{
    ssize_t  n;
    int     fd;

    d->len = 0;

    if ((fd = open(fname, O_RDONLY)) == -1)
        return -1;

    while ((n = read(fd, d->buf + d->len, d->cap - d->len)) > 0) {
        d->len += n;

        if (d->len == d->cap) {
            d->cap *= 2;
            if (!(d->buf = realloc(d->buf, d->cap)))
                error_exit("No memory!");
        }
    }

    close(fd);
    return 0;
}

void run_cli(int argc, char** argv)
{
    pid_t pid = fork();

    if (pid == -1)
        perror_exit("fork");

    if (pid == 0) {
        execvp(argv[0], argv);
        perror_exit("execvp");
    }

    waitpid(pid, NULL, 0);
}

void kill_and_run_cli(pid_t pid, int argc, char** argv) 
{
    if (kill(pid, SIGTERM) == -1)
        perror_exit("kill");
    run_cli(argc, argv);
}

static bool is_self_with_different_cli(dbuf_t cmdline_content,
                                       const char* program) 
{
    int proc_argc = 0;
    size_t cursor = 0;

    while (cursor < cmdline_content.len) {
        char* proc_arg = &cmdline_content.buf[cursor];

        if (proc_argc == 0 && strcmp(proc_arg, program) != 0)
            return false;

        cursor += strlen(proc_arg) + 1;
        proc_argc += 1;
    }

    if (proc_argc > 0)
        return true;

    return false;
}

static bool is_self_with_cli(dbuf_t cmdline_content,
                             int argc, char** argv) 
{
    int proc_argc = 0;
    size_t cursor = 0;

    while (cursor < cmdline_content.len && proc_argc < argc) {
        char* proc_arg = &cmdline_content.buf[cursor];
        // /proc/<pid>/cmdline separates each cmd line argument with \0

        if (strcmp(proc_arg, argv[proc_argc]) != 0)
            break;

        cursor += strlen(proc_arg) + 1;
        proc_argc += 1;
    }

    if (proc_argc == argc && cursor == cmdline_content.len)
        return true;

    return false;
}

void match_proc(int argc, char** argv) 
{
    struct dirent*  ent;
    DIR*        proc_dp;
    char   cmd[PATH_MAX];

    pid_t  selfpid = getpid();

    dbuf_t content = {
        .len = 0,
        .cap = BUFSIZ,
        .buf = NULL
    };
    
    if ((content.buf = malloc(content.cap)) == NULL)
        error_exit("No memory!");

    if ((proc_dp = opendir("/proc/")) == NULL)
        perror_exit("opendir");

    while ((ent = readdir(proc_dp)) != NULL) {
        if (!isdigit(ent->d_name[0])) // Skip, not a pid
            continue;

        pid_t pid = atoi(ent->d_name);

        if (pid == selfpid)
            continue; // Lookking at ourself

        snprintf(cmd, PATH_MAX, "/proc/%s/cmdline", ent->d_name);
        
        if (read_growing(cmd, &content) == -1)
            continue;
        
        if (is_self_with_cli(content, argc, argv)) {
            if (kill(-pid, SIGTERM) == -1)
                perror_exit("kill");
            
            return;
        } else if (is_self_with_different_cli(content, argv[0])) {
            kill_and_run_cli(-pid, argc - 1, argv + 1);
            return;
        }
    }

    // Nothing matched
    run_cli(argc - 1, argv + 1);
}

int main(int argc, char** argv) 
{
    if (argc <= 1)
        error_exit("A CLI expected, but nothing given\n");

    setpgid(0, 0);
    match_proc(argc, argv);
}