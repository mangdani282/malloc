#include <sys/mman.h>
#include "allocator.h"

block_header_t *free_list = NULL;

void *my_malloc(size_t size) {
    // Check free list
    block_header_t *prev = NULL;
    for (block_header_t *p = free_list; p != NULL; p = p->next) {
        if (p->size < size) {
            prev = p;
            continue;
        }

        if (p == free_list) free_list = p->next;
        else prev->next = p->next;
        
        p->in_use = true;
        p->next = NULL;
        return (void*)p + HEADER_SIZE;
    }

    // If no block exists then ask system for more memory
    block_header_t *block = mmap(NULL, HEADER_SIZE + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    block->size = size;
    block->in_use = true;
    block->next = NULL;

    return (void*) block + HEADER_SIZE;
}

void my_free(void *ptr) {
    //Get header and mark it as free
    block_header_t *header = (void*)ptr - HEADER_SIZE;
    header->in_use = false;

    // Add block back to free list
    header->next = free_list;
    free_list = header;
}

void *my_realloc(void *ptr, size_t size) {

}

void *my_calloc(size_t nmemb, size_t size) {
    
}