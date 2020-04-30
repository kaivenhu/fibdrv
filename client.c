#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "bignum.h"
#include "utime.h"

#define FIB_DEV "/dev/fibonacci"
#define OPTION_CHECK "check"

void check(const int fd)
{
    int offset = 100;
    long long sz;
    BigNum num;

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, &num, sizeof(num));
        if (!sz) {
            printf("Failed to read from " FIB_DEV " at offset %d\n", i);
            exit(1);
        }
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "0x%llx%016llx.\n",
               i, num.upper, num.lower);
    }
    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, &num, sizeof(num));
        if (!sz) {
            printf("Failed to read from " FIB_DEV " at offset %d\n", i);
            exit(1);
        }
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "0x%llx%016llx.\n",
               i, num.upper, num.lower);
    }
}

int main(int argc, char *argv[])
{
    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    if (2 > argc && NULL == argv[1]) {
        printf("Usage: %s [%s]\n", argv[0], OPTION_CHECK);
        exit(1);
    }

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        long long sz;
        sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }

    if (0 == strcmp(argv[1], OPTION_CHECK)) {
        check(fd);
    } else {
        printf("Invalid option: %s\n", argv[1]);
        exit(1);
    }

    close(fd);
    return 0;
}
