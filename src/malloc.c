#include <sys/mman.h>
#include <allocator.h>

void *malloc(size_t size) {
    block_header_t *block = mmap(NULL, HEADER_SIZE + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    block->size = size;
    block->in_use = true;
    block->next = NULL;

    block = (void*) block + HEADER_SIZE;

    return block;
}

void free(void *ptr) {

}

void *realloc(void *ptr, size_t size) {

}

void *calloc(size_t nmemb, size_t size) {

}