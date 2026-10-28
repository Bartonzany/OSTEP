#include <stdlib.h>

int main(void) {
    int *p = malloc(sizeof(int) * 10);  // 申请 40 字节
    p[0] = 1;
    return 0;  // 没 free，泄漏 40 字节
}