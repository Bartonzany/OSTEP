#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#include "main-header.h"
#include "vector-header.h"

// 原内联汇编在 64 位工具链下无法编译，改用等价的 GCC 内置 __sync_fetch_and_add
int fetch_and_add(int * variable, int value) {
    return __sync_fetch_and_add(variable, value);
}

void vector_add(vector_t *v_dst, vector_t *v_src) {
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
	fetch_and_add(&v_dst->values[i], v_src->values[i]);
    }
}

void fini() {}


#include "main-common.c"

