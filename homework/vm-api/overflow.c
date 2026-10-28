#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char *src = "hello";
    char *dst = malloc(strlen(src));  // 只算 5 个字符，没留 \0
    strcpy(dst, src);                 // 写入 6 字节（含 \0），溢出 1 字节
    printf("%s\n", dst);
    free(dst);
    return 0;
}