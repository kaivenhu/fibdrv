#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "fibdrv.h"

#include "utime.h"

#define FIB_DEV "/dev/fibonacci"
#define OPTION_CHECK "check"
#define OPTION_BECHMARK "benchmark"
#define FIB_MAX_NUM (MAX_LENGTH + 1)
#define BENCHMARK_FILE "result.dat"

void check(const int fd)
{
    int offset = 100;
    long long sz;
    f_uint num;

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
               i,
               (sizeof(num) > 8)
                   ? (unsigned long long) ((num >> 64) & ~((uint64_t) 0))
                   : 0,
               (unsigned long long) (num & ~((uint64_t) 0)));
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
               i,
               (sizeof(num) > 8)
                   ? (unsigned long long) ((num >> 64) & ~((uint64_t) 0))
                   : 0,
               (unsigned long long) (num & ~((uint64_t) 0)));
    }
}

typedef struct BenchmarkResult {
    double user_mean[FIB_MAX_NUM];
    double cal_mean[FIB_MAX_NUM];
    double kernel2user_mean[FIB_MAX_NUM];
    double user_var[FIB_MAX_NUM];
    double cal_var[FIB_MAX_NUM];
    double kernel2user_var[FIB_MAX_NUM];
} BenchmarkResult;

static inline void cal_time(const unsigned int run,
                            const double t,
                            double *mean,
                            double *var)
{
    double delta = t - *mean;
    *mean += (delta / (double) run);
    *var += delta * (t - *mean);
}

static void read_proxy(const int fd,
                       const unsigned int run,
                       const unsigned int offset,
                       BenchmarkResult *result)
{
    struct timespec prev_t, cur_t;
    f_uint num;
    size_t sz;

    lseek(fd, offset, SEEK_SET);
    clock_gettime(CLOCK_BOOTTIME, &prev_t);
    sz = read(fd, &num, sizeof(num));
    clock_gettime(CLOCK_BOOTTIME, &cur_t);
    if (!sz) {
        printf("Failed to read from " FIB_DEV " at offset %u\n", offset);
        exit(1);
    }
    cal_time(run, (double) timespec_to_ns(timespec_diff(prev_t, cur_t)),
             &(result->user_mean[offset]), &(result->user_var[offset]));
    cal_time(run, (double) read_fib_cal_time(), &(result->cal_mean[offset]),
             &(result->cal_var[offset]));
    cal_time(run, (double) read_kernel_to_user_time(),
             &(result->kernel2user_mean[offset]),
             &(result->kernel2user_var[offset]));
}

static void record_result(BenchmarkResult *result)
{
    FILE *fp = fopen(BENCHMARK_FILE, "w");
    if (NULL == fp) {
        printf("Failed to write benchmark result");
        exit(1);
    }
    for (unsigned int i = 0; i < FIB_MAX_NUM; ++i) {
        printf("%u: user %lf/%lf, cal %lf/%lf, kernel %lf/%lf \n", i,
               result->user_mean[i], sqrt(result->user_var[i]),
               result->cal_mean[i], sqrt(result->cal_var[i]),
               result->kernel2user_mean[i], sqrt(result->kernel2user_var[i]));
        fprintf(fp, "%u %lf %lf %lf\n", i, result->user_mean[i],
                result->cal_mean[i], result->kernel2user_mean[i]);
    }
    fclose(fp);
}

static void benchmark(const int fd, unsigned int runtime)
{
    BenchmarkResult *result =
        (BenchmarkResult *) calloc(sizeof(BenchmarkResult), 1);
    if (NULL == result) {
        printf("Failed to calloc benchmark result");
        exit(1);
    }
    for (unsigned int i = 1; i <= runtime; ++i) {
        for (unsigned int k = 0; k < FIB_MAX_NUM; ++k) {
            read_proxy(fd, i, k, result);
        }
    }
    for (unsigned int k = 0; k < FIB_MAX_NUM; ++k) {
        result->user_var[k] /= runtime;
        result->cal_var[k] /= runtime;
        result->kernel2user_var[k] /= runtime;
    }
    record_result(result);
    free(result);
}

int main(int argc, char *argv[])
{
    if (3 > argc || NULL == argv[1] || NULL == argv[2]) {
        printf("Usage: %s [%s|%s] [%s|%s]\n", argv[0], OPT_FIB_NAIVE,
               OPT_FIB_FAST_DOUBLE, OPTION_CHECK, OPTION_BECHMARK);
        exit(1);
    }

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    write(fd, argv[1], strlen(argv[1]));

    if (0 == strcmp(argv[2], OPTION_CHECK)) {
        check(fd);
    } else if (0 == strcmp(argv[2], OPTION_BECHMARK)) {
        if (3 > argc && NULL == argv[3]) {
            printf("Usage: %s " OPTION_BECHMARK " <runtime>", argv[0]);
            exit(1);
        }
        benchmark(fd, atoi(argv[3]));
    } else {
        printf("Invalid option: %s\n", argv[2]);
        exit(1);
    }

    close(fd);
    return 0;
}
