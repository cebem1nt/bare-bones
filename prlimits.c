#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

#define PRLIMIT(name) printlimit(#name, name)

void 
error(const char* reason) 
{
    perror(reason);
    exit(EXIT_FAILURE);
}

void 
printlimit(const char* name, int resource) 
{    
    struct rlimit lim;

    if (getrlimit(resource, &lim) == -1)
        error("getrlimit");
    
    printf("%-20s ", name);
    
    if (lim.rlim_cur == RLIM_INFINITY)
        printf("(infinite) ");
    else
        printf("%ld ", lim.rlim_cur);

    if (lim.rlim_max == RLIM_INFINITY)
        printf("(infinite)");
    else
        printf("%ld", lim.rlim_max);

    putchar('\n');
}

int 
main()
{
    PRLIMIT(RLIMIT_CORE);
    PRLIMIT(RLIMIT_CPU);
    PRLIMIT(RLIMIT_DATA);
    PRLIMIT(RLIMIT_FSIZE);
    PRLIMIT(RLIMIT_NOFILE);
    PRLIMIT(RLIMIT_STACK);
    PRLIMIT(RLIMIT_AS);
    PRLIMIT(RLIMIT_LOCKS);
    PRLIMIT(RLIMIT_MEMLOCK);
    PRLIMIT(RLIMIT_MSGQUEUE);
    PRLIMIT(RLIMIT_NICE);
    PRLIMIT(RLIMIT_NOFILE);
    PRLIMIT(RLIMIT_NPROC);
    PRLIMIT(RLIMIT_RSS);
    PRLIMIT(RLIMIT_RTPRIO);
    PRLIMIT(RLIMIT_RTTIME);
    PRLIMIT(RLIMIT_SIGPENDING);
    PRLIMIT(RLIMIT_STACK);

    return 0;
} 