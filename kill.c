#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 20
#define STR_ISEQ(a, b) (strcmp(a, b) == 0)
#define STARTS_WITH(str, prefix, len) (strncasecmp(str, prefix, len) == 0)

typedef struct {
    const char *name;
    int val;
} signal_map_t;

static signal_map_t sig_map[] = {
    {"SIGHUP",  SIGHUP},  {"SIGINT",    SIGINT},    {"SIGQUIT",   SIGQUIT},
    {"SIGILL",  SIGILL},  {"SIGTRAP",   SIGTRAP},   {"SIGIOT",    SIGIOT},
    {"SIGABRT", SIGABRT}, {"SIGBUS",    SIGBUS},    {"SIGFPE",    SIGFPE},
    {"SIGKILL", SIGKILL}, {"SIGUSR1",   SIGUSR1},   {"SIGSEGV",   SIGSEGV},
    {"SIGUSR2", SIGUSR2}, {"SIGPIPE",   SIGPIPE},   {"SIGALRM",   SIGALRM},
    {"SIGTERM", SIGTERM}, {"SIGSTKFLT", SIGSTKFLT}, {"SIGCHLD",   SIGCHLD},
    {"SIGCONT", SIGCONT}, {"SIGSTOP",   SIGSTOP},   {"SIGTSTP",   SIGTSTP},
    {"SIGTTIN", SIGTTIN}, {"SIGTTOU",   SIGTTOU},   {"SIGURG",    SIGURG},
    {"SIGXCPU", SIGXCPU}, {"SIGXFSZ",   SIGXFSZ},   {"SIGVTALRM", SIGVTALRM},
    {"SIGPROF", SIGPROF}, {"SIGWINCH",  SIGWINCH},  {"SIGPOLL",   SIGPOLL}, 
    {"SIGPWR",  SIGPWR},  {"SIGSYS",    SIGSYS},    {NULL, -1}
};

int main(int argc, char** argv) 
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <pid> [ <signal> ]\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    int   sig = SIGKILL;
    char* sigtype = argc == 3 ? argv[2]
                              : "SIGKILL";

    if (pid == 0) {
        fprintf(stderr, "%s: invalid pid '%s'\n", argv[0], argv[1]);
        return 1;
    }

    if (!STARTS_WITH(sigtype, "SIG", 3)) {
        size_t len = strlen(sigtype);
        sigtype = malloc(len + 4);

        if (!sigtype)
            return 1;
        
        memcpy(sigtype, "SIG", 3);
        memcpy(sigtype + 3, sigtype, len);
        sigtype[len + 3] = '\0';
    } 
    
    for (signal_map_t* m = sig_map; m->name; m++) {
        if (STR_ISEQ(m->name, sigtype)) {
            sig = m->val;
            break;
        }
    }

    if (kill(pid, sig) == -1) {
        perror(argv[0]);
        return 1;
    }

    return 0;
}