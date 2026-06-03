/*
 * While coding downgrade python script, I saw that searching for specific
 * files in a directory is kinda slow in python. Purpose of this tiny program is 
 * to see how fast can such task be executed in c.
 *
 * Result:
 *  C:      0.00s user 0.01s system 93% cpu 0.012 total
 *  Python: 0.60s user 0.12s system 98% cpu 0.723 total
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define CACHE_DIR "/var/cache/pacman/pkg"

void perror_exit() 
{
    perror("fcache");
    exit(EXIT_FAILURE);
}

int endswith(const char* str, const char* sub, size_t substr_len) 
{
    size_t str_len = strlen(str);
    
    if (substr_len > str_len)
        return 0;


    return strncmp(str + (str_len - substr_len), sub, substr_len) == 0;
}

void find_cached(const char* query) 
{
    DIR*             dp;
    struct dirent*   dir;
    size_t     query_len;
    
    if ((query_len = strlen(query)) > 256) {
        fprintf(stderr, "Given package name is too long.\n");
        exit(EXIT_FAILURE);
    }

    if ((dp = opendir(CACHE_DIR)) == NULL)
        perror_exit();

    while ((dir = readdir(dp)) != NULL) {
        if (strncmp(dir->d_name, query, query_len) != 0)
            continue;

        if (endswith(dir->d_name, ".sig", 4))
            continue;

        char* ptr = dir->d_name;
        ptr += query_len;

        if (*ptr != '-')
            continue;

        ptr++;
        
        if (!isdigit(*ptr))
            continue;

        printf(CACHE_DIR"/%s\n", dir->d_name);
    }
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s <package name>\n", argv[0]);
        return 1;
    }

    find_cached(argv[1]);
    return 0;
}