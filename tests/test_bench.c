#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef USE_SYSTEM_MALLOC
#define MALLOC malloc
#define FREE free
#define REALLOC realloc
#define CALLOC calloc
#else
#include "../src/allocator.h"
extern size_t total_user_bytes;
extern size_t total_os_bytes;
#define MALLOC my_malloc
#define FREE my_free
#define REALLOC my_realloc
#define CALLOC my_calloc
#endif

int main() {
    // malloc/free cycle
    size_t NUM_CYCLES = 1e5;
    size_t SIZE = 128;
    void *p = NULL;
    clock_t start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        p = MALLOC(SIZE);
        FREE(p);
    }
    double sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of malloc/free in %fs\n", NUM_CYCLES, sec);

    // malloc cycles -> free cycles
    void *p_arr[NUM_CYCLES];
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) p_arr[i] = MALLOC(SIZE);
    for (size_t i = 0; i < NUM_CYCLES; i++) FREE(p_arr[i]);
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of malloc followed by %lu cycles of free in %fs\n", NUM_CYCLES, NUM_CYCLES, sec);

    // malloc/free cycles with random size
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        size_t rand_size = (rand() % SIZE) + 1;
        p = MALLOC(rand_size);
        FREE(p);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of random malloc/free in %fs\n", NUM_CYCLES, sec);

    // realloc chain
    start = clock();
    p = NULL;
    for (size_t i = 0; i < NUM_CYCLES / 100; i++) {
        p = REALLOC(p, i + 1);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of reallocs in %fs\n", NUM_CYCLES, sec);

    // calloc/free cycles with random size
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        size_t rand_size = (rand() % SIZE) + 1;
        p = CALLOC(1, rand_size);
        FREE(p);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of random calloc/free in %fs\n", NUM_CYCLES, sec);

    // pool of allocations with random freeing and sizes
    size_t POOL_SIZE = 10;
    void *pool[10] = {NULL};
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        size_t ind = rand() % POOL_SIZE;
        if (pool[ind] != NULL) FREE(pool[ind]);
        size_t rand_size = (rand() % SIZE) + 1;
        pool[ind] = MALLOC(rand_size);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of random free/allocations in a pool in %fs\n", NUM_CYCLES, sec);

#ifndef USE_SYSTEM_MALLOC
    printf("Fragmentation ratio: %.2f\n", (double)total_os_bytes / total_user_bytes);
#endif
}