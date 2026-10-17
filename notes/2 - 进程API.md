# 2 - 进程API

## 一、进程 API 概览

UNIX 提供一组进程操作接口，核心是 `fork()`、`exec()`、`wait()` 三个系统调用的组合。设计难点在于：接口既要功能强大、易用，又要保证高性能。理解这套接口，是掌握操作系统进程管理的关键。

## 二、`fork()`：创建子进程

`fork()` 用于创建新进程，行为很特别——创建出的子进程几乎是父进程的完整副本。

- 子进程拥有独立的地址空间、寄存器、程序计数器（PC），与父进程共享代码段，但数据段、堆、栈独立复制（用写时复制优化）。
- 返回值区分父子：父进程得到子进程的 PID，子进程得到 0，失败返回负数。
- 子进程不是从头执行 `main()`，而是从 `fork()` 调用处继续向下执行。
- 父子进程的执行顺序由 CPU 调度器决定，不确定（两种输出顺序都可能出现）。

### 示例（p1.c）

```shell
~/DevKit/Projects/OSTEP/codes/cpu-api main ?7 ❯ ./p1
hello world (pid:11712)
hello, I am parent of 11713 (pid:11712)
hello, I am child (pid:11713)
```

## 三、`wait()`：等待子进程结束

`wait()` 让父进程等待子进程终止，用来消除 `fork()` 输出顺序的不确定性。

- 父进程调用后阻塞，直到某个子进程终止（或收到信号）才返回该子进程的 PID。
- 若父进程先运行，`wait()` 会让它暂停，保证子进程输出先于父进程。

### 示例（p2.c）

```shell
~/DevKit/Projects/OSTEP/codes/cpu-api main ?7 ❯ ./p2
hello world (pid:11737)
hello, I am child (pid:11738)
hello, I am parent of 11738 (wc:11738) (pid:11737)
```

### 变体与场景

- `waitpid()` 可指定等待的子进程（按 PID）并设置选项（如非阻塞），更灵活。
- 适用于需要依赖子进程执行结果的场景。

## 四、`exec()`：替换进程映像

`exec()` 家族（如 `execvp()`）让进程运行一个全新的程序，而不是继续父进程的代码。

- 加载指定的可执行文件（如 `wc`、`ls`），覆盖当前进程的代码段、静态数据，并重新初始化堆、栈。
- 不创建新进程，只替换当前进程的运行内容；成功调用后不返回，失败才返回 -1。

### 示例（p3.c）

```shell
~/DevKit/Projects/OSTEP/codes/cpu-api main ?7 ❯ ./p3
hello world (pid:11971)
hello, I am child (pid:11972)
 30 120 982 p3.c
hello, I am parent of 11972 (wc:11972) (pid:11971)
```

### 家族变体

Linux 有 6 种变体（`execl`、`execlp`、`execle`、`execv`、`execvp`、`execvpe`），差异在参数传递方式（列表/数组）、是否搜索 PATH、是否传递环境变量。

## 五、`fork()` + `exec()` 分离的价值

UNIX 把"创建进程"和"运行新程序"拆成两个调用，而不是合并成一个 `spawn()`，这是它灵活性的来源：

- 中间执行空间：子进程在 `fork()` 之后、`exec()` 之前，可以先修改自身环境（IO 重定向、设置环境变量），再去运行新程序。
- 典型应用是 shell 的实现：打印提示符 → `fork()` 创建子进程 → 子进程在 `exec()` 前做 IO 重定向 → 子进程 `exec()` 运行命令 → 父进程 `wait()` 等待。

### IO 重定向（p4.c）

子进程关闭标准输出 `STDOUT_FILENO`，打开文件 `p4.output`，再 `execvp("wc", ...)`；`wc` 的输出写入 `p4.output` 而非屏幕（文件描述符在 `exec()` 后仍有效）。

```shell
 34 114 884 p4.c
```

### 管道（Pipe）

`pipe()` 在内核创建队列，把一个进程的 stdout 连到另一个进程的 stdin，实现命令链（如 `grep -o foo file | wc -l`），本质仍是 `fork()` + `exec()` 的分离设计。

## 六、进程控制与拓展

### 信号（Signal）

- 作用：向进程发送外部事件（终止、暂停、继续），是进程间通信的基础。
- 常用信号：`SIGINT`（Ctrl+C，终止）、`SIGTSTP`（Ctrl+Z，暂停）。
- 系统调用：`kill()` 发送信号，`signal()` 注册处理函数。

### 用户与权限

- 普通用户只能控制自己的进程（发信号、终止），防止误伤他人进程。
- root 可控制所有进程，需谨慎使用。

### 实用工具

- `ps`：查看进程列表（PID、状态等）。
- `top`：实时显示资源占用（CPU、内存）。
- `kill`/`killall`：向进程发信号（如 `kill -9 PID` 强制终止）。

## 七、作业方向（理论 + 实操）

### 1. 模拟作业（fork.py）

`fork.py` 用于练习"进程树"的生成与销毁。运行 `./fork.py -s 4` 会给出若干动作（`fork`/`exit`），让你推导每一步的进程树；加 `-c` 直接给出答案：

```shell
./fork.py -s 4 -c
```

```text
                           Process Tree:
                               a

Action: a forks b
                               a
                               └── b
Action: a forks c
                               a
                               ├── b
                               └── c
Action: b forks d
                               a
                               ├── b
                               │   └── d
                               └── c
Action: d EXITS
                               a
                               ├── b
                               └── c
Action: a forks e
                               a
                               ├── b
                               ├── c
                               └── e
```

也可用 `-A` 精确指定动作序列（`a+b` 表示 a fork b，`c-` 表示 c exit）：

```shell
./fork.py -A a+b,b+c,c-,a+d -c
```

```text
                           Process Tree:
                               a

Action: a forks b
                               a
                               └── b
Action: b forks c
                               a
                               └── b
                                   └── c
Action: c EXITS
                               a
                               └── b
Action: a forks d
                               a
                               ├── b
                               └── d
```

### 2. 编程作业（实操API）

通过编写 C 程序巩固 `fork()`、`wait()`、`exec()`、`pipe()` 的使用。以下为各题的执行结果。

**题 1：父子进程变量是否隔离**

`fork()` 前把 `x` 设为 100，父子进程各自修改 `x`：

```c
#include <stdio.h>
#include <unistd.h>
int main() {
    int x = 100;
    int rc = fork();
    if (rc == 0) { printf("child: x = %d -> ", x); x = 200; printf("%d\n", x); }
    else         { printf("parent: x = %d -> ", x); x = 300; printf("%d\n", x); }
    return 0;
}
```

```text
parent: x = 100 -> 300
child: x = 100 -> 200
```

结果：子进程看到的是 `fork()` 时刻的副本 100，父子各自修改互不影响——说明 `fork()` 后地址空间是独立复制的。

**题 2：父子进程是否共享文件描述符**

先 `open()` 文件再 `fork()`，父子进程都往里面写：

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {
    int fd = open("/tmp/hw2.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
    int rc = fork();
    if (rc == 0) { write(fd, "child\n", 6); write(fd, "child2\n", 7); }
    else         { write(fd, "parent1\n", 8); write(fd, "parent2\n", 8); wait(NULL); }
    close(fd);
    return 0;
}
```

```text
parent1
parent2
child
child2
```

结果：父子进程都能访问同一个文件描述符，写的内容都进了文件。两者共享同一个"打开文件描述"（含共享的文件偏移），所以不会互相覆盖，但写入顺序由调度决定（本运行恰好父进程先写完）。

**题 3：不靠 `wait()` 让子进程先打印**

子进程打印 "hello"，父进程打印 "goodbye"，父进程用 `sleep(1)` 让子进程先跑：

```c
#include <stdio.h>
#include <unistd.h>
int main() {
    int rc = fork();
    if (rc == 0) { printf("hello\n"); }
    else         { sleep(1); printf("goodbye\n"); }
    return 0;
}
```

```text
hello
goodbye
```

结果：`sleep()` 能"碰巧"保证顺序，但不严谨——更好的办法还是 `wait()` 这类同步原语。

**题 4：`exec()` 变体运行 `/bin/ls`**

子进程用 `execvp` 运行 `ls -l`：

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {
    int rc = fork();
    if (rc == 0) {
        char *args[] = { "/bin/ls", "-l", NULL };
        execvp(args[0], args);
        printf("execvp failed\n");
    } else { wait(NULL); }
    return 0;
}
```

```text
total 160
-rw-r--r-- 1 developer developer  269 q1.c
-rw-r--r-- 1 developer developer  371 q2.c
...（当前目录文件列表）
```

结果：`exec` 成功用 `ls` 替换了子进程，其后的 `printf` 不再执行。6 种变体差异仅在传参方式（列表/数组）和是否搜索 PATH、是否传环境变量。

**题 5：`wait()` 的返回值**

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {
    int rc = fork();
    if (rc == 0) {
        int wc = wait(NULL);
        printf("child: wait(NULL) returned %d\n", wc);
    } else {
        int wc = wait(NULL);
        printf("parent: wait(NULL) returned %d (=%d child pid)\n", wc, rc);
    }
    return 0;
}
```

```text
child: wait(NULL) returned -1
parent: wait(NULL) returned 2670177
```

结果：父进程 `wait()` 返回被回收的子进程 PID；子进程没有子进程，`wait()` 返回 -1（失败）。

**题 6：`waitpid()`**

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {
    int rc = fork();
    if (rc == 0) { printf("child pid = %d\n", getpid()); }
    else {
        int wc = waitpid(rc, NULL, 0);
        printf("parent: waitpid(%d) returned %d\n", rc, wc);
    }
    return 0;
}
```

```text
child pid = 2670178
parent: waitpid(2670178) returned 2670178
```

结果：`waitpid(pid, ...)` 可精确等待指定 PID 的子进程，适用于有多个子进程、只想等其中一个的场景。

**题 7：关闭 stdout 后再 `printf`**

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {
    int rc = fork();
    if (rc == 0) {
        close(STDOUT_FILENO);
        printf("(child) this printf goes nowhere\n");
    } else {
        wait(NULL);
        printf("(parent) done\n");
    }
    return 0;
}
```

```text
(parent) done
```

结果：子进程关闭了 `STDOUT_FILENO` 后，`printf` 写不到任何地方，所以屏幕上只有父进程的输出（对应 p4.c 的重定向思路）。

**题 8：两个子进程的管道通信**

第一个子进程把 stdout 接到管道写端，第二个子进程把 stdin 接到管道读端：

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
int main() {
    int fd[2];
    pipe(fd);
    int rc1 = fork();
    if (rc1 == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        printf("hello from child1\n");
        close(fd[1]);
        exit(0);
    }
    int rc2 = fork();
    if (rc2 == 0) {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        char buf[256];
        fgets(buf, sizeof(buf), stdin);
        printf("child2 received: %s", buf);
        close(fd[0]);
        exit(0);
    }
    close(fd[0]); close(fd[1]);
    wait(NULL); wait(NULL);
    return 0;
}
```

```text
child2 received: hello from child1
```

结果：`pipe()` 在内核建立队列，一个进程写、另一个进程读，实现进程间单向数据流。

**配套工具 `generator.py`**

`generator.py` 随机生成由 `fork()`/`wait()`/`exit()` 构成的 C 程序，用于练习预测 `fork` 的执行结果：

```shell
./generator.py -n 1 -s 0    # 生成随机 C 程序
./generator.py -n 1 -s 0 -c # 附带答案时间轴
```

生成的 C 代码（节选）：

```c
int main(int argc, char *argv[]) {
    // process a
    if (fork_or_die() == 0) {
        sleep(6);
        // process b
        exit(0);
    }
    wait_or_die();
    return 0;
}
```

答案时间轴（`a+` 启动、`a->b` 表示 fork、`b+`/`b-` 表示 b 启动/退出、`a<-b` 表示 `wait()` 返回）：

```text
  0 a+
  0 a->b
  6      b+
  6      b-
  6 a<-b
```

常用参数：`-s` 随机种子、`-n` 动作数量、`-f`/`-w`/`-e` 控制 fork/wait/exit 概率、`-A "fork b,1 {} wait"` 精确指定动作序列（`{}` 内为子进程执行的动作）。

## 八、总结

1. UNIX 进程 API 的核心是 `fork()`（创建副本）、`wait()`（同步）、`exec()`（替换程序）的组合，分离设计带来极强灵活性。
2. 核心价值：支持 shell 的 IO 重定向、管道等功能，无需修改被运行程序的代码。
3. `fork()` 复制、`exec()` 替换、`wait()` 同步，三者协同实现进程的创建与控制。
4. 拓展：信号机制做事件通知，用户权限保证安全，工具链提升管理效率。
