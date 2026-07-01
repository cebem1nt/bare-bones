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
    size_t keep;
    bool   do_remove;
} args_t;

#define CACHE_DIR "/var/cache/pacman/pkg"
#define NKEEP_DEFAULT 3
#define HELP \
    "Usage: %s [PACKAGE ...]\n" \
    "   -r, --remove    Remove cached instead of printing\n"

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

#define error_exit(...)                   \
    do {                                  \
        fprintf(stderr, __VA_ARGS__);     \
        exit(EXIT_FAILURE);               \
    } while (0)

void help_exit(const char* program) 
{
    printf(HELP, program);
    exit(EXIT_FAILURE);
}

void perror_exit() 
{
    perror("paccache");
    exit(EXIT_FAILURE);
}

bool startswith(const char* str, const char* sub, size_t substr_len) 
{
    return strncmp(str, sub, substr_len) == 0;
}

bool endswith(const char* str, const char* sub, size_t substr_len) 
{
    size_t str_len = strlen(str);
    
    if (substr_len > str_len)
        return 0;

    return strncmp(str + (str_len - substr_len), sub, substr_len) == 0;
}

bool is_signature(char* pkg) {
    if (endswith(pkg, ".sig", 4))
        return true;
    return false;
}

bool is_package(char* pkg) {
    if (endswith(pkg, ".tar.zst", 8))
        return true;
    return false;
}

bool is_matching_pkg(char* pkg, const char* query, size_t query_len) 
{
    if (!startswith(pkg, query, query_len))
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

size_t get_package_base(char* pkg) 
{
    size_t i = 0;
    size_t len = strlen(pkg);

    for (; i < len; i++) {
        char c = pkg[i];

        if (c == '-' && (i + 1) < len && isdigit(pkg[i + 1]))
            break;
    }

    return i;
}

void find_cached(const char* query) 
{
    DIR*             dp;
    struct dirent*   dir;
    size_t     query_len;
    
    if ((query_len = strlen(query)) > MAXNAMLEN)
        error_exit("Given package name is too long.\n");

    if ((dp = opendir(CACHE_DIR)) == NULL)
        perror_exit();

    while ((dir = readdir(dp)) != NULL) {
        char* pkg = dir->d_name;

        if (is_matching_pkg(pkg, query, query_len) && !is_signature(pkg))
            printf(CACHE_DIR"/%s\n", pkg);
    }
}

void remove_cached(const char* query, size_t query_len, size_t keep) 
{
    DIR*             dp;
    struct dirent*   dir;
    packages_t       packages, signatures;

    DA_INIT(&packages);
    DA_INIT(&signatures);

    if (query_len > MAXNAMLEN)
        error_exit("Given package name is too long.\n");

    if ((dp = opendir(CACHE_DIR)) == NULL)
        perror_exit();

    chdir(CACHE_DIR);
    
    while ((dir = readdir(dp)) != NULL) {
        char* pkg = dir->d_name;

        if (dir->d_type != DT_REG)
            continue;

        if (!is_matching_pkg(pkg, query, query_len))
            continue;

        if (is_signature(pkg))
            DA_APPEND(&signatures, strdup(pkg));
        else
            DA_APPEND(&packages, strdup(pkg));
    }

    closedir(dp);

    if (packages.len <= keep) 
        return;  

    size_t to_delete = packages.len - keep; 

    // packages array WILL have packages pre sorted.
    for (size_t i = 0; i < packages.len; i++) {
        char* pkg = packages.arr[i];
        char* sig = signatures.arr[i];

        size_t deleted = i + 1;
        
        if (deleted > to_delete) 
            return;

        if (remove(pkg) == -1) perror_exit();
        if (remove(sig) == -1) perror_exit();

        free(pkg);
        free(sig);
    }

    free(packages.arr);
    free(signatures.arr);
}

void remove_cached_all(size_t keep) 
{
    DIR*             dp;
    struct dirent*   dir;
    
    if ((dp = opendir(CACHE_DIR)) == NULL)
        perror_exit();

    chdir(CACHE_DIR);
    
    while ((dir = readdir(dp)) != NULL) {
        char* pkg = dir->d_name;

        if (dir->d_type != DT_REG)
            continue;

        if (!is_package(pkg) && !is_signature(pkg))
            continue;

        size_t pkg_base_len = get_package_base(pkg);
        remove_cached(pkg, pkg_base_len, keep);
    }
}

int main(int argc, char** argv)
{
    if (argc == 1)
        help_exit(argv[0]);

    packages_t  pkgs;
    args_t      args = {NKEEP_DEFAULT, false};

    DA_INIT(&pkgs);

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        
        if (arg[0] != '-')
            DA_APPEND(&pkgs, arg);
        else {
            arg++;

            if (arg[0] == 'r' || strcmp(arg, "-remove") == 0)
                args.do_remove = true;
            else
                help_exit(argv[0]);
        }
    }

    if (pkgs.len > 0) {
        for (size_t i = 0; i < pkgs.len; i++) {
            if (args.do_remove) 
                remove_cached(pkgs.arr[i], strlen(pkgs.arr[i]), args.keep);
            else
                find_cached(pkgs.arr[i]);
        }   
    } else if (args.do_remove) {
        remove_cached_all(args.keep);
    }

    return 0;
}