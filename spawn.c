/*
 * A program to just spawn given arguments in a completely
 * detached way from the parent process / terminal
 */

#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("spawn: Creates a process from given argument[s] in a detached way and prints it's pid to stdout.\n" 
               "       Newly spawned process will have stdin, stdout, stderr redirected to /dev/null\n");
        return 1;
    }

    pid_t   pid;
    int     nullfd;

    nullfd = open("/dev/null", O_RDWR);

    if ((pid = fork()) == -1) {
        perror("fork");
    } else if (pid == 0) { // Child 
        const char* program = argv[1];
        argv += 2;
        
        dup2(nullfd, STDOUT_FILENO);
        dup2(nullfd, STDERR_FILENO);
        dup2(nullfd, STDIN_FILENO);
        
        setsid();
        execvp(program, argv);
    } else { // Parent
        printf("%d\n", pid);
        return 0;
    }
}