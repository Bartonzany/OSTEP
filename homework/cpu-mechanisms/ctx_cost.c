#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/wait.h>

static double now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 100000;

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);    // pin both processes to CPU 0

    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);
    char c = 'x';

    if (fork() == 0) {
        for (int i = 0; i < n; i++) {
            read(p1[0], &c, 1);
            write(p2[1], &c, 1);
        }
        exit(0);
    }

    double s = now_us();
    for (int i = 0; i < n; i++) {
        write(p1[1], &c, 1);
        read(p2[0], &c, 1);
    }
    double e = now_us();
    printf("round trips=%d  per round trip=%.2f us  per switch=%.2f us\n",
           n, (e - s) / n, (e - s) / (2.0 * n));
    wait(NULL);
    return 0;
}