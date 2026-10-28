#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *x = malloc(sizeof(int));
    *x = 42;
    free(x);
    printf("%d\n", *x);  // 释放后再读：悬空指针（use-after-free）
    return 0;
}