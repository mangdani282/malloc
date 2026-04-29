#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "allocator.h"

#define MAGIC 0xDEADBEEF

size_t total_user_bytes = 0;
size_t total_os_bytes = 0;

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

void set_footer(block_header_t *header) {
    size_t *footer = (void *)header + HEADER_SIZE + header->size;
    *footer = header->size;
}

void add_to_free_list(block_header_t *header) {
    // Get relevant free list
    size_t ind = get_list_ind(header->size);

    // Add to free list
    header->in_use = false;
    header->next = free_lists[ind];
    header->prev = NULL;
    if (free_lists[ind] != NULL) free_lists[ind]->prev = header;
    free_lists[ind] = header;
}

void remove_from_free_list(block_header_t *header) {
    // Get relevant free list
    size_t ind = get_list_ind(header->size);

    // Remove from free list
    if (header == free_lists[ind]) free_lists[ind] = header->next;
    if (header->next != NULL) header->next->prev = header->prev;
    if (header->prev != NULL) header->prev->next = header->next;

    header->in_use = true;
    header->next = NULL;
    header->prev = NULL;
}

void *my_malloc(size_t size) {
    // Check for edge case
    if (size == 0) return NULL;

    // Get relevant free list
    size_t ind = get_list_ind(size);

    // Check free list
    for (block_header_t *p = free_lists[ind]; p != NULL; p = p->next) {
        if (p->size < size) continue;

        remove_from_free_list(p);

        // Split block if possible
        if (p->size > size + HEADER_SIZE + FOOTER_SIZE) {
            block_header_t *split = (void *)p + HEADER_SIZE + size + FOOTER_SIZE;
            split->magic = MAGIC;
            split->size = p->size - size - HEADER_SIZE - FOOTER_SIZE;

            add_to_free_list(split);

            // Change current block size
            p->size = size;

            set_footer(p);
            set_footer(split);
        }

        p->in_use = true;
        p->next = NULL;
        p->prev = NULL;

        total_user_bytes += HEADER_SIZE + p->size + FOOTER_SIZE;
        return (void *)p + HEADER_SIZE;
    }

    // If no block exists then ask system for more memory
    block_header_t *block = mmap(NULL, HEADER_SIZE * 3 + size + FOOTER_SIZE * 2, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) return NULL;
    total_os_bytes += HEADER_SIZE * 3 + size + FOOTER_SIZE * 2;

    // Place start sentinel block
    block_header_t *sentinel = block;
    sentinel->magic = MAGIC;
    sentinel->size = 0;
    sentinel->in_use = true;
    sentinel->next = NULL;
    sentinel->prev = NULL;
    set_footer(sentinel);
    // Place end sentinel block
    sentinel = (void *)block + HEADER_SIZE * 2 + size + FOOTER_SIZE * 2;
    sentinel->magic = MAGIC;
    sentinel->size = 0;
    sentinel->in_use = true;
    sentinel->next = NULL;
    sentinel->prev = NULL;

    // Set block position and fill variables
    block = (void *)block + HEADER_SIZE + FOOTER_SIZE;
    block->magic = MAGIC;
    block->size = size;
    block->in_use = true;
    block->next = NULL;
    block->prev = NULL;

    set_footer(block);

    total_user_bytes += HEADER_SIZE + block->size + FOOTER_SIZE;
    return (void *)block + HEADER_SIZE;
}

void my_free(void *ptr) {
    // Get header and mark it as free
    block_header_t *header = (void *)ptr - HEADER_SIZE;
    header->in_use = false;

    total_user_bytes -= HEADER_SIZE + header->size + FOOTER_SIZE;

    // Coalesce forwards
    block_header_t *next = (void *)header + HEADER_SIZE + header->size + FOOTER_SIZE;
    while (next->magic == MAGIC && !next->in_use) {
        remove_from_free_list(next);

        // Update header and footer sizes
        header->size += next->size + HEADER_SIZE + FOOTER_SIZE;
        set_footer(header);

        // Check next possible forward coalesce
        next = (void *)header + HEADER_SIZE + header->size + FOOTER_SIZE;
    }

    // Coalesce backwards
    size_t *prev_footer = (void *)header - FOOTER_SIZE;
    block_header_t *prev = (void *)prev_footer - *prev_footer - HEADER_SIZE;
    while (prev->magic == MAGIC && !prev->in_use) {
        remove_from_free_list(prev);

        // Update header and footer sizes
        prev->size += header->size + HEADER_SIZE + FOOTER_SIZE;
        set_footer(prev);

        // Change header variable being added to free_list
        header = prev;

        // Check next possible backwards coalesce
        prev_footer = (void *)header - FOOTER_SIZE;
        prev = (void *)prev_footer - *prev_footer - HEADER_SIZE;
    }

    add_to_free_list(header);
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