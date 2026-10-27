#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFSIZE 4096

static void *client_handler(void *arg) {
    long fd = (long) arg;
    char buf[BUFSIZE];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) { close(fd); return NULL; }
    buf[n] = '\0';

    if (strncmp(buf, "TIME", 4) == 0) {
        time_t now = time(NULL);
        char *s = ctime(&now);
        write(fd, s, strlen(s));
    } else if (strncmp(buf, "FILE ", 5) == 0) {
        char *path = buf + 5;
        path[strcspn(path, "\r\n")] = '\0';
        FILE *f = fopen(path, "r");
        if (!f) { write(fd, "can't open file\n", 16); close(fd); return NULL; }
        char line[256];
        while (fgets(line, sizeof(line), f)) write(fd, line, strlen(line));
        fclose(f);
    } else {
        write(fd, "unknown request\n", 16);
    }
    close(fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 10000;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 64);

    printf("threaded server listening on port %d\n", port);

    while (1) {
        int cfd = accept(listen_fd, NULL, NULL);
        if (cfd < 0) continue;
        pthread_t t;
        pthread_create(&t, NULL, client_handler, (void *) (long) cfd);
        pthread_detach(t);   // 处理完自动回收，不 join
    }
    return 0;
}
