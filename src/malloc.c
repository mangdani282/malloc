#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "allocator.h"

size_t total_os_bytes = 0;
size_t total_user_bytes = 0;
size_t peak_user_bytes = 0;

__thread tcache_bin_t tcache[TCACHE_BINS];

block_header_t *free_lists[NUM_FREE_LISTS];

size_t get_list_ind(size_t size) {
    size_t ind = 0;
    while (size > ALIGN && ind < NUM_FREE_LISTS - 1) {
        size >>= 1;
        ind++;
    }
    return ind;
}

size_t get_tcache_ind(size_t size) { return (size / ALIGN) - 1; }

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

void split_block(block_header_t *header, size_t size) {
    if (header->size - size > HEADER_SIZE + FOOTER_SIZE) {
        block_header_t *split = (void *)header + HEADER_SIZE + size + FOOTER_SIZE;
        split->magic = MAGIC;
        split->size = header->size - size - HEADER_SIZE - FOOTER_SIZE;
        split->in_use = false;
        split->next = NULL;
        split->prev = NULL;

        // Change current block size
        header->size = size;

        set_footer(header);
        set_footer(split);

        add_to_free_list(split);
    }
}

void *my_malloc(size_t size) {
    // Check for edge case
    if (size == 0) return NULL;

    // Align block sizes to multiples of ALIGN
    size = (size + (ALIGN - 1)) & ~(ALIGN - 1);

    // Check tcache first
    if (size <= TCACHE_MAX_SIZE) {
        size_t ind = get_tcache_ind(size);
        tcache_bin_t *bin = &tcache[ind];

        // If bin isn't empty
        if (bin->head) {
            // Remove entry from bin
            tcache_entry_t *entry = bin->head;
            bin->head = entry->next;
            bin->count--;

            block_header_t *header = (void *)entry - HEADER_SIZE;

            total_user_bytes += header->size;
            if (total_user_bytes > peak_user_bytes) peak_user_bytes = total_user_bytes;

            return (void *)header + HEADER_SIZE;
        }
    }

    // Get relevant free list
    size_t ind = get_list_ind(size);

    // Check free list
    for (int i = ind; i < NUM_FREE_LISTS; i++) {
        for (block_header_t *p = free_lists[i]; p != NULL; p = p->next) {
            if (p->size < size) continue;

            remove_from_free_list(p);

            // Split block if possible
            split_block(p, size);

            p->in_use = true;
            p->next = NULL;
            p->prev = NULL;

            total_user_bytes += p->size;
            if (total_user_bytes > peak_user_bytes) peak_user_bytes = total_user_bytes;

            return (void *)p + HEADER_SIZE;
        }
    }

    // If no block exists then ask system for more memory
    size_t alloc_size =
        (CHUNK_SIZE > HEADER_SIZE * 3 + size + FOOTER_SIZE * 2) ? CHUNK_SIZE : HEADER_SIZE * 3 + size + FOOTER_SIZE * 2;
    block_header_t *block = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) return NULL;
    total_os_bytes += alloc_size;

    // Place start sentinel block
    block_header_t *sentinel = block;
    sentinel->magic = MAGIC;
    sentinel->size = 0;
    sentinel->in_use = true;
    sentinel->next = NULL;
    sentinel->prev = NULL;
    set_footer(sentinel);
    // Place end sentinel block
    sentinel = (void *)block + alloc_size - HEADER_SIZE;
    sentinel->magic = MAGIC;
    sentinel->size = 0;
    sentinel->in_use = true;
    sentinel->next = NULL;
    sentinel->prev = NULL;

    // Set block position and fill variables
    block = (void *)block + HEADER_SIZE + FOOTER_SIZE;
    block->magic = MAGIC;
    block->size = alloc_size - HEADER_SIZE * 3 - FOOTER_SIZE * 2;
    block->in_use = true;
    block->next = NULL;
    block->prev = NULL;

    set_footer(block);

    split_block(block, size);

    total_user_bytes += block->size;
    if (total_user_bytes > peak_user_bytes) peak_user_bytes = total_user_bytes;

    return (void *)block + HEADER_SIZE;
}

void my_free(void *ptr) {
    // Check edge cases
    if (ptr == NULL) return;
    block_header_t *header = (void *)ptr - HEADER_SIZE;
    if (header->magic != MAGIC) return;

    total_user_bytes -= header->size;

    // Free to tcache if possible
    if (header->size <= TCACHE_MAX_SIZE) {
        size_t ind = get_tcache_ind(header->size);
        tcache_bin_t *bin = &tcache[ind];

        // If bin has room, add block to bin
        if (bin->count < TCACHE_COUNT) {
            tcache_entry_t *entry = (void *)header + HEADER_SIZE;
            entry->next = bin->head;
            bin->head = entry;
            bin->count++;

            return;
        }
    }

    header->in_use = false;

    // Defer coalescing for small blocks
    if (header->size <= TCACHE_MAX_SIZE) {
        add_to_free_list(header);
        return;
    }

    // Coalesce forwards
    block_header_t *next = (void *)header + HEADER_SIZE + header->size + FOOTER_SIZE;
    while (next->magic == MAGIC && !next->in_use) {
        remove_from_free_list(next);

        // Update header and footer sizes
        header->size += next->size + HEADER_SIZE + FOOTER_SIZE;
        set_footer(header);

        // Check next possible forward coalesce
        next = (void *)next + HEADER_SIZE + next->size + FOOTER_SIZE;
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

    // Align block sizes to multiples of ALIGN
    size = (size + (ALIGN - 1)) & ~(ALIGN - 1);

    block_header_t *header = (void *)ptr - HEADER_SIZE;
    if (header->size >= size) {
        // Split block if possible
        split_block(header, size);

        return ptr;
    }

    size_t old_size = header->size;

    // Try to realloc in-place
    block_header_t *next = (void *)header + HEADER_SIZE + header->size + FOOTER_SIZE;
    while (header->size < size && next->magic == MAGIC && !next->in_use) {
        remove_from_free_list(next);

        // Update header and footer sizes
        header->size += next->size + HEADER_SIZE + FOOTER_SIZE;
        set_footer(header);

        next = (void *)header + HEADER_SIZE + header->size + FOOTER_SIZE;
    }
    if (header->size >= size) {
        // Split block if possible
        split_block(header, size);
        return ptr;
    }

    // Create new block of memory
    void *new_ptr = my_malloc(size);
    if (new_ptr == NULL) return NULL;
    block_header_t *new_header = (void *)new_ptr - HEADER_SIZE;

    // Move memory over
    size_t move_size = (old_size < new_header->size) ? old_size : new_header->size;
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