CC = gcc
CFLAGS = -Wall -Wextra -g

test_basic: src/malloc.c tests/test_basic.c
	$(CC) $(CFLAGS) src/malloc.c tests/test_basic.c -o test_basic

test_stress: src/malloc.c tests/test_stress.c
	$(CC) $(CFLAGS) src/malloc.c tests/test_stress.c -o test_stress

bench_mine: src/malloc.c tests/test_bench.c
	$(CC) $(CFLAGS) -I. src/malloc.c tests/test_bench.c -o bench_mine

bench_glibc: tests/test_bench.c
	$(CC) $(CFLAGS) -DUSE_SYSTEM_MALLOC -I. tests/test_bench.c -o bench_glibc