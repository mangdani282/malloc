#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/allocator.h"

// Get size of block given pointer to memory
size_t get_size(void *p) {
    block_header_t *header = (void *)p - HEADER_SIZE;
    return header->size;
}

int main() {
    printf("%p %p %p\n", free_lists[0], free_lists[1], free_lists[2]);
    // Create string using malloc
    printf("String:\n");
    size_t str_size = sizeof(char) * 32;
    char *str = my_malloc(str_size);
    assert(str != NULL);
    strcpy(str, "Hello, world!");
    assert(strcmp(str, "Hello, world!") == 0);
    printf("%s\n", str);
    // Check header block
    block_header_t *header = (void *)str - HEADER_SIZE;
    printf("Size: %lu, In Use: %d, Next: %p\n\n", header->size, header->in_use, header->next);
    assert(header->size >= str_size);
    assert((header->size % ALIGN) == 0);
    assert(header->in_use);
    assert(header->magic == MAGIC);
    my_free(str);

    // Create int array using malloc
    printf("Int Array:\n");
    size_t arr_size = sizeof(int) * 10;
    int *arr = my_malloc(arr_size);
    assert(arr != NULL);
    arr[0] = 0;
    arr[1] = 1;
    for (int i = 2; i < 10; i++) {
        arr[i] = arr[i - 2] + arr[i - 1];
    }
    assert(arr[9] == 34);
    for (int i = 0; i < 10; i++) printf("%d ", arr[i]);
    printf("\n");
    // Check header block
    header = (void *)arr - HEADER_SIZE;
    assert(header->size >= arr_size);
    assert(header->in_use);
    printf("Size: %lu, In Use: %d, Next: %p\n\n", header->size, header->in_use, header->next);
    my_free(arr);
    arr = NULL;

    // Create struct using malloc
    printf("Struct:\n");
    struct Car {
        char *make;
        char *model;
        int num_wheels;
    };
    size_t struct_size = sizeof(struct Car) * 10;
    struct Car *car = my_malloc(struct_size);
    assert(car != NULL);
    car->make = "Mazda";
    car->model = "MX-5";
    car->num_wheels = 4;
    assert(strcmp(car->make, "Mazda") == 0);
    assert(strcmp(car->model, "MX-5") == 0);
    assert(car->num_wheels == 4);
    printf("Make: %s, Model: %s, Number of Wheels: %d\n", car->make, car->model, car->num_wheels);
    // Check header block
    header = (void *)car - HEADER_SIZE;
    assert(header->size >= struct_size);
    assert(header->in_use);
    printf("Size: %lu, In Use: %d, Next: %p\n\n", header->size, header->in_use, header->next);
    my_free(car);
    car = NULL;

    // Check TCACHE
    printf("TCACHE:\n");
    size_t size = 8;
    void *p1 = my_malloc(size);
    my_free(p1);
    void *p2 = my_malloc(size);
    assert(get_size(p2) >= size);
    printf("p1: %p, p2: %p\n\n", p1, p2);
    my_free(p2);
    p1 = NULL;
    p2 = NULL;

    // Test reallloc
    printf("realloc():\n");
    size = 8;
    arr = my_malloc(size);
    arr[0] = 10;
    arr[1] = 20;
    printf("arr[0]: %d, arr[1]: %d\n", arr[0], arr[1]);
    assert(get_size(arr) >= size);
    arr = my_realloc(arr, size * 2);
    assert(get_size(arr) >= size * 2);
    assert(arr[0] == 10);
    assert(arr[1] == 20);
    printf("arr[0]: %d, arr[1]: %d\n\n", arr[0], arr[1]);
    my_free(arr);
    arr = NULL;

    // Test calloc
    printf("calloc():\n");
    char *char_arr = my_calloc(2, size / 2);
    assert(get_size(char_arr) >= size);
    printf("char_arr: %d", *char_arr);
    for (char *p = char_arr; p < char_arr + size; p++) {
        assert(!*p);
        printf(",%d", *p);
    }
    printf("\n\n");
    my_free(char_arr);

    // Test splitting
    printf("Splitting:\n");
    p1 = my_malloc(1024 * 2 + HEADER_SIZE + FOOTER_SIZE);
    my_free(p1);
    p1 = NULL;
    p1 = my_malloc(1024);
    p2 = my_malloc(1024);
    assert((void *)p1 + 1024 + FOOTER_SIZE + HEADER_SIZE == p2);
    printf("p1: %p, p2: %p\n\n", p1, p2);
    my_free(p1);
    my_free(p2);
    p1 = NULL;
    p2 = NULL;

    // Test free list
    printf("Segregated Free List:\n");
    p1 = my_malloc(1024);
    p2 = my_malloc(2048);
    void *p3 = my_malloc(4096);
    assert(get_size(p1) >= 1024);
    assert(get_size(p2) >= 2048);
    assert(get_size(p3) >= 4096);
    printf("p1: %p, p2: %p, p3: %p\n", p1, p2, p3);
    printf("p1 Size: %lu, p2 Size: %lu, p3 Size: %lu\n", get_size(p1), get_size(p2), get_size(p3));
    my_free(p1);
    my_free(p2);
    my_free(p3);
    for (size_t i = 0; i < NUM_FREE_LISTS; i++) {
        if (free_lists[i]) printf("free_lists[%lu]: %p, ", i, free_lists[i]);
    }
    printf("\n\n");
    p1 = NULL;
    p2 = NULL;
    p3 = NULL;

    // Check coalescing
    printf("Coalescing:\n");
    void *a = my_malloc(1024);
    void *b = my_malloc(1024);
    // Force coalescing by requesting large block
    void *c = my_malloc(2048);
    assert(c != NULL);
    printf("a: %p, b: %p, c: %p\n\n", a, b, c);
    my_free(a);
    my_free(b);
    my_free(c);
    a = NULL;
    b = NULL;
    c = NULL;
}