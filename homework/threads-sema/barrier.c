#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"

// If done correctly, each child should print their "before" message
// before either prints their "after" message. Test by adding sleep(1)
// calls in various locations.

// You likely need two semaphores to do this correctly, and some
// other integers to track things.

typedef struct __barrier_t {
    sem_t mutex;       // 保护 count 的锁
    sem_t turnstile1;  // 第一阶段放行
    sem_t turnstile2;  // 第二阶段放行（保证可复用）
    int count;         // 已到达的线程数
    int num_threads;   // 总数
} barrier_t;


// the single barrier we are using for this program
barrier_t b;

void barrier_init(barrier_t *b, int num_threads) {
    b->count = 0;
    b->num_threads = num_threads;
    Sem_init(&b->mutex, 1);
    Sem_init(&b->turnstile1, 0);
    Sem_init(&b->turnstile2, 0);
}

void barrier(barrier_t *b) {
    int i;
    // 第一阶段：到达并等所有人都到
    Sem_wait(&b->mutex);
    b->count++;
    if (b->count == b->num_threads) {
        for (i = 0; i < b->num_threads; i++)
            Sem_post(&b->turnstile1);
    }
    Sem_post(&b->mutex);
    Sem_wait(&b->turnstile1);

    // 第二阶段：离开并重置计数（保证可复用）
    Sem_wait(&b->mutex);
    b->count--;
    if (b->count == 0) {
        for (i = 0; i < b->num_threads; i++)
            Sem_post(&b->turnstile2);
    }
    Sem_post(&b->mutex);
    Sem_wait(&b->turnstile2);
}

//
// XXX: don't change below here (just run it!)
//
typedef struct __tinfo_t {
    int thread_id;
} tinfo_t;

void *child(void *arg) {
    tinfo_t *t = (tinfo_t *) arg;
    printf("child %d: before\n", t->thread_id);
    barrier(&b);
    printf("child %d: after\n", t->thread_id);
    return NULL;
}


// run with a single argument indicating the number of 
// threads you wish to create (1 or more)
int main(int argc, char *argv[]) {
    assert(argc == 2);
    int num_threads = atoi(argv[1]);
    assert(num_threads > 0);

    pthread_t p[num_threads];
    tinfo_t t[num_threads];

    printf("parent: begin\n");
    barrier_init(&b, num_threads);
    
    int i;
    for (i = 0; i < num_threads; i++) {
	t[i].thread_id = i;
	Pthread_create(&p[i], NULL, child, &t[i]);
    }

    for (i = 0; i < num_threads; i++) 
	Pthread_join(p[i], NULL);

    printf("parent: end\n");
    return 0;
}