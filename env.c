/*
 * Sort of GNU env, but.
 *   - If no args, print all varibles
 *   - There is an arg treat it as variable to get
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char** environ;

int 
main(int argc, char** argv) 
{
    if (argc == 1) {
        char** env = environ;

        while (*env != NULL) {
            printf("%s\n", *env);
            ++env;
        }

        return 0;
    }

    for (int i = 1; i < argc; i++) {
        const char* env = getenv(argv[i]);

        if (env) {
            printf("%s\n", env);
        } else {
            return 1;
        }
    }

    return 0;
}