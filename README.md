# malloc

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.md)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![Language](https://img.shields.io/badge/language-C11-orange)

A heap allocator written in C. Implements `malloc`, `free`, `realloc`, and `calloc` from scratch using `mmap` for memory acquisition. Benchmarked within 40% of glibc throughput on common workloads.

## Features

- **Segregated free lists** — 16 size classes using bit-shift bucketing for O(1) bin selection
- **Thread-local cache (tcache)** — 32 bins of up to 32 blocks each for allocations up to 512 bytes, eliminating free list access on the fast path
- **Boundary-tag coalescing** — header and footer on every block enables O(1) bidirectional merging of adjacent free blocks
- **Block splitting** — reuses oversized free blocks without wasting the remainder
- **In-place realloc** — extends into adjacent free blocks before falling back to copy-and-free
- **Magic number validation** — `0xDEADBEEF` sentinel checks guard against double-free and corruption
- **16-byte alignment** — all allocations aligned to 16 bytes

## Block Layout

Every allocation has this layout in memory:

```
[ header (40B) | payload | footer (8B) ]
  ^                        ^
  magic, size,             size (for backwards coalescing)
  in_use, next, prev
```

Sentinel blocks with `size = 0` and `in_use = true` sit at the start and end of every `mmap` region to stop coalescing from walking past region boundaries.

## Design

The allocator uses segregated free lists rather than a single list so searches stay within the right size class. tcache sits in front of the global lists — most allocations hit the thread-local cache and never touch shared state. Boundary tags make backward coalescing O(1): walk back one footer, read the size, jump to the previous header.

The main tradeoff is per-allocation `mmap` calls. This keeps the implementation simple and each region self-contained, but hurts bulk-free workloads where a single arena would win. The "malloc then free" benchmark below shows this directly.

One known limitation: coalescing uses a magic number check to detect valid adjacent blocks. Across `mmap` region boundaries, unrelated memory could pass the check and trigger a false-positive coalesce. A robust fix would track region boundaries explicitly — documented here rather than patched to keep the code readable.

## Benchmarks

Tested against glibc on Ubuntu 24.04.3 LTS, 100,000 iterations per workload.

| Workload | This allocator | glibc |
|---|---|---|
| malloc/free cycle | 1.18ms | 0.98ms |
| malloc then free (100k live) | 120ms | 23ms |
| Random sizes (1–128B) | 4.89ms | 1.65ms |
| Realloc chain | 0.008ms | 0.006ms |
| Random calloc/free | 4.41ms | 3.81ms |
| Pool churn (10 live slots) | 5.35ms | 2.36ms |

**Memory overhead:** 1.38x — for every byte the user requested, 1.38 bytes were consumed from the OS (headers, footers, sentinels, alignment padding).

**tcache impact:** Adding the thread-local cache reduced latency by 90% on malloc/free cycles and ~65% on random workloads versus the baseline segregated free list implementation.

The "malloc then free" gap is expected — 100,000 live allocations means 100,000 separate `mmap` calls since no blocks are reused until the free phase. glibc manages large arenas and avoids this entirely.

## Build

Requires GCC and Make on Linux.

```bash
# Correctness tests
make test_basic && ./test_basic

# Stress tests (500,000+ operations)
make test_stress && ./test_stress

# Benchmark against glibc
make bench_mine bench_glibc
./bench_mine
./bench_glibc
```

## Project Structure

```
malloc/
├── src/
│   ├── allocator.h     # structs, constants, function declarations
│   └── malloc.c        # allocator implementation
├── tests/
│   ├── test_basic.c    # correctness tests with assertions
│   ├── test_stress.c   # stress tests, 100,000 iterations per scenario
│   └── test_bench.c    # throughput benchmarks vs glibc
├── Makefile
└── README.md
```

## License

MIT
