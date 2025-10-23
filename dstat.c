/*
 * Fetches more detailed information about directory.
 * Also counts types of files in a directory
 *
 * TODO Multithreading
 */

#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

static long n_blk, n_char, n_dir, n_pipe, n_slink, n_reg, n_sock, 
       n_nread, n_unknown;

char*
path_alloc() 
{
    char* new = malloc(PATH_MAX+1);
    if (!new) {
        fprintf(stderr, "Out of memory");
        exit(EXIT_FAILURE);
    }

    return new;
}

int
classify(struct stat* s) 
{
    switch (s->st_mode & S_IFMT) {
        case S_IFBLK:  return n_blk++;      break;
        case S_IFCHR:  return n_char++;     break;
        case S_IFDIR:  return n_dir++;      break;
        case S_IFIFO:  return n_pipe++;     break;
        case S_IFLNK:  return n_slink++;    break;
        case S_IFREG:  return n_reg++;      break;
        case S_IFSOCK: return n_sock++;     break;
        default:       return n_unknown++;  break;
    }
}

int
traverse(char* path) 
{
    struct stat stbuf;
    struct dirent* dirp;
    char* end;
    DIR* dp;

    if (lstat(path, &stbuf) == -1)
        return n_unknown++;

    if (S_ISDIR(stbuf.st_mode) == 0)
        return classify(&stbuf);
    
    if ((dp = opendir(path)) == NULL)
        return n_nread++;

    end = path + strlen(path);

    *end++ = '/';
    *end = '\0';

    n_dir++;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) 
            continue;
        
        strcat(path, dirp->d_name);
        traverse(path);
        *end = '\0';
    }

    closedir(dp);
    return 0;
}

int 
main(int argc, char** argv) 
{
    if (argc != 2) {
        fprintf(stderr, "usage: dstat <starting-path>\n");
        return 1;
    }

    char* path = path_alloc();
    strncpy(path, argv[1], PATH_MAX);
    path[PATH_MAX-1] = '\0';

    traverse(path);
    unsigned long n_tot = n_reg + n_blk + n_char + n_dir + n_nread + n_pipe + n_slink + n_sock + n_unknown;

    printf(" Total elements: %lu\n", n_tot);
    printf("    Directories: %lu\n", n_dir);
    printf("  Regular files: %lu\n", n_reg);
    printf(" Symbolic links: %lu\n", n_slink);
    printf("   Socket files: %lu\n", n_sock);
    printf("     FIFO/pipes: %lu\n", n_pipe);
    printf("    Block files: %lu\n", n_blk);
    printf("     Char files: %lu\n", n_char);
    printf("  Couldn't read: %lu\n", n_nread);
    
    if (n_unknown > 0) 
        printf("Unknown files?: %lu\n", n_unknown);

    return 0;
}