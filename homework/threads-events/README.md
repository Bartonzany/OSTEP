# threads-events：事件驱动并发

对应 OSTEP 的基于事件的并发（Event-based Concurrency）作业。目录里实现了两种并发服务器（事件驱动 `select()` 版、每连接一个线程版）、一个异步 I/O 演示，以及一个压测脚本。

## 文件

| 文件 | 说明 |
|------|------|
| `server-select.c` | 事件驱动版：单线程 + `select()` 多路复用，回时间、文件服务、`SIGUSR1` 清缓存 |
| `server-thread.c` | 线程版：每连接一个 `pthread`，和事件驱动版对照性能 |
| `aio-demo.c` | 异步读文件演示，`aio_read`/`aio_error`/`aio_return` |
| `bench.py` | 并发压测脚本，测吞吐量 |

## 编译

```shell
gcc -o server-select server-select.c -Wall
gcc -o server-thread server-thread.c -Wall -pthread
gcc -o aio-demo     aio-demo.c     -Wall -lrt
```

## 服务器协议与运行

两个服务器协议一致：客户端连上来发一行请求，服务器处理完**立即关闭连接**（一请求一连接）。

- `TIME` → 返回当前时间；
- `FILE <路径>` → 返回该文件内容；
- 其他 → 返回 `unknown request`。

启动（默认端口 10000，可带一个参数改端口）：

```shell
./server-select 10086
```

```text
server listening on port 10086
```

客户端示例（用 python 发请求）：

```shell
python3 -c "import socket; s=socket.create_connection(('127.0.0.1',10086)); s.sendall(b'TIME'); print(s.recv(200).decode())"
```

```text
Tue Oct 27 21:12:07 2026
```

```shell
python3 -c "import socket; s=socket.create_connection(('127.0.0.1',10086)); s.sendall(b'FILE /etc/hostname'); print(s.recv(200).decode())"
```

## 信号处理（仅 server-select）

给服务器发 `SIGUSR1`，事件循环的 `select()` 会返回 `EINTR`，循环顶部 `continue` 后打印清缓存、继续服务：

```shell
kill -USR1 <server pid>
```

```text
[signal] cache cleared
```

## 异步读文件（aio-demo）

```shell
./aio-demo <文件路径>
```

`aio_read` 启动后立即返回，程序轮询 `aio_error`（`EINPROGRESS` 表示没读完）直到完成：

```text
(reading asynchronously...)
read 24 bytes:
hello from file service
```

## 压测（bench.py）

`bench.py <port> <总请求数> <并发数>`，每个请求单独建连接、发 `TIME`、收响应：

```shell
./server-select 10092 &          # 事件驱动版
python3 bench.py 10092 2000 50
./server-thread 10093 &          # 线程版
python3 bench.py 10093 2000 50
```

```text
事件驱动 select 版: 2000/2000 requests in 0.13s = 14983 req/s
线程版:            2000/2000 requests in 0.14s = 14354 req/s
```

`TIME` 这种几乎不干活的请求两者吞吐差不多；差距要在请求变重时才拉开——线程版一个线程阻塞不影响别人，事件驱动版一旦在处理器里做了阻塞调用整个循环就卡死（这就是任务 4 要换异步 I/O 的原因）。