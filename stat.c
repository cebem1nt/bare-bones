#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MODE_STR_SIZE 11 // 10 + 1 for '\0'

const char*
get_file_type(struct stat* s) 
{
    switch (s->st_mode & S_IFMT) {
        case S_IFBLK:  return "block device";      break;
        case S_IFCHR:  return "character device";  break;
        case S_IFDIR:  return "directory";         break;
        case S_IFIFO:  return "FIFO/pipe";         break;
        case S_IFLNK:  return "symlink";           break;
        case S_IFREG:  return "regular file";      break;
        case S_IFSOCK: return "socket";            break;
        default:       return "unknown ???";       break;
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
print_stat(char* file) 
{
    struct stat s;

    if (lstat(file, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    const char* filetype = get_file_type(&s);
    char* mode = get_mode_pretty(&s);

    const char* uname = gid_to_name(s.st_uid);
    const char* gname = gid_to_name(s.st_gid);

    printf("  File: %s\t %s\n", file, filetype);
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
main(int argc, char** argv) 
{
    if (argc < 2) {
        fprintf(stderr, "Usage: stat <file 1> <file 2> <file n> ....\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        print_stat(argv[i]);
    }

    return 0;
}