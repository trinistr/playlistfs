#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
    if (argc < 1) {
        return 1;
    }
    struct stat s;
    if (lstat (argv[1], &s) < 0) {
        return errno;
    }
    // stat --format %X-%Y-%Z, but with nanoseconds
    fprintf (stdout, "%ld.%09ld-%ld.%09ld-%ld.%09ld\n",
        s.st_atim.tv_sec, s.st_atim.tv_nsec,
        s.st_mtim.tv_sec, s.st_mtim.tv_nsec,
        s.st_ctim.tv_sec, s.st_ctim.tv_nsec);
    return 0;
}
