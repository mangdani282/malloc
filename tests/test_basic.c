#include "../src/allocator.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    // Create string using malloc
    size_t str_size = sizeof(char) * 128;
    char *str = malloc(sizeof(char) * 128);
    assert(str != NULL);
    strcpy(str, "Hello, world!");
    assert(strcmp(str, "Hello, world!") == 0);
    printf("%s\n", str);

    // Check header block
    block_header_t *str_header = (void*)str - HEADER_SIZE;
    assert(str_header->size == str_size);
    assert(str_header->in_use);
    printf("Size: %u, In Use: %d, Next: %p\n", str_header->size, str_header->in_use, str_header->next);
}