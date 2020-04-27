#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utime.h"


#define FIB_CAL_TIME_SYSFS "/sys/kernel/fibdrv/fib_cal_time"
#define KERNEL_TO_USER_SYSFS "/sys/kernel/fibdrv/kernel_to_user_time"

struct timespec timespec_diff(const struct timespec start,
                              const struct timespec stop)
{
    struct timespec result;
    if ((stop.tv_nsec - start.tv_nsec) < 0) {
        result.tv_sec = stop.tv_sec - start.tv_sec - 1;
        result.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
    } else {
        result.tv_sec = stop.tv_sec - start.tv_sec;
        result.tv_nsec = stop.tv_nsec - start.tv_nsec;
    }

    return result;
}

unsigned long long timespec_to_ns(const struct timespec t)
{
    return t.tv_sec * 1000000000.0 + t.tv_nsec;
}

static inline unsigned long long read_time(const char *time_path)
{
    char buf[64] = {'\0'};
    unsigned long long nsec = 0;
    ssize_t sz;
    int fd = open(time_path, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open %s", time_path);
        exit(1);
    }
    sz = read(fd, buf, sizeof(buf));
    if (!sz) {
        printf("Failed to read from %s", time_path);
        exit(1);
    }
    if (1 != sscanf(buf, "%llu", &nsec)) {
        printf("Failed to parse time from %s", time_path);
        exit(1);
    }
    close(fd);
    return nsec;
}

unsigned long long read_fib_cal_time(void)
{
    return read_time(FIB_CAL_TIME_SYSFS);
}

unsigned long long read_kernel_to_user_time(void)
{
    return read_time(KERNEL_TO_USER_SYSFS);
}
