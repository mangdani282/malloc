#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "allocator.h"

block_header_t *free_list = NULL;

void *my_malloc(size_t size) {
    // Check for edge case
    if (size == 0) return NULL;

    // Check free list
    block_header_t *prev = NULL;
    for (block_header_t *p = free_list; p != NULL; p = p->next) {
        if (p->size < size) {
            prev = p;
            continue;
        }

        if (p == free_list)
            free_list = p->next;
        else
            prev->next = p->next;

        p->in_use = true;
        p->next = NULL;
        return (void *)p + HEADER_SIZE;
    }

    // If no block exists then ask system for more memory
    block_header_t *block = mmap(NULL, HEADER_SIZE + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    block->size = size;
    block->in_use = true;
    block->next = NULL;

    return (void *)block + HEADER_SIZE;
}

void my_free(void *ptr) {
    // Get header and mark it as free
    block_header_t *header = (void *)ptr - HEADER_SIZE;
    header->in_use = false;

    // Add block back to free list
    header->next = free_list;
    free_list = header;
}

void *my_realloc(void *ptr, size_t size) {
    // Check edge case
    if (ptr == NULL) return my_malloc(size);
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    // Create new block of memory
    void *new_ptr = my_malloc(size);
    block_header_t *old_header = (void *)ptr - HEADER_SIZE, *new_header = (void *)new_ptr - HEADER_SIZE;

    // Move memory over
    size_t move_size = (old_header->size < new_header->size) ? old_header->size : new_header->size;
    memcpy(new_ptr, ptr, move_size);

    // Free old block
    my_free(ptr);

    return new_ptr;
}

void *my_calloc(size_t nmemb, size_t size) {
    // Handle overflow case
    if (size != 0 && nmemb > SIZE_MAX / size) return NULL;

    // Create block
    size_t total_size = nmemb * size;
    void *ptr = my_malloc(total_size);

    // Fill with zeroes
    memset(ptr, 0, total_size);

    return ptr;
}