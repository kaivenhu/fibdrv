#ifndef FIBDRV_UTIME_H_
#define FIBDRV_UTIME_H_
#include <time.h>

struct timespec timespec_diff(const struct timespec start,
                              const struct timespec stop);

unsigned long long timespec_to_ns(const struct timespec t);

unsigned long long read_fib_cal_time(void);

unsigned long long read_kernel_to_user_time(void);

#endif /* FIBDRV_UTIME_H_ */
