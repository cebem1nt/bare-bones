#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>

void
err_exit(const char* msg) 
{
    perror(msg);
    exit(1);
}

void
print_times(clock_t real, struct tms* tms_start, 
                          struct tms* tms_end) 
{
    static long clktck = 0;

    if (clktck == 0) 
        clktck = sysconf(_SC_CLK_TCK);

    printf(" real: %7.2f\n", real / (double) clktck);
    printf(" user: %7.2f\n",
        (tms_end->tms_utime - tms_start->tms_utime) / (double) clktck);

    printf(" sys: %7.2f\n",
        (tms_end->tms_stime - tms_start->tms_stime) / (double) clktck);
    printf(" child user: %7.2f\n",
        (tms_end->tms_cutime - tms_start->tms_cutime) / (double) clktck);
    printf(" child sys: %7.2f\n",
        (tms_end->tms_cstime - tms_start->tms_cstime) / (double) clktck);
}

void 
measure(char* cmd) 
{
    struct tms  tms_start, tms_end;
    clock_t     start, end;
    int         status;

    if ((start = times(&tms_start)) == -1)
        err_exit("times");

    if ((status = system(cmd)) == -1)
        err_exit("system");

    if ((end = times(&tms_end)) == -1)
        err_exit("times");

    printf("\ncommand: %s\n", cmd);
    print_times(end - start, &tms_start, &tms_end);
}

int 
main(int argc, char** argv) 
{
    for (int i = 1; i < argc; i++) {
        measure(argv[i]);
    }
}
