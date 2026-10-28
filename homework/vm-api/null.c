#include <stdio.h>

int main(void) {
    int *p = 0;  // 空指针
    *p = 1;      // 解引用 → 段错误
    return 0;
}