#include <stdio.h>
#include <unistd.h>
#include <limits.h>

int 
main() 
{
    char buf[PATH_MAX];
    
    if (getcwd(buf, PATH_MAX) != NULL) {
        printf("%s\n", buf);
        return 0;
    }

    return 1;
}