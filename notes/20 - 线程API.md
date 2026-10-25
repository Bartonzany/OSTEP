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

### 1. main-race.c（竞态）

`balance++` 被主线程和 worker 线程同时无保护访问：

```shell
make
valgrind --tool=helgrind ./main-race
```

```text
Possible data race during read of size 4 ... by thread #1
   at 0x...: main (main-race.c:15)
This conflicts with a previous write ... by thread #2
   at 0x...: worker (main-race.c:8)
Address ... is 0 bytes inside data symbol "balance"
ERROR SUMMARY: 2 errors
```

helgrind 精确指出 `balance` 在主线程第 15 行读、worker 第 8 行写，两者冲突。给 `balance++` 加锁后错误消失。

### 2. main-deadlock.c（死锁）

两个线程按**相反顺序**拿 m1、m2，可能互相等锁：

```shell
valgrind --tool=helgrind ./main-deadlock
```

```text
Thread #3: lock order "0x... before 0x..." violated
ERROR SUMMARY: 1 errors
```

helgrind 报"锁序违反"。`main-deadlock-global.c` 用一个全局大锁 `g` 先包住整个取锁顺序，再 `helgrind` 就不报错——代价是并发度下降。

### 3. main-signal.c vs main-signal-cv.c（自旋 vs 条件变量）

`main-signal.c` 用 `while (done == 0);` 忙等；`main-signal-cv.c` 用条件变量：

```shell
valgrind --tool=helgrind ./main-signal      # 自旋
valgrind --tool=helgrind ./main-signal-cv   # 条件变量
```

```text
main-signal:    Possible data race ... "done"   ERROR SUMMARY: 1 errors
main-signal-cv:                                  ERROR SUMMARY: 0 errors
```

自旋版本对 `done` 的读/写无同步，helgrind 报竞态（还烧 CPU）；条件变量版本 0 错误——锁 + `cond_wait` 既省 CPU 又正确。

### 4. 组合练习

自己写个"创建线程 + 锁 + 条件变量"的小程序（如生产者-消费者），跑 `helgrind` 确认 0 错误。