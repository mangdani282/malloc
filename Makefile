CC = gcc
CFLAGS = -Wall -Wextra

malloc: src/malloc.c tests/test_basic.c
	$(CC) $(CFLAGS) src/malloc.c tests/test_basic.c -o malloc