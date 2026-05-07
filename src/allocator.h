#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

typedef struct block_header {
    size_t magic;
    size_t size;
    bool in_use;
    struct block_header *next, *prev;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)
#define FOOTER_SIZE sizeof(size_t)

#define ALIGN 16

#define TCACHE_MAX_SIZE 512
#define TCACHE_BINS (TCACHE_MAX_SIZE / ALIGN)
#define TCACHE_COUNT 32

typedef struct tcache_entry {
    struct tcache_entry *next;
} tcache_entry_t;

typedef struct tcache_bin {
    tcache_entry_t *head;
    size_t count;
} tcache_bin_t;

extern __thread tcache_bin_t tcache[TCACHE_BINS];

#define NUM_FREE_LISTS 16
extern block_header_t *free_lists[NUM_FREE_LISTS];

#define CHUNK_SIZE 65536

#define MAGIC 0xDEADBEEF

extern size_t total_os_bytes;
extern size_t total_user_bytes;
extern size_t peak_user_bytes;

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);
void *my_calloc(size_t nmemb, size_t size);

#endif