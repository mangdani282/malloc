CC = gcc
CFLAGS = -Wall -Wextra -g

test_basic: src/malloc.c tests/test_basic.c
	$(CC) $(CFLAGS) src/malloc.c tests/test_basic.c -o test_basic

test_stress: src/malloc.c tests/test_stress.c
	$(CC) $(CFLAGS) src/malloc.c tests/test_stress.c -o test_stress