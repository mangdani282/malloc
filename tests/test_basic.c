#include "src/allocator.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    // Create string using malloc
    printf("String:\n");
    size_t str_size = sizeof(char) * 128;
    char *str = my_malloc(str_size);
    assert(str != NULL);
    strcpy(str, "Hello, world!");
    assert(strcmp(str, "Hello, world!") == 0);
    printf("%s\n", str);

    // Check header block
    block_header_t *header = (void*)str - HEADER_SIZE;
    assert(header->size == str_size);
    assert(header->in_use);
    printf("Size: %lu, In Use: %d, Next: %p\n\n", header->size, header->in_use, header->next);


    // Create int array using malloc
    printf("Int array:\n");
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
    header = (void*)arr - HEADER_SIZE;
    assert(header->size == arr_size);
    assert(header->in_use);
    printf("Size: %lu, In Use: %d, Next: %p\n\n", header->size, header->in_use, header->next);


    // Create struct using malloc
    printf("Struct:\n");
    struct Car {
        char* make;
        char* model;
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
    header = (void*)car - HEADER_SIZE;
    assert(header->size == struct_size);
    assert(header->in_use);
    printf("Size: %lu, In Use: %d, Next: %p\n", header->size, header->in_use, header->next);
}