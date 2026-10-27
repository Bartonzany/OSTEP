#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"

//
// 用"转门"（turnstile）实现无饥饿互斥锁：等待者逐个排队，
// 一次只放一个线程进临界区，按到达顺序拿锁，不会有人饿死。
//

typedef struct __ns_mutex_t {
    sem_t mutex;      // 真正的锁，1=空闲
    sem_t turnstile;  // 公平闸门
} ns_mutex_t;

ns_mutex_t m;
int shared = 0;
int loops = 100000;

void ns_mutex_init(ns_mutex_t *m) {
    Sem_init(&m->mutex, 1);
    Sem_init(&m->turnstile, 1);
}

void ns_mutex_acquire(ns_mutex_t *m) {
    Sem_wait(&m->turnstile);
    Sem_wait(&m->mutex);
    Sem_post(&m->turnstile);
}

void ns_mutex_release(ns_mutex_t *m) {
    Sem_post(&m->mutex);
}

void *worker(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
        ns_mutex_acquire(&m);
        shared++;
        ns_mutex_release(&m);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    assert(argc == 3);
    int num_threads = atoi(argv[1]);
    loops = atoi(argv[2]);

    pthread_t p[num_threads];

    printf("parent: begin\n");
    ns_mutex_init(&m);

    int i;
    for (i = 0; i < num_threads; i++)
        Pthread_create(&p[i], NULL, worker, NULL);
    for (i = 0; i < num_threads; i++)
        Pthread_join(p[i], NULL);

    printf("parent: end\n");
    printf("shared = %d, expected = %d\n", shared, num_threads * loops);
    return 0;
}