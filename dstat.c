/*
 * Fetches more detailed information about directory.
 * Also counts types of files in a directory
 *
 * TODO Multithreading
 */

#include <stddef.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define MODE_STR_SIZE 11 // 10 + 1 for '\0'

static long double size_bytes;
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

char*
get_mode_pretty(struct stat* s)
{
    char* mode = malloc(MODE_STR_SIZE);
    if (!mode)
        return NULL;

    switch (s->st_mode & S_IFMT) {
        case S_IFREG:  mode[0] = '-'; break;
        case S_IFDIR:  mode[0] = 'd'; break;
        case S_IFLNK:  mode[0] = 'l'; break;
        case S_IFCHR:  mode[0] = 'c'; break;
        case S_IFBLK:  mode[0] = 'b'; break;
        case S_IFSOCK: mode[0] = 's'; break;
        case S_IFIFO:  mode[0] = 'p'; break;
        default:       mode[0] = '?'; break;
    }

    mode[1] = (s->st_mode & S_IRUSR) ? 'r' : '-';
    mode[2] = (s->st_mode & S_IWUSR) ? 'w' : '-';
    mode[3] = (s->st_mode & S_IXUSR) ? 'x' : '-';

    mode[4] = (s->st_mode & S_IRGRP) ? 'r' : '-';
    mode[5] = (s->st_mode & S_IWGRP) ? 'w' : '-';
    mode[6] = (s->st_mode & S_IXGRP) ? 'x' : '-';

    mode[7] = (s->st_mode & S_IROTH) ? 'r' : '-';
    mode[8] = (s->st_mode & S_IWOTH) ? 'w' : '-';
    mode[9] = (s->st_mode & S_IXOTH) ? 'x' : '-';

    mode[10] = '\0';

    // setuid/setgid/sticky bits
    if (s->st_mode & S_ISUID) {
        mode[3] = (mode[3] == 'x') ? 's' : 'S';
    }
    if (s->st_mode & S_ISGID) {
        mode[6] = (mode[6] == 'x') ? 's' : 'S';
    }
    if (s->st_mode & S_ISVTX) {
        mode[9] = (mode[9] == 'x') ? 't' : 'T';
    }

    return mode;
}

const char* uid_to_name(uid_t uid)
{
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : NULL;
}

const char* gid_to_name(gid_t gid)
{
    struct group *gr = getgrgid(gid);
    return gr ? gr->gr_name : NULL;
}

void
print_stat(char* dir) 
{
    struct stat s;

    if (lstat(dir, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    char* mode = get_mode_pretty(&s);

    const char* uname = gid_to_name(s.st_uid);
    const char* gname = gid_to_name(s.st_gid);

    printf("  Path: %s", dir);
    if (S_ISLNK(s.st_mode)) {
        char buf[PATH_MAX];

        size_t n = readlink(dir, buf, PATH_MAX);
        buf[n+1] = '\0';  

        printf(" -> %s", buf);
    }

    putchar('\n');
    printf("  Size: %ld\t Blocks: %ld\t IO Block: %ld\n", s.st_size, s.st_blocks, s.st_blksize);
    printf("Device: [%u,%u]\t Links: %lu\t Inode: %lu\n", 
            major(s.st_dev), minor(s.st_dev), s.st_nlink, s.st_ino);

    printf("  Mode: (%u/%s)\t Uid: %u/%s\t Gid: %u/%s\n", 
            s.st_mode, mode, s.st_uid, uname, s.st_gid, gname);

    printf("Access: %s", ctime(&s.st_atim.tv_sec));
    printf("Modify: %s", ctime(&s.st_mtim.tv_sec));
    printf(" Birth: %s", ctime(&s.st_ctim.tv_sec));

    free(mode);
}

int
traverse(char* path) 
{
    struct stat stbuf;
    struct dirent* dirp;
    char*  end;
    DIR*   dp;

    if (lstat(path, &stbuf) == -1)
        return n_unknown++;

    size_bytes += stbuf.st_size;
    if (S_ISDIR(stbuf.st_mode) == 0)
        return classify(&stbuf);
    
    if ((dp = opendir(path)) == NULL)
        return n_nread++;

    end = path + strlen(path);

    // Intresting thing, readdir doesnt care about
    // amount of slashes in part of directory, nor anything
    // on linux. You can try cd or ls with multiple ///
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

    print_stat(argv[1]);
    putchar('\n');
    printf(" Total elements: %lu\n", n_tot);
    printf("    Directories: %lu\n", n_dir);
    printf("  Regular files: %lu\n", n_reg);
    printf(" Symbolic links: %lu\n", n_slink);
    
    if (n_sock > 0)
        printf("   Socket files: %lu\n", n_sock);
    if (n_pipe > 0)
        printf("     FIFO/pipes: %lu\n", n_pipe);
    if (n_blk > 0)
        printf("    Block files: %lu\n", n_blk);
    if (n_char > 0)
        printf("     Char files: %lu\n", n_char);
    if (n_nread > 0)
        printf("  Couldn't read: %lu\n", n_nread);
    if (n_unknown > 0) 
        printf(" Unknown files?: %lu\n", n_unknown);

    int   times_divided = 0;
    char* mem[] = {"Bytes", "KiB", "MiB", "GiB"};

    for (;size_bytes > 1024; ++times_divided) {
        size_bytes /= 1024;
    }

    printf("        Total size: %.2Lf %s\n", size_bytes, mem[times_divided]);
    return 0;
}