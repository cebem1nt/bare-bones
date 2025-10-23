/*
 * Simple shell
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXINPUT 4096
#define PROMPT   "> "

int 
main() 
{
    char   buf[MAXINPUT];
    int    status;
    pid_t  pid; 

    printf(PROMPT);
    while (fgets(buf, MAXINPUT, stdin) != NULL) {
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0; // Won't it allways end with '\n' ?

        if ( (pid = fork()) < 0) {
            perror("fork");
            exit(1);
        } 
        else if (pid == 0) { // Child
            execlp(buf, buf, (char*)0);
            fprintf(stderr, "Couldn't execute %s\n", buf);
            exit(127);
        } 

        if ((pid = waitpid(pid, &status, 0)) < 0) {
            perror("waitpid");
            return 1;
        }

        printf(PROMPT);
    }

    return 0;
}