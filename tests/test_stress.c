#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/allocator.h"

int main() {
    // malloc/free cycle
    size_t NUM_CYCLES = 1e5;
    size_t SIZE = 128;
    void *p = NULL;
    block_header_t *header;
    size_t *footer;
    clock_t start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        p = my_malloc(SIZE);
        assert(p != NULL);
        header = p - HEADER_SIZE;
        footer = p + SIZE;
        assert(header != NULL);
        assert(header->size == SIZE);
        assert(header->in_use);
        assert(header->next == NULL);
        assert(header->prev == NULL);
        assert(footer != NULL);
        assert(*footer == SIZE);
        my_free(p);
        assert(header != NULL);
        assert(header->size == SIZE);
        assert(footer != NULL);
        assert(*footer == SIZE);
    }
    double sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of malloc/free in %fs\n", NUM_CYCLES, sec);

    // malloc cycles -> free cycles
    void *p_arr[NUM_CYCLES];
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        p_arr[i] = my_malloc(SIZE);
        assert(p_arr[i] != NULL);
        header = p_arr[i] - HEADER_SIZE;
        footer = p_arr[i] + SIZE;
        assert(header != NULL);
        assert(header->size == SIZE);
        assert(header->in_use);
        assert(header->next == NULL);
        assert(header->prev == NULL);
        assert(footer != NULL);
        assert(*footer == SIZE);
    }
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        header = p_arr[i] - HEADER_SIZE;
        footer = p_arr[i] + SIZE;
        my_free(p_arr[i]);
        assert(header != NULL);
        assert(header->size == SIZE);
        assert(footer != NULL);
        assert(*footer == SIZE);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of malloc followed by %lu cycles of free in %fs\n", NUM_CYCLES, NUM_CYCLES, sec);

    // malloc/free cycles with random size
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        size_t rand_size = (rand() % SIZE) + 1;
        p = my_malloc(rand_size);
        assert(p != NULL);
        header = p - HEADER_SIZE;
        footer = p + header->size;
        assert(header != NULL);
        assert(header->size >= rand_size);
        assert(header->in_use);
        assert(header->next == NULL);
        assert(header->prev == NULL);
        assert(footer != NULL);
        assert(*footer >= rand_size);
        my_free(p);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of random malloc/free in %fs\n", NUM_CYCLES, sec);

    // realloc chain
    start = clock();
    p = NULL;
    for (size_t i = 0; i < NUM_CYCLES / 100; i++) {
        p = my_realloc(p, i + 1);
        ((char *)p)[i] = (char)i;
        assert(p != NULL);
        header = p - HEADER_SIZE;
        footer = p + header->size;
        assert(header != NULL);
        assert(header->size >= i);
        assert(header->in_use);
        assert(header->next == NULL);
        assert(header->prev == NULL);
        assert(footer != NULL);
        assert(*footer >= i);
    }
    for (char *j = p; j < (char *)p + NUM_CYCLES / 100; j++) assert(*j == (char)(j - (char *)p));
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of reallocs in %fs\n", NUM_CYCLES, sec);

    // calloc/free cycles with random size
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        size_t rand_size = (rand() % SIZE) + 1;
        p = my_calloc(1, rand_size);
        assert(p != NULL);
        header = p - HEADER_SIZE;
        footer = p + header->size;
        assert(header != NULL);
        assert(header->size >= rand_size);
        assert(header->in_use);
        assert(header->next == NULL);
        assert(header->prev == NULL);
        assert(footer != NULL);
        assert(*footer >= rand_size);
        for (char *j = p; j < (char *)p + rand_size; j++) assert(*j == 0);
        my_free(p);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of random calloc/free in %fs\n", NUM_CYCLES, sec);

    // pool of allocations with random freeing and sizes
    size_t POOL_SIZE = 10;
    void *pool[10] = {NULL};
    start = clock();
    for (size_t i = 0; i < NUM_CYCLES; i++) {
        size_t ind = rand() % POOL_SIZE;
        if (pool[ind] != NULL) my_free(pool[ind]);
        size_t rand_size = (rand() % SIZE) + 1;
        pool[ind] = my_malloc(rand_size);
        assert(pool[ind] != NULL);
        header = pool[ind] - HEADER_SIZE;
        footer = pool[ind] + header->size;
        assert(header != NULL);
        assert(header->size >= rand_size);
        assert(header->in_use);
        assert(header->next == NULL);
        assert(header->prev == NULL);
        assert(footer != NULL);
        assert(*footer >= rand_size);
    }
    sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Completed %lu cycles of random free/allocations in a pool in %fs\n", NUM_CYCLES, sec);
}