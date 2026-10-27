#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"

sem_t s; 

void *child(void *arg) {
    printf("child\n");
    Sem_post(&s);   // 子线程完成，通知父线程
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    printf("parent: begin\n");
    Sem_init(&s, 0);   // 初始值 0
    Pthread_create(&p, NULL, child, NULL);
    Sem_wait(&s);       // 等子线程
    printf("parent: end\n");
    return 0;
}