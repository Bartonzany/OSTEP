# 20 - 线程 API

POSIX 线程（pthread）就三样东西：**创建/等待线程、锁（互斥量）、条件变量**。创建线程跑任务，锁保证临界区互斥，条件变量做线程间"等待-唤醒"。用不好就出竞态、死锁、自旋空转。

## 一、线程创建

```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void*), void *arg);
```

- `thread`：线程 id 指针，后面 `pthread_join` 用；
- `attr`：线程属性（栈大小、优先级等），默认传 `NULL`；
- `start_routine`：入口函数，参数和返回值都必须是 `void*`；
- `arg`：传给入口函数的参数，靠 `void*` 适配任意类型。

传多个参数就包成结构体、传结构体指针；传单个简单值（如 `long long` 和指针同宽）可直接强转成 `void*`。

## 二、线程完成与返回值

```c
int pthread_join(pthread_t thread, void **value_ptr);
```

关键作用：

- 阻塞调用线程，直到目标线程执行完毕；
- 接收目标线程的返回值：`value_ptr` 是指向"返回值指针"的指针，线程通过 `return (void*)result` 返回结果。

返回值传递注意事项：

- 允许返回任意类型数据（通过 `void*`），但**禁止返回线程栈上的局部变量指针**（线程结束后栈空间释放，指针失效）；
- 若无需返回值，`value_ptr` 传 `NULL`；
- 简单场景可直接返回数值（如 `return (void*)100`），接收时再强制转换。

## 三、锁（互斥量）

```c
int pthread_mutex_lock(pthread_mutex_t *mutex);    // 拿不到就阻塞
int pthread_mutex_unlock(pthread_mutex_t *mutex);  // 只有持锁线程能解
```

### 1. 初始化方式

- 静态初始化（全局/静态锁）：`pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;`
- 动态初始化（运行时）：`pthread_mutex_init(&lock, NULL);`（需搭配 `pthread_mutex_destroy(&lock)` 销毁）；
- 必须初始化：未初始化的锁会导致程序行为异常。

### 2. 扩展接口

- `pthread_mutex_trylock`：尝试加锁，失败立即返回（不阻塞）；
- `pthread_mutex_timedlock`：超时未获取锁则返回，避免无限阻塞；
- 适用场景：死锁检测等需避免长期阻塞的场景，一般场景优先用 `lock`/`unlock`。

### 3. 使用规范

- 临界区前后必须配对调用 `lock` 和 `unlock`；
- 加锁后临界区应尽量简短，减少阻塞其他线程的时间；
- 必须检查返回值（可通过封装函数断言成功），避免锁操作失败导致竞态条件。

## 四、条件变量

```c
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
```

用来做线程间"等一个条件成立"（如线程A等待线程B完成某操作后再执行），替代自旋空转。三条规则：

1. 调 `wait`/`signal` 时必须持有锁；
2. `cond_wait` 会**先释放锁**、让别的线程进来，被唤醒后**重新拿锁**再返回；
3. 初始化：支持静态（`PTHREAD_COND_INITIALIZER`）和动态（`pthread_cond_init`）两种方式，需对应销毁。
4. 用`while`循环检查条件：避免虚假唤醒（部分实现可能无信号却唤醒线程），确保条件满足后再执行后续逻辑：

```c
Pthread_mutex_lock(&lock);
while (condition == false) {
    Pthread_cond_wait(&cond, &lock);
}
Pthread_mutex_unlock(&lock);
```

对比自旋等待（`while(flag==0);`）：自旋烧 CPU 还容易错；条件变量让线程休眠、不占 CPU，靠锁保数据一致。

## 五、编译

`#include <pthread.h>`，编译加 `-pthread`：`gcc -o main main.c -Wall -pthread`。

## 六、避坑指南

1. 保持简单：线程交互越简单越好，复杂交互易出 bug；
2. 减少线程间共享资源和通信，每处交互都谨慎设计；
3. 锁和条件变量必须初始化（静态或动态）；
4. 所有 pthread 接口都可能失败，检查返回码或封装断言；
5. 不传/不返回栈上变量的指针（线程栈独立，别的线程访问会失效）；
6. 共享数据放堆或全局，别放线程局部栈；
7. 用条件变量通信，别用裸标志位；
8. 查手册 `man -k pthread`。

## 七、总结

1. 创建/等待是基础，锁保互斥，条件变量做通信，三者缺一不可。
2. 细节决定稳定性：初始化、错误检查、返回值传递，是避免程序崩溃或逻辑错误的关键；。
3. 适用：并行计算（拆任务）、I/O 密集（重叠 I/O 与计算）、服务器并发（多请求）。

## 八、作业方向（Code）

用 `homework/threads-api` 的样例 + `valgrind --tool=helgrind` 检测竞态/死锁。

1. 分析`main-race.c`的竞态条件，用helgrind验证，添加锁后观察检测结果；
2. 分析`main-deadlock.c`的死锁问题，用helgrind定位，对比`main-deadlock-global.c`的差异；
3. 分析`main-signal.c`的低效原因（自旋等待），对比`main-signal-cv.c`（条件变量）的优势，验证helgrind检测结果；
4. 编写简单多线程程序，练习线程创建、锁、条件变量的组合使用，用helgrind排查问题。

### 1. main-race.c（竞态，2 个错误）

`balance++` 被主线程和 worker 线程同时无保护访问，helgrind 报两次竞态：

```shell
valgrind --tool=helgrind ./main-race
```

```text
Possible data race during read of size 4 at 0x... by thread #1
   at 0x...: main (main-race.c:15)     ← 主线程第 15 行 balance++
This conflicts with a previous write of size 4 by thread #2
   at 0x...: worker (main-race.c:8)     ← worker 第 8 行 balance++
Possible data race during write of size 4 ...
   at 0x...: main (main-race.c:15)
ERROR SUMMARY: 2 errors from 2 contexts
```

helgrind 精确指出 `balance` 在 worker 第 8 行写、主线程第 15 行又读又写，两者无锁冲突。给 `balance++` 加锁后就是 0 错误。

### 2. main-deadlock.c（死锁）vs main-deadlock-global.c

main-deadlock.c 两个线程按**相反顺序**拿 m1、m2：

```shell
valgrind --tool=helgrind ./main-deadlock
```

```text
Thread #3: lock order "0x... before 0x..." violated
   at 0x...: worker (main-deadlock.c:10)   ← 线程0 先拿 m1
   at 0x...: worker (main-deadlock.c:11)   ←      再拿 m2
   at 0x...: worker (main-deadlock.c:13)   ← 线程1 先拿 m2
   at 0x...: worker (main-deadlock.c:14)   ←      再拿 m1
ERROR SUMMARY: 1 errors
```

helgrind 用"锁获取顺序"报出这个死锁隐患——两个线程拿同一对锁、顺序相反。

`main-deadlock-global.c` 加了全局锁 `g`，把"取 m1+m2"整段包起来，运行时不会真死锁；**但 helgrind 仍然报 lock order violated**——因为它检查的是"锁获取顺序是否一致"，不是"这次运行有没有真卡住"。它照样看到线程 0 拿 m1→m2、线程 1 拿 m2→m1，顺序相反，照旧报警：

```shell
valgrind --tool=helgrind ./main-deadlock-global
```

```text
Thread #3: lock order "0x... before 0x..." violated
ERROR SUMMARY: 1 errors
```

所以结论：`g` 能防运行时死锁，但 helgrind 的"锁序检查"仍告警——它报的是潜在锁序不一致，需要你自己判断"到底会不会真死锁"。真正干净的做法是**固定拿锁顺序**（比如都先 m1 后 m2），而不是加全局大锁。

### 3. main-signal.c vs main-signal-cv.c（自旋 vs 条件变量）

`main-signal.c` 用 `while (done == 0);` 忙等，对 `done` 无同步：

```shell
valgrind --tool=helgrind ./main-signal
valgrind --tool=helgrind ./main-signal-cv
```

```text
main-signal:    Possible data race ... "done"（main 第 16 行读、worker 第 9 行写）
                ERROR SUMMARY: 1 errors
main-signal-cv: ERROR SUMMARY: 0 errors
```

自旋版本对 `done` 的读/写无同步，helgrind 报竞态（而且忙等还烧 CPU）；条件变量版本用锁 + `cond_wait`，0 错误、也不空转。

### 4. 组合练习（生产者-消费者）

写了个生产者-消费者程序（代码在 `homework/threads-api/main-pc.c`），三样东西全用上：创建/join 线程、`pthread_mutex` 保护缓冲区、两个条件变量（`not_full`/`not_empty`）做"满/空"的等待唤醒：

```c
#define MAX 10
int buffer[MAX], count = 0, fill_ptr = 0, use_ptr = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  not_full  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  not_empty = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {
    for (int i = 0; i < loops; i++) {
        pthread_mutex_lock(&lock);
        while (count == MAX) pthread_cond_wait(&not_full, &lock);  // 满等空
        buffer[fill_ptr] = i; fill_ptr = (fill_ptr+1) % MAX; count++;
        pthread_cond_signal(&not_empty);                            // 通知有数据
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}
void *consumer(void *arg) {
    int sum = 0;
    for (int i = 0; i < loops; i++) {
        pthread_mutex_lock(&lock);
        while (count == 0) pthread_cond_wait(&not_empty, &lock);    // 空等满
        sum += buffer[use_ptr]; use_ptr = (use_ptr+1) % MAX; count--;
        pthread_cond_signal(&not_full);                             // 通知有空位
        pthread_mutex_unlock(&lock);
    }
    printf("consumer sum = %d\n", sum);
    return NULL;
}
```

```shell
gcc -o main-pc main-pc.c -Wall -pthread
./main-pc
valgrind --tool=helgrind ./main-pc
```

```text
consumer sum = 499500                     ← 0+1+...+999，全部消费完、无丢失
ERROR SUMMARY: 0 errors from 0 contexts  ← helgrind 无竞态、无死锁
```

要点：生产者满了等 `not_full`、消费者空了等 `not_empty`，两边用 `while` 而非 `if` 重查条件；锁只包"放/取 + 改计数"，`signal` 在锁内发送保证原子。所以结果正确（499500）、helgrind 全绿。