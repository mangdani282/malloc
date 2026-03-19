#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "allocator.h"

block_header_t *free_lists[3];

size_t get_list_ind(size_t size) {
    // 0-16 bytes -> free_list[0]
    // 17-64 bytes -> free_list[1]
    // 65+ bytes -> free_list[2]
    if (size <= 16)
        return 0;
    else if (size <= 64)
        return 1;
    else
        return 2;
}

void *my_malloc(size_t size) {
    // Check for edge case
    if (size == 0) return NULL;

    // Get relevant free list
    size_t ind = get_list_ind(size);

    // Check free list
    block_header_t *prev = NULL;
    for (block_header_t *p = free_lists[ind]; p != NULL; p = p->next) {
        if (p->size < size) {
            prev = p;
            continue;
        }

        // Prepare for giving out block
        if (p == free_lists[ind])
            free_lists[ind] = p->next;
        else
            prev->next = p->next;

        // Split block if possible
        if (p->size > size + HEADER_SIZE) {
            block_header_t *split = (void *)p + HEADER_SIZE + size;
            split->size = p->size - size - HEADER_SIZE;
            split->in_use = false;

            // Add split block to corresponding free list
            size_t split_ind = get_list_ind(split->size);
            split->next = free_lists[split_ind];
            free_lists[split_ind] = split;

            // Change current block size
            p->size = size;
        }

        p->in_use = true;
        p->next = NULL;

        return (void *)p + HEADER_SIZE;
    }

    // If no block exists then ask system for more memory
    block_header_t *block = mmap(NULL, HEADER_SIZE + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) return NULL;

    block->size = size;
    block->in_use = true;
    block->next = NULL;

    return (void *)block + HEADER_SIZE;
}

void my_free(void *ptr) {
    // Get header and mark it as free
    block_header_t *header = (void *)ptr - HEADER_SIZE;
    header->in_use = false;

    // Get relevant free list
    size_t ind = get_list_ind(header->size);

    // Add block back to free list
    header->next = free_lists[ind];
    free_lists[ind] = header;
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
    if (new_ptr == NULL) return NULL;
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
    if (ptr == NULL) return NULL;

    // Fill with zeroes
    memset(ptr, 0, total_size);

    return ptr;
}