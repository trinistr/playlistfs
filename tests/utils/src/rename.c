#include <stdio.h>
#include <errno.h>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;
    if (rename (argv[1], argv[2]) == 0) {
        return 0;
    }
    else {
        perror ("rename");
        return errno;
    }
}
