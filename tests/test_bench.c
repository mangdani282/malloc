#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef USE_SYSTEM_MALLOC
#define MALLOC malloc
#define FREE free
#else
#include "../src/allocator.h"
#define MALLOC my_malloc
#define FREE my_free
#endif

int main() {
    // malloc/my_free cycle
    size_t NUM_CYCLES = 1e5;
    size_t SIZE = 128;
    void *p = NULL;
    clock_t start;
    double sec;
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        p = MALLOC(SIZE);
        FREE(p);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("%lu cycles of malloc/my_free in %fs\n", NUM_CYCLES, sec);
}