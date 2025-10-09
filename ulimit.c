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
    long int open_file;
    errno = 0;

#ifdef OPEN_MAX
    open_file = sysconf(OPEN_MAX);
#else
    // Lets assume that _SC_OPEN_MAX is set otherwise
    open_file = sysconf(_SC_OPEN_MAX);
#endif

    if (open_file > 0)
        printf("Open files: \t %li\n", open_file);
    else if (open_file == -1 && errno == 0) 
        printf("Open filesss: \t unlimited\n");
    else
        err_exit();

    return 0;
}