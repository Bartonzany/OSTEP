# 9 - 内存API

这一章讲 UNIX/C 里的内存分配接口：怎么申请、怎么释放、常见的坑、以及这些函数底下靠什么撑起来。

## 一、两种内存

C 程序里有两类内存，管理方式完全不同。

### 1. 栈内存（自动内存）

由编译器隐式管理：函数一进来就给局部变量在栈上留位置，函数一返回就自动收回。生命周期绑定在函数调用上，存不了需要跨函数保留的数据。

```c
void func() {
    int x;   // 栈上分配，函数返回即释放
}
```

### 2. 堆内存（手动管理）

由程序员显式管理：自己 `malloc` 申请、自己 `free` 释放。灵活、适合长生命周期的数据，但责任大、容易出错。

```c
void func() {
    int *x = (int *)malloc(sizeof(int));   // 堆上分配
}
```

一个容易搞混的点：上面这段里，指针 `x` 本身待在栈上，它指向的那块内存才在堆上。

## 二、malloc 和 free

### 1. `malloc()`

原型 `void *malloc(size_t size)`，在 `<stdlib.h>` 里。向堆申请 `size` 字节的连续内存，成功返回起始地址，失败返回 `NULL`。

几个要点：

- `size` 用 `sizeof()` 来写，别手写数字，避免跨平台大小不一致：

  ```c
  double *d = (double *)malloc(sizeof(double));
  int *arr = (int *)malloc(10 * sizeof(int));
  ```

- 字符串要记得给结尾符 `\0` 留一位：`malloc(strlen(s) + 1)`（`strlen` 只算有效字符，不含 `\0`）。
- 返回值是 `void*`，要强转成目标类型（只是为了告诉编译器和读者，不影响正确性）。

### 2. `free()`

原型 `void free(void *ptr)`，释放 `malloc`/`calloc`/`realloc` 申请的内存。只能传这些函数返回的有效指针，不能传栈地址或非法地址；传 `NULL` 无害（没效果）。不用传大小，分配库自己记着每块的大小。

```c
int *x = (int *)malloc(10 * sizeof(int));
free(x);
x = NULL;   // 顺手置空，避免悬空指针
```

## 三、七个经典错误

C 的内存管理没有自动校验，下面这些错编译器一般不吭声，后果却从崩溃到安全漏洞都有：

| 错误 | 后果 | 示例代码 | 规避 |
|------|------|----------|------|
| 忘记分配 | 野指针写内存，段错误崩溃 | `char *src = "hello";`<br>`char *dst; // 未分配内存的野指针`<br>`strcpy(dst, src); // 触发段错误（segmentation fault）` | 先 `malloc` 分配足够空间，或用 `strdup` 直接复制 |
| 分配不足（缓冲区溢出） | 覆盖相邻内存，可能被利用为攻击 | `char *src = "hello";`<br>`char *dst = (char *)malloc(strlen(src)); // 仅分配5字节，未留\0空间`<br>`strcpy(dst, src); // 写入6字节（含\0），溢出堆内存` | 预留 `\0`：`malloc(strlen(s) + 1)` |
| 忘记初始化 | 读垃圾值，逻辑错且时好时坏 | `int *x = (int *)malloc(sizeof(int));`<br>`printf("%d", *x); // 随机值` | 分配后显式初始化，或用 `calloc` |
| 忘记释放（泄漏） | 长命程序内存逐渐耗尽 | `int *x = (int *)malloc(sizeof(int));`<br>`// 一直不 free，短命进程靠 OS 兜底，长命进程漏光` | 用完及时 `free` |
| 提前释放（悬空指针） | 访问已释放内存，行为未定义 | `int *x = (int *)malloc(sizeof(int));`<br>`free(x); // 释放后x成为悬空指针`<br>`printf("%d\n", *x); // 访问已释放内存，行为未定义` | 释放后置 `x = NULL` |
| 重复释放 | 分配库状态混乱 | `free(x);`<br>`free(x); // 第二次释放同一块` | 释放后置 `NULL`，避免二次 `free` |
| 错误调用 free | 崩溃或内存管理异常 | `free(&local); // 传栈地址等非法指针` | 只传 `malloc`/`calloc`/`realloc` 返回的指针 |

## 四、底层靠什么

`malloc`/`free` 不是系统调用，是库函数。库在底下靠两个系统调用管堆：

- **`brk` / `sbrk`**：调整程序的 break 位置（堆的末尾），让堆变大变小。程序员别自己调，否则会和分配库的管理逻辑打架。
- **`mmap`**：建一块"匿名内存区"（不关联文件，只关联交换空间），灵活、适合大块分配——上一章 pmap 里那个 200MB 的大 `malloc` 就是它干的。

## 五、其他接口

- **`calloc(nmemb, size)`**：申请 `nmemb` 个 `size` 字节并全部清零，适合数组/结构体，能避免"忘了初始化"。
- **`realloc(ptr, size)`**：调整已有块的大小，尽量原地扩，扩不开就搬走（返回新地址、旧块自动释放）；失败返回 `NULL` 且旧块仍有效。适合动态数组/向量。

## 六、查错工具与参考

- **valgrind（memcheck）**：`valgrind --leak-check=yes ./程序`，能抓泄漏、越界、悬空指针等绝大多数内存错。
- **gdb**：编译加 `-g`，调试时定位错误代码位置。
- **purify**：商业级检测工具。

参考：

- 书籍：《The C Programming Language》（C语言发明者著作）、《Advanced Programming in the UNIX Environment》（UNIX编程权威指南）。
- 论文：《Exterminator: Automatically Correcting Memory Errors with High Probability》（自动检测和修复内存错误）、《Survey on Buffer Overflow Attacks and Countermeasures》（缓冲区溢出攻击与防御综述）。

## 七、实践作业

通过编写含内存错误的程序并使用工具检测，加深对内存管理的理解：
1. 编写空指针解引用、内存泄漏、缓冲区溢出、悬空指针等错误程序。
2. 使用`gdb`（带符号编译）调试程序，定位错误位置。
3. 使用`valgrind`检测内存错误，解读工具输出。
4. 实践`realloc`实现动态向量，对比其与链表的性能差异。
5. 熟练掌握`gdb`和`valgrind`，提升UNIX/C环境下的调试能力。

### 1. 内存泄漏

```c
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 10);
    p[0] = 1;
    return 0;   // 没 free
}
```

```shell
valgrind --leak-check=full --show-leak-kinds=all ./leak
```

```text
HEAP SUMMARY:
    in use at exit: 144 bytes in 3 blocks
  total heap usage: 3 allocs, 0 frees, 144 bytes allocated
    definitely lost: 40 bytes in 1 blocks
ERROR SUMMARY: 1 errors from 1 contexts
```

`definitely lost` 直接点名了那 40 字节（10 个 int）没被释放。

### 2. 缓冲区溢出

```c
char *src = "hello";
char *dst = malloc(strlen(src));   // 少算 \0
strcpy(dst, src);                  // 溢出 1 字节
```

```shell
valgrind ./overflow
```

```text
Invalid write of size 1
   Address 0x... is 0 bytes after a block of size 5 alloc'd
ERROR SUMMARY: 1 errors
```

`0 bytes after a block of size 5` 说明越界写了刚好 1 字节（漏掉的 `\0`）。

### 3. 悬空指针（use-after-free）

```c
int *x = malloc(sizeof(int));
*x = 42;
free(x);
printf("%d\n", *x);   // 释放后再读
```

```shell
valgrind ./useafterfree
```

```text
Invalid read of size 4
   Address 0x... is 0 bytes inside a block of size 4 free'd
ERROR SUMMARY: 1 errors
```

### 4. 空指针解引用 + gdb 定位

```c
int *p = 0;
*p = 1;   // 段错误
```

直接运行得到 `Segmentation fault (core dumped)`。用 gdb 定位崩溃点：

```shell
gcc -g -o null null.c
gdb -batch -ex run -ex bt ./null
```

```text
Program received signal SIGSEGV, Segmentation fault.
#0  0x0000aaaaaaaa0728 in main () at null.c:3
```

`bt`（backtrace）直接指出崩溃在 `null.c:3`。

### 5. realloc 实现动态向量

用 `realloc` 按需扩容，避免一开始分配过大：

```c
int *v = NULL;
size_t cap = 0, len = 0;
// 每次追加前：容量不够就翻倍
if (len == cap) {
    cap = cap ? cap * 2 : 4;
    v = realloc(v, cap * sizeof(int));
}
v[len++] = x;
```

对比链表：向量随机访问 O(1)、连续内存缓存友好，但扩容要整体搬移；链表插入删除 O(1)，但随机访问 O(n)、每元素有指针开销。

## 八、总结

1. 栈自动、堆手动；栈短命，堆灵活但易错。
2. `malloc` 申请、`free` 释放，配套 `calloc`（清零）和 `realloc`（调整大小）。
3. 那些常见错误（泄漏、溢出、悬空指针等）没有编译器兜底，靠好习惯 + valgrind。
4. `malloc` 库底下是 `brk`/`sbrk` 和 `mmap`，程序员不用直接碰系统调用。
