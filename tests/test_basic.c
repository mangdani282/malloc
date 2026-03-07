#include "../src/allocator.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    // Create string using malloc
    char *str = malloc(sizeof(char) * 128);
    assert(str != NULL);
    strcpy(str, "Hello, world!");
    printf("%s\n", str);

    // Check header block
    block_header_t *str_header = (void*)str - HEADER_SIZE;
    printf("Size: %u, In Use: %d, Next: %p\n", str_header->size, str_header->in_use, str_header->next);
}