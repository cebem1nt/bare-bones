/*
 * While coding downgrade python script, I saw that searching for specific
 * files in a directory is kinda slow in python. Purpose of this tiny program is 
 * to see how fast can such task be executed in c.
 *
 * Result:
 *  C:      0.00s user 0.01s system 93% cpu 0.012 total
 *  Python: 0.60s user 0.12s system 98% cpu 0.723 total
 *
 * UPDATE:
 * This program will try to mimic some features of paccache
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct {
    size_t len, cap;
    char** arr;
} packages_t;

typedef struct {
    bool do_remove;
} args_t;

#define perror_exit()           \
    do {                        \
        perror("paccache");     \
        exit(EXIT_FAILURE);     \
    } while (0);

#define error_exit(...)                   \
    do {                                  \
        fprintf(stderr, __VA_ARGS__);     \
        exit(EXIT_FAILURE);               \
    } while (0);

#define DA_DEFAULT_CAP 10

#define DA_INIT(da) do {                                        \
    size_t cap = DA_DEFAULT_CAP;                                \
                                                                \
    (da)->len = 0;                                              \
    (da)->cap = cap;                                            \
    (da)->arr = malloc(cap * sizeof(*(da)->arr));               \
    assert((da)->arr && "Could not alloc array, no memory");    \
} while(0)

#define DA_APPEND(da, ...) do {                                         \
    if ((da)->len >= (da)->cap) {                                       \
        (da)->cap = (da)->cap ? (da)->cap * 2                           \
                              : DA_DEFAULT_CAP;                         \
        (da)->arr = realloc((da)->arr, (da)->cap * sizeof(*(da)->arr)); \
        assert((da)->arr && "Could not realloc array, no memory");      \
    }                                                                   \
    (da)->arr[(da)->len++] = __VA_ARGS__;                               \
} while(0) 

#define CACHE_DIR "/var/cache/pacman/pkg"

#define HELP \
    "Usage: %s [PACKAGE ...]\n" \
    "   -r, --remove    Remove cached instead of printing\n"

int endswith(const char* str, const char* sub, size_t substr_len) 
{
    size_t str_len = strlen(str);
    
    if (substr_len > str_len)
        return 0;

    return strncmp(str + (str_len - substr_len), sub, substr_len) == 0;
}


bool is_matching_pkg(char* pkg, const char* query, size_t query_len) 
{
    if (strncmp(pkg, query, query_len) != 0)
        return false;

    char* ptr = pkg;
    ptr += query_len;

    if (*ptr != '-')
        return false;

    ptr++;
    
    if (!isdigit(*ptr))
        return false;

    return true;
}

bool is_signature(char* pkg) {
    if (endswith(pkg, ".sig", 4))
        return true;
    return false;
}

void find_cached(const char* query) 
{
    DIR*             dp;
    struct dirent*   dir;
    size_t     query_len;
    
    if ((query_len = strlen(query)) > 256)
        error_exit("Given package name is too long.\n");

    if ((dp = opendir(CACHE_DIR)) == NULL)
        perror_exit();

    while ((dir = readdir(dp)) != NULL) {
        char* pkg = dir->d_name;

        if (is_matching_pkg(pkg, query, query_len) && !is_signature(pkg))
            printf(CACHE_DIR"/%s\n", pkg);
    }
}

void remove_cached(const char* query) 
{
    DIR*             dp;
    struct dirent*   dir;
    size_t     query_len;
    
    if ((query_len = strlen(query)) > 256)
        error_exit("Given package name is too long.\n");

    if ((dp = opendir(CACHE_DIR)) == NULL)
        perror_exit();

    chdir(CACHE_DIR);
    
    while ((dir = readdir(dp)) != NULL) {
        char* pkg = dir->d_name;

        if (!is_matching_pkg(pkg, query, query_len))
            continue;

        if (remove(pkg) == -1)
            perror_exit();
    }
}


int main(int argc, char** argv)
{
    if (argc == 1) {
        printf(HELP, argv[0]);
        return 1;
    }

    packages_t  pkgs;
    args_t      args;

    DA_INIT(&pkgs);

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        
        if (arg[0] != '-')
            DA_APPEND(&pkgs, arg);
        else {
            arg++;

            if (arg[0] == 'r' || strcmp(arg, "-remove") == 0)
                args.do_remove = true;
        }
    }

    for (size_t i = 0; i < pkgs.len; i++) {
        if (args.do_remove) 
            remove_cached(pkgs.arr[i]);
        else
            find_cached(pkgs.arr[i]);
    }   

    return 0;
}