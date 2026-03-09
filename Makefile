CC = gcc
CFLAGS = -Wall -Wextra -g

malloc: src/malloc.c tests/test_basic.c
	$(CC) $(CFLAGS) src/malloc.c tests/test_basic.c -o malloc