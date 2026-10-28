#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

static double now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

int main(int argc, char* argv[]) {
    int            n  = argc > 1 ? atoi(argv[1]) : 1000000;
    int            fd = open("/dev/zero", O_RDONLY);
    char           c;
    struct timeval tv;

    // 1) measure the cost of consecutive gettimeofday() calls
    int    m  = 1000;
    double t0 = now_us();
    for (int i = 0; i < m; i++) gettimeofday(&tv, NULL);
    double t1 = now_us();
    printf("gettimeofday() cost: %.4f us/call\n", (t1 - t0) / m);

    // 2) measure the cost of an empty syscall (0-byte read)
    double s = now_us();
    for (int i = 0; i < n; i++) read(fd, &c, 0);
    double e = now_us();
    printf("empty syscall cost: %.4f us/call (n=%d)\n", (e - s) / n, n);
    return 0;
}