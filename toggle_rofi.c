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
    exit(1);                              \
} while(0)

typedef struct {
    size_t len, cap;
    char*  buf;
} dbuf_t;

static int read_growing(char* fname, dbuf_t* d) 
{
    d->len = 0;
    ssize_t  n;
    int     fd;

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

int get_running_cli(DIR* proc_dp, int argc, char** argv) 
{
    struct dirent*  ent;
    char   cmd[PATH_MAX];

    pid_t  selfpid = getpid();

    dbuf_t content = {
        .len = 0,
        .cap = BUFSIZ,
        .buf = NULL
    };
    
    if ((content.buf = malloc(content.cap)) == NULL)
        error_exit("No memory!");

    while ((ent = readdir(proc_dp)) != NULL) {
        if (!isdigit(ent->d_name[0])) // Skip, not a pid
            continue;

        pid_t file_name_pid = atoi(ent->d_name);

        if (file_name_pid == selfpid)
            continue; // Lookking at ourself

        snprintf(cmd, PATH_MAX, "/proc/%s/cmdline", ent->d_name);
        
        if (read_growing(cmd, &content) == -1)
            continue;
        
        int proc_argc = 0;
        size_t cursor = 0;

        while (cursor < content.len && proc_argc < argc) {
            char* proc_arg = &content.buf[cursor];
            // /proc/<pid>/cmdline separates each cmd line argument with \0

            if (strcmp(proc_arg, argv[proc_argc]) != 0)
                break;

            cursor += strlen(proc_arg) + 1;
            proc_argc += 1;
        }

        if (proc_argc == argc && cursor == content.len) {
            free(content.buf);
            return file_name_pid;
        }
    }

    free(content.buf);
    return -1;
}

void run_cli(int argc, char** argv)
{
    pid_t pid = fork();
    int         status;

    if (pid == -1)
        perror_exit("fork");

    if (pid == 0) {
        if (execvp(argv[0], argv) == -1)
            perror_exit("execvp");
    }

    waitpid(pid, &status, 0);
}

int main(int argc, char** argv) 
{
    if (argc <= 1)
        error_exit("A CLI expected, but nothing given\n");

    char* program = argv[0];
    char  presolved[PATH_MAX];
    DIR*  proc_dp;

    realpath(program, presolved);
    argv[0] = presolved;

    if ((proc_dp = opendir("/proc/")) == NULL)
        perror_exit("opendir");

    printf("Program name: %s\n", presolved);
    printf("CLI: ");
    for (int i = 1; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");

    pid_t running = get_running_cli(proc_dp, argc, argv);
    printf("Running: %i\n", running);

    if (running == -1)
        run_cli(argc - 1, argv + 1);
}