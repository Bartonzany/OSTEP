#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 64
#define BUFSIZE 4096

static volatile sig_atomic_t clear_cache = 0;

static void on_signal(int sig) {
    clear_cache = 1;   // 只在信号处理器里置标志，不干重活
}

static void handle_request(int fd) {
    char buf[BUFSIZE];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) return;      // 连接关闭
    buf[n] = '\0';

    if (strncmp(buf, "TIME", 4) == 0) {
        time_t now = time(NULL);
        char *s = ctime(&now);
        write(fd, s, strlen(s));
    } else if (strncmp(buf, "FILE ", 5) == 0) {
        char *path = buf + 5;
        path[strcspn(path, "\r\n")] = '\0';
        FILE *f = fopen(path, "r");
        if (!f) { write(fd, "can't open file\n", 16); return; }
        char line[256];
        while (fgets(line, sizeof(line), f)) write(fd, line, strlen(line));
        fclose(f);
    } else {
        write(fd, "unknown request\n", 16);
    }
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
    listen(listen_fd, MAX_CLIENTS);

    signal(SIGUSR1, on_signal);

    int clients[MAX_CLIENTS];
    int nclients = 0;
    int i;

    printf("server listening on port %d\n", port);

    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);
        int maxfd = listen_fd;
        for (i = 0; i < nclients; i++) {
            FD_SET(clients[i], &rfds);
            if (clients[i] > maxfd) maxfd = clients[i];
        }

        if (clear_cache) {
            clear_cache = 0;
            printf("[signal] cache cleared\n");
        }

        int rc = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR) continue;   // 被信号打断，回到顶部继续
            perror("select");
            break;
        }

        // 新连接
        if (FD_ISSET(listen_fd, &rfds)) {
            int cfd = accept(listen_fd, NULL, NULL);
            if (cfd >= 0 && nclients < MAX_CLIENTS) {
                clients[nclients++] = cfd;
            }
        }
        // 已有连接发来数据
        for (i = 0; i < nclients; i++) {
            if (FD_ISSET(clients[i], &rfds)) {
                handle_request(clients[i]);
                close(clients[i]);
                clients[i] = clients[--nclients];   // 用完移除
                i--;
            }
        }
    }
    return 0;
}
