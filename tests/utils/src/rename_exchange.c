#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;
    if (renameat2 (AT_FDCWD, argv[1], AT_FDCWD, argv[2], RENAME_EXCHANGE) == 0) {
        return 0;
    }
    else {
        perror ("rename RENAME_EXCHANGE");
        return errno;
    }
}
