#include <stdio.h>
#include <pthread.h>

// 生产者-消费者：多线程 + 锁 + 条件变量的组合练习
#define MAX 10

int buffer[MAX];
int count = 0;
int fill_ptr = 0, use_ptr = 0;

pthread_mutex_t lock      = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  not_full  = PTHREAD_COND_INITIALIZER;   // 缓冲区没满，生产者等这个
pthread_cond_t  not_empty = PTHREAD_COND_INITIALIZER;   // 缓冲区没空，消费者等这个

int loops = 1000;

void put(int value) {
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % MAX;
    count++;
}

int get() {
    int tmp = buffer[use_ptr];
    use_ptr = (use_ptr + 1) % MAX;
    count--;
    return tmp;
}

void *producer(void *arg) {
    for (int i = 0; i < loops; i++) {
        pthread_mutex_lock(&lock);
        while (count == MAX)                  // 满了就等空位
            pthread_cond_wait(&not_full, &lock);
        put(i);
        pthread_cond_signal(&not_empty);      // 放完通知"有数据了"
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *consumer(void *arg) {
    int sum = 0;
    for (int i = 0; i < loops; i++) {
        pthread_mutex_lock(&lock);
        while (count == 0)                    // 空了就等数据
            pthread_cond_wait(&not_empty, &lock);
        sum += get();
        pthread_cond_signal(&not_full);       // 取完通知"有空位了"
        pthread_mutex_unlock(&lock);
    }
    printf("consumer sum = %d\n", sum);
    return NULL;
}

int main(void) {
    pthread_t p, c;
    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
    return 0;
}
