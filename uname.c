#include <sys/utsname.h>
#include <stdio.h>

int 
main() 
{
    struct utsname buf;

    if (uname(&buf) == -1) {
        perror("uname");
        return 1;
    }

    printf("%s %s %s %s %s\n", 
        buf.sysname, buf.release, buf.nodename, buf.machine, buf.version);
}