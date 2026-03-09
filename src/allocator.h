#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

typedef struct block_header {
    size_t size;
    bool in_use;
    struct block_header *next;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

extern block_header_t *free_list;

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);
void *my_calloc(size_t nmemb, size_t size);

#endif