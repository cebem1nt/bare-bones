#include <stdio.h>
#include <unistd.h>

int
main() 
{
    printf("Username: %s\n", getlogin());
    printf("User id: %i\n", getuid());
    printf("Group id: %i\n", getgid());

    return 0;
}