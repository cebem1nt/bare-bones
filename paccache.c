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

#define HM_INIT(hm, capacity) DA_INIT(hm, capacity)

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
    bool       do_remove,
               reverse;
} args_t;

struct hm_bucket {
    packages_t val;
    char*      key;
};

struct hmap {
    struct hm_bucket** arr;
    size_t cap, len;
};

static uint32_t 
djb2(const char* str) 
{
    uint32_t hash = 5381;
    char c;

    while ((c = *str++))
        hash = 33 * hash + c;

    return hash;
}

struct hm_bucket* hm_new_bucket(const char* key, packages_t value) 
{
    struct hm_bucket* out = malloc(sizeof(struct hm_bucket));
    
    if (out == NULL)
        error_exit("No memory!");

    out->val = value;
    out->key = strdup(key);
    return out;
}

void hm_resize(struct hmap* hm)
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

    free(hm->arr);
    hm->arr = new_arr;
    hm->cap = new_cap;
}

void hm_set(struct hmap* hm, const char* key, packages_t value) 
{
    struct hm_bucket* new_bucket;
    struct hm_bucket* itr;

    int cap_percent = (hm->len * 100) / hm->cap;

    if (cap_percent >= HM_RESIZE_PERCENT)
        hm_resize(hm);

    new_bucket = hm_new_bucket(key, value);
    size_t index = HM_INDEX(key, hm->cap);

    itr = hm->arr[index];

    while (itr != NULL) {
        if (strcmp(itr->key, new_bucket->key) == 0) {
            itr->val = new_bucket->val;
            free(new_bucket);
            return;
        }

        index = (index + 1) % hm->cap;
        itr = hm->arr[index];
    }

    hm->arr[index] = new_bucket;
    hm->len++;
}

struct hm_bucket* hm_get(struct hmap* hm, const char* key) 
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


static inline int 
parse_int(const char* in)
{
    errno = 0;
    char* end;
    int out = strtol(in, &end, 10);

    if (errno != 0)
        perror_exit("Parsing error");
    if (end == in)
        error_exit("Parsing error: Expected a valid number\n");

    return out;
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
    if (is_signature(d->d_name) || d->d_type != DT_REG)
        return 0;
    return 1;
}

bool 
is_matching_pkg(char* pkg, const char* query, size_t query_len) 
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

size_t 
get_package_base(char* pkg) 
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

void 
find_cached(packages_t* queries, bool is_reversed) 
{
    struct dirent** namelist;
    int             n;

    n = scandir(CACHE_DIR, &namelist, selectpkgs, alphasort);

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

        free(namelist[n]);
    }

    free(namelist);
}

void 
remove_cached(packages_t* queries, int keep) 
{
    struct dirent** namelist;
    struct hmap     found;
    int             n;

    HM_INIT(found, HM_DEFAULT_CAP);

    n = scandir(CACHE_DIR, &namelist, selectpkgs, alphasort);
    
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
            struct hm_bucket* stored = hm_get(&found, query);
            
            if (stored != NULL) {
                DA_APPEND(stored->val, strdup(pkg));
                continue;
            }
            
            // First match, create a new one
            packages_t new;
            DA_INIT(new, DA_DEFAULT_CAP);
            DA_APPEND(new, strdup(pkg));
            hm_set(&found, query, new);
        }

        free(namelist[i]);
    }

    chdir(CACHE_DIR);

    for (int i = 0; i < found.cap; i++) {
        if (!found.arr[i])
            continue;

        packages_t cached = found.arr[i]->val;

        if (cached.len <= keep) {
            printf("0 items deleted\n");
            continue; // Skip this bucket
        }

        size_t n_deleted = 0,
               to_delete = cached.len - keep;

        for (int j = 0; j < cached.len; j++) {
            char* pkg = cached.arr[j];
            char  sig[MAXNAMLEN + 6];

            snprintf(sig, sizeof(sig), "%s.sig", pkg);

            if (n_deleted >= to_delete)
                break;

            if (remove(pkg) == -1) perror_exit("remove");
            remove(sig); // If it does not exist, do nothing
            n_deleted++;
        }

        printf("%lu items deleted\n", n_deleted);
    }

    free(namelist);
}

void 
remove_cached_all(size_t keep) 
{
    
}

void 
help(const char* program) 
{
    printf("Usage: %s [-r|--remove] [-k|--keep <NKEEP>] [PACKAGE ...]                          \n"
           "  -r, --remove    Remove matching entries from the cache instead of printing them. \n"
           "                  If no packages are provided, clean up the entire cache,          \n"
           "                  keeping NKEEP versions.                                          \n"
           "  -k, --keep      Number of versions to keep when cleaning up the cache            \n"
           "  -R, --reverse   Print cached versions in reverse order when searching.           \n"
           ,program);

    exit(EXIT_FAILURE);
}

args_t 
argparse(char** argv) 
{
    args_t out = { 
        .nkeep = NKEEP_DEFAULT, 
        .do_remove = false,
        .reverse = false
    };

    DA_INIT(out.pkgs, DA_DEFAULT_CAP);

    for (char** itr = argv + 1; *itr != NULL; itr++) {
        char* arg = *itr;

        if (IS(arg, "-k") || IS(arg, "--keep")) {
            arg = *(++itr);

            if (arg != NULL)
                out.nkeep = parse_int(arg);
            else
                error_exit("Parsing error: --keep expects a number\n");
        }
        else if (IS(arg, "-r") || IS(arg, "--remove"))
            out.do_remove = true;
        else if (IS(arg, "-R") || IS(arg, "--reverse"))
            out.reverse = true;
        else
            DA_APPEND(out.pkgs, arg);
    }

    return out;
}

int 
main(int argc, char** argv)
{
    args_t args = argparse(argv);
    
    if (args.do_remove && args.pkgs.len == 0)
        remove_cached_all(args.nkeep);
    else if (args.do_remove)
        remove_cached(&args.pkgs, args.nkeep);
    else
        find_cached(&args.pkgs, args.reverse);
    
    return 0;
}