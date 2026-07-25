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
 * 
 */

#include <limits.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>

#define IS(arg, str) strcmp(arg, str) == 0

#define CACHE_DIR         "/var/cache/pacman/pkg"
#define NKEEP_DEFAULT     3
#define DA_DEFAULT_CAP    20
#define HM_DEFAULT_CAP    5
#define HM_RESIZE_PERCENT 70

#define HM_INDEX(key, capacity) (djb2(key) % capacity) 
#define HM_INIT(hm, capacity) DA_INIT(hm, capacity)

#define DA_INIT(da, capacity) do {                              \
    size_t cap = (capacity) ? (capacity)                        \
                            : DA_DEFAULT_CAP;                   \
                                                                \
    (da).len = 0;                                               \
    (da).cap = cap;                                             \
    (da).arr = calloc(cap, sizeof(*(da).arr));                  \
    assert((da).arr && "Could not alloc array, no memory");     \
} while(0)

#define DA_APPEND(da, ...) do {                                        \
    if ((da).len >= (da).cap) {                                        \
        (da).cap = (da).cap ? (da).cap * 2                             \
                            : DA_DEFAULT_CAP;                          \
        (da).arr = realloc((da).arr, (da).cap * sizeof(*(da).arr));    \
        assert((da).arr && "Could not realloc array, no memory");      \
    }                                                                  \
    (da).arr[(da).len++] = __VA_ARGS__;                                \
} while(0) 

#define error_exit(...)                   \
    do {                                  \
        fprintf(stderr, __VA_ARGS__);     \
        exit(EXIT_FAILURE);               \
    } while (0)

#define perror_exit(msg) do  {            \
    perror(msg);                          \
    exit(1);                              \
} while(0)

typedef struct {
    size_t len, cap;
    char** arr;
} packages_t;

typedef struct {
    packages_t pkgs;
    int        nkeep;
    bool       remove,
               reverse,
               quiet;
} args_t;

struct hm_bucket {
    packages_t val;
    char*      key;
};

struct hmap {
    struct hm_bucket** arr;
    size_t cap, len;
};

uint32_t 
djb2(const char* str) 
{
    uint32_t hash = 5381;
    char c;

    while ((c = *str++))
        hash = 33 * hash + c;

    return hash;
}

struct hm_bucket* 
hm_new_bucket(const char* key, packages_t value) 
{
    struct hm_bucket* out = malloc(sizeof(struct hm_bucket));
    
    if (out == NULL)
        error_exit("No memory!");

    out->val = value;
    out->key = strdup(key);
    return out;
}

void 
hm_free_bucket(struct hm_bucket* b) 
{
    free(b->key);
    free(b->val.arr);
    free(b);
}

void 
hm_resize(struct hmap* hm)
{
    struct hm_bucket** new_arr;
    size_t new_cap = hm->cap * 2;

    if (new_cap < hm->cap)
        error_exit("Overflow. Too much entries");

    if ((new_arr = calloc(new_cap, sizeof(struct hm_bucket*))) == NULL)
        error_exit("No memory!");

    for (size_t i = 0; i < hm->cap; i++) {
        if (hm->arr[i] == NULL)
            continue;

        size_t index = HM_INDEX(hm->arr[i]->key, new_cap);

        while (new_arr[index] != NULL)
            index = (index + 1) % new_cap;

        new_arr[index] = hm->arr[i];
    }

    free(hm->arr); // Free the old array, dont free buckets
    hm->arr = new_arr;
    hm->cap = new_cap;
}

struct hm_bucket*
hm_set(struct hmap* hm, const char* key, packages_t value) 
{
    struct hm_bucket* itr;
    size_t index;

    int cap_percent = (hm->len * 100) / hm->cap;

    if (cap_percent >= HM_RESIZE_PERCENT)
        hm_resize(hm);

    index = HM_INDEX(key, hm->cap);
    itr = hm->arr[index];

    while (itr != NULL) {
        if (IS(itr->key, key)) {
            itr->val = value;
            return itr;
        }

        index = (index + 1) % hm->cap;
        itr = hm->arr[index];
    }

    hm->arr[index] = hm_new_bucket(key, value);
    hm->len++;
    return hm->arr[index];
}

struct hm_bucket* 
hm_get(struct hmap* hm, const char* key) 
{
    size_t index = HM_INDEX(key, hm->cap);
    struct hm_bucket* itr = hm->arr[index];

    while (itr != NULL) {
        if (IS(itr->key, key)) 
            return itr;
        
        index = (index + 1) % hm->cap;
        itr = hm->arr[index];
    }

    return NULL;
}

struct hm_bucket* 
hm_getdef(struct hmap* hm, const char* key) 
{
    struct hm_bucket* prev = hm_get(hm, key);

    if (!prev) {
        packages_t empty;
        DA_INIT(empty, DA_DEFAULT_CAP);
        return hm_set(hm, key, empty);
    }

    return prev;
}

int 
strtoi(const char* in)
{
    errno = 0;
    char* end;
    long out = strtol(in, &end, 10);

    if (errno != 0)
        perror_exit("Parsing error");
    if (end == in || *end != '\0')
        error_exit("Parsing error: Expected a valid number\n");
    if (out < INT_MIN || out > INT_MAX)
        error_exit("Parsing error: Out of int range\n");

    return (int)out;
}

static inline bool 
startswith(const char* str, const char* sub, size_t substr_len) 
{
    return strncmp(str, sub, substr_len) == 0;
}

static inline bool 
endswith(const char* str, const char* sub, size_t substr_len) 
{
    size_t str_len = strlen(str);
    
    if (substr_len > str_len)
        return 0;

    return strncmp(str + (str_len - substr_len), sub, substr_len) == 0;
}

static inline bool 
is_signature(const char* pkg) 
{
    if (endswith(pkg, ".sig", 4))
        return true;
    return false;
}

static inline bool 
is_package(const char* pkg) 
{
    if (endswith(pkg, ".tar.zst", 8))
        return true;
    return false;
}

static int 
selectpkgs(const struct dirent * d) 
{
    if (is_signature(d->d_name) || !is_package(d->d_name))
        return 0;
    return 1;
}

bool 
is_matching_pkg(char* pkg, const char* query, size_t query_len) 
{
    if (!startswith(pkg, query, query_len))
        return false;

    if (pkg[query_len] != '-')
        return false;

    return isdigit(pkg[query_len + 1]);
}

char* 
get_package_base(char* pkg) 
{
    size_t i = 0;
    size_t len = strlen(pkg);

    for (; i < len; i++) {
        char c = pkg[i];

        if (c == '-' && (i + 1) < len && isdigit(pkg[i + 1]))
            break;
    }

    char* key = strdup(pkg);
    key[i] = '\0';
    return key;
}

void 
find_cached(packages_t* queries, bool is_reversed) 
{
    struct dirent** namelist;
    int             n;

    if ((n = scandir(CACHE_DIR, &namelist, selectpkgs, alphasort)) == -1)
        perror_exit("scandir");

    for (int i = is_reversed ? n - 1  : 0;
                 is_reversed ? i >= 0 : i < n;
                 is_reversed ? i--    : i++) 
    {
        char* pkg = namelist[i]->d_name;

        for (int j = 0; j < queries->len; j++) {
            char*  query = queries->arr[j];
            size_t query_len = strlen(query);

            if (query_len > MAXNAMLEN)
                error_exit("Given package name \"%s\" is too long.\n", query);

            if (is_matching_pkg(pkg, query, query_len))
                printf(CACHE_DIR"/%s\n", pkg);
        }

        free(namelist[i]);
    }

    free(namelist);
}

static size_t 
_remove_packages(struct hm_bucket* b, int keep, bool quiet) 
{
    packages_t pkgs = b->val;
 
    size_t n_deleted = 0,
           to_delete = pkgs.len - keep;

    for (int j = 0; j < pkgs.len; j++) {
        char* pkg = pkgs.arr[j];
        char  sig[MAXNAMLEN];

        snprintf(sig, sizeof(sig), "%s.sig", pkg);

        if (n_deleted >= to_delete)
            break;

        remove(sig); // If it does not exist, do nothing
        
        if (remove(pkg) == -1) 
            perror_exit("remove");

        if (!quiet)
            printf("\t%s\n", pkg);
    
        n_deleted++;
        free(pkg);
    }

    hm_free_bucket(b);
    return n_deleted;
}

void 
remove_cached(packages_t* queries, int keep, bool quiet) 
{
    struct dirent** namelist;
    struct hmap     found;
    int             n;

    HM_INIT(found, HM_DEFAULT_CAP);

    if ((n = scandir(CACHE_DIR, &namelist, selectpkgs, alphasort)) == -1)
        perror_exit("scandir");
    
    for (int i = 0; i < n; i++) {
        char* pkg = namelist[i]->d_name;
        
        for (int j = 0; j < queries->len; j++) {
            char* query = queries->arr[j];
            size_t query_len = strlen(query);

            if (query_len > MAXNAMLEN)
                error_exit("Given package name \"%s\" is too long.\n", query);

            if (!is_matching_pkg(pkg, query, query_len))
                continue;
            
            // Matching one
            struct hm_bucket* stored = hm_getdef(&found, query);
            DA_APPEND(stored->val, strdup(pkg));
        }

        free(namelist[i]);
    }

    chdir(CACHE_DIR);

    for (int i = 0; i < found.cap; i++) {
        if (!found.arr[i])
            continue;

        struct hm_bucket* cached = found.arr[i];
        
        if (!quiet)
            printf("%s:\n", cached->key);

        if (cached->val.len <= keep) {
            printf("0 items deleted\n");
            hm_free_bucket(cached); // Skip this bucket, but free
            continue;
        }

        printf("%lu items deleted\n", _remove_packages(cached, keep, quiet));
    }

    free(namelist);
}

void 
remove_cached_all(size_t keep, bool quiet) 
{
    struct dirent** namelist;
    struct hmap     found;
    int             n;

    HM_INIT(found, HM_DEFAULT_CAP);

    if ((n = scandir(CACHE_DIR, &namelist, selectpkgs, alphasort)) == -1)
        perror_exit("scandir");
    
    for (int i = 0; i < n; i++) {
        char* pkg = namelist[i]->d_name;
        char* key = get_package_base(pkg);

        struct hm_bucket* stored = hm_getdef(&found, key);
        DA_APPEND(stored->val, strdup(pkg));

        free(key); // Key will be strdup'd in hm_new_bucket
        free(namelist[i]);
    }

    size_t deleted = 0;
    chdir(CACHE_DIR);

    for (size_t i = 0; i < found.cap; i++) {
        if (found.arr[i] == NULL)
            continue;

        if (!quiet)
            printf("%s:\n", found.arr[i]->key);

        deleted += _remove_packages(found.arr[i], keep, quiet);
    }

    printf("%lu items were deleted\n", deleted);
}

void 
help(const char* program) 
{
    printf("Usage: %s [-r|--remove] [-k|--keep <NKEEP>] [PACKAGE ...]                              \n"
           "  -r, --remove    Remove matching entries from the cache instead of printing them.     \n"
           "                  If no packages are provided, clean up the entire cache,              \n"
           "                  keeping NKEEP versions.                                              \n"
           "  -k, --keep      Number of versions to keep when cleaning up the cache, defaults to 3 \n"
           "  -R, --reverse   Print cached versions in reverse order when searching.               \n"
           "  -q, --quiet     Do not print each deleted file to stdout, just summary               \n"
           ,program);

    exit(EXIT_FAILURE);
}

args_t 
argparse(char** argv) 
{
    args_t out = { 
        .nkeep = NKEEP_DEFAULT, 
        .remove = false,
        .reverse = false,
        .quiet = false
    };

    DA_INIT(out.pkgs, DA_DEFAULT_CAP);

    for (char** itr = argv + 1; *itr != NULL; itr++) {
        char* arg = *itr;

        if (IS(arg, "-k") || IS(arg, "--keep")) {
            arg = *(++itr);

            if (arg != NULL)
                out.nkeep = strtoi(arg);
            else
                error_exit("Parsing error: --keep expects a number\n");
        }
        else if (IS(arg, "-r") || IS(arg, "--remove"))
            out.remove = true;
        else if (IS(arg, "-R") || IS(arg, "--reverse"))
            out.reverse = true;
        else if (IS(arg, "-q") || IS(arg, "--quiet"))
            out.quiet = true;
        else
            DA_APPEND(out.pkgs, arg);
    }

    return out;
}

int 
main(int argc, char** argv)
{
    args_t args = argparse(argv);
    
    if (args.remove && args.pkgs.len == 0)
        remove_cached_all(args.nkeep, args.quiet);
    else if (args.remove)
        remove_cached(&args.pkgs, args.nkeep, args.quiet);
    else if (args.pkgs.len > 0)
        find_cached(&args.pkgs, args.reverse);
    else
        help(argv[0]);

    return 0;
}