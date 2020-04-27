#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "bignum.h"
#include "utime.h"

#define FIB_DEV "/dev/fibonacci"

static inline void read_proxy(const int fd, const int offset)
{
    struct timespec prev_t, cur_t;
    long long sz;
    BigNum num;

    clock_gettime(CLOCK_BOOTTIME, &prev_t);
    sz = read(fd, &num, sizeof(num));
    clock_gettime(CLOCK_BOOTTIME, &cur_t);
    if (!sz) {
        printf("Failed to read from " FIB_DEV " at offset %d\n", offset);
        exit(1);
    }
    printf("Reading from " FIB_DEV
           " at offset %d, returned the sequence "
           "0x%llx%016llx. usertime: %llu, caltime: %llu, kern2user: %llu.\n",
           offset, num.upper, num.lower,
           timespec_to_ns(timespec_diff(prev_t, cur_t)), read_fib_cal_time(),
           read_kernel_to_user_time());
}

int main()
{
    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

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

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        read_proxy(fd, i);
    }
    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        read_proxy(fd, i);
    }

    close(fd);
    return 0;
}
