#include <stdio.h>
#include <stdlib.h>

// 用 realloc 实现动态向量：容量不够就翻倍
int main(void) {
    int *v = NULL;
    size_t cap = 0, len = 0;
    for (int x = 0; x < 100; x++) {
        if (len == cap) {
            cap = cap ? cap * 2 : 4;      // 初始 4，之后每次翻倍
            v = realloc(v, cap * sizeof(int));
        }
        v[len++] = x;
    }
    printf("len=%zu cap=%zu v[99]=%d\n", len, cap, v[99]);
    free(v);
    return 0;
}