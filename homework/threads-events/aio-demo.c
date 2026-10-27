#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <aio.h>

#define BUFSIZE 4096

// 异步读文件：aio_read 启动后立即返回，之后轮询 aio_error 直到完成
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    char buf[BUFSIZE];
    struct aiocb cb;
    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = BUFSIZE - 1;
    cb.aio_offset = 0;

    if (aio_read(&cb) != 0) { perror("aio_read"); return 1; }

    // 轮询：EINPROGRESS 表示还没读完
    while (aio_error(&cb) == EINPROGRESS) {
        printf("(reading asynchronously...)\n");
        usleep(100000);
    }

    ssize_t n = aio_return(&cb);
    if (n < 0) { perror("aio_return"); return 1; }
    buf[n] = '\0';

    printf("read %zd bytes:\n%s\n", n, buf);
    close(fd);
    return 0;
}
