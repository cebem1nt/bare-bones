/*
 * Simple ulimit-like program to print 
 * different limits for a machine
 */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

void err_exit()
{
    perror("unlimit"); 
    exit(EXIT_FAILURE);
}

int
main()
{
    long int open_file,
             atexit_max,
             args_max,
             stream_max,
             page_size,
             symloop_max;
    errno = 0;

    open_file = sysconf(_SC_OPEN_MAX);
    atexit_max = sysconf(_SC_ATEXIT_MAX);
    args_max = sysconf(_SC_ARG_MAX);
    stream_max = sysconf(_SC_STREAM_MAX);
    symloop_max = sysconf(_SC_SYMLOOP_MAX);

    page_size = sysconf(_SC_PAGESIZE);

    if (open_file > 0)
        printf("Open files: %li\n", open_file);
    else if (open_file == -1 && errno == 0) 
        printf("Open files: unlimited\n");
    else
        err_exit();

    printf("Open streams: %li\n", stream_max);
    printf("Atexit handlers: %li\n", atexit_max);
    printf("Arguments: %li\n", args_max);

    if (symloop_max == -1 && errno == 0)
        printf("Links seen before ELOOP: indeterminate\n");
    else
        printf("Links seen before ELOOP: %li\n", symloop_max);

    putchar('\n');
    printf("Page size: %li\n", page_size);

    return 0;
}