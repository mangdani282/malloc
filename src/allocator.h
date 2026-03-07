#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

typedef struct block_header {
    size_t size;
    bool in_use;
    struct block_header* next;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

#endif