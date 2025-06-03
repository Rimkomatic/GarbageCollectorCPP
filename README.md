# Simple Garbage Collector in C++

This project implements a minimal garbage collector (GC) in C++, adapted from a classic C implementation. It provides custom heap allocation and basic mark-and-sweep garbage collection by scanning the stack and heap for reachable memory.

---

## Project Structure

```
.
├── gc.hpp        # The core garbage collector logic (C++ header)
├── tester.cpp    # A test program that uses the garbage collector
└── README.md     # Project documentation
```

---

## Overview

This implementation demonstrates how a garbage collector can be constructed from scratch in C++. It does the following:

- Allocates memory using a custom function `GC_malloc()`
- Tracks all allocations internally
- Scans stack and global memory for references
- Frees memory that is no longer in use (i.e., unreachable)

It is educational in nature and not intended for production use.

---

## How It Works

### 1. Custom Allocation

Memory is allocated using `GC_malloc(size)`. It does not use the C++ `new` keyword or `malloc` directly, but wraps low-level system calls such as `sbrk()` to manage heap memory manually.

```cpp
void* ptr = GC_malloc(128);
```

### 2. Memory Tracking

Allocated blocks are stored in a circular linked list (`usedp`). This list is traversed during garbage collection.

### 3. Mark Phase

The collector scans:

- The stack (from base pointer to system-reported stack bottom)
- The BSS and data segments (globals)
- The heap itself

It marks any memory that is still reachable based on pointer values found in those areas.

### 4. Sweep Phase

Any block not marked during the mark phase is considered garbage and returned to the free list for future allocation.

---

## Example: `tester.cpp`

```cpp
#include "gc.hpp"
#include <iostream>

int main() {
    GC_init();  // Initialize the garbage collector

    void* a = GC_malloc(64);
    void* b = GC_malloc(128);

    std::cout << "Allocated a at: " << a << std::endl;
    std::cout << "Allocated b at: " << b << std::endl;

    GC_collect(); // Run the garbage collector

    return 0;
}
```

This program allocates two memory blocks, prints their addresses, and then runs garbage collection.

---

## Compilation

**Requirements:**

- C++20 or later
- Linux (relies on `/proc/self/stat` and `sbrk()`)

### Build

```bash
g++ -std=c++20 tester.cpp -o tester
```

### Run

```bash
./tester
```

---

## Design Details

| Function           | Purpose                                                         |
|--------------------|------------------------------------------------------------------|
| `GC_init()`        | Initializes free list and determines stack limits                |
| `GC_malloc()`      | Allocates memory and adds it to the used list                    |
| `GC_collect()`     | Performs garbage collection (mark and sweep)                     |
| `scan_region()`    | Scans memory region for references to allocated memory           |
| `scan_heap()`      | Scans all live blocks for embedded references to other blocks    |
| `add_to_free_list()` | Frees and coalesces adjacent memory blocks                      |
| `morecore()`       | Requests more heap space from the OS using `sbrk()`              |

---

## Limitations

- Only works on Linux (requires `/proc/self/stat`)
- Single-threaded only
- Cannot handle C++ destructors or RAII patterns
- Conservative GC: may not free all unreachable memory
- Unsafe if the program uses `setjmp`, signals, or certain optimizations

---

## Suggested Improvements

- Add thread safety
- Integrate smart pointers or RAII-compatible wrappers
- Track memory type metadata
- Implement compacting or generational garbage collection
- Provide diagnostics or allocation tracking logs

---

## Educational Value

This project serves as a practical reference for:

- Understanding how manual memory allocation works
- Exploring low-level system interfaces for memory management
- Learning the internals of garbage collection algorithms
- Seeing how memory scanning and pointer tagging are used in practice

---

## References

- Hans Boehm's Garbage Collector: https://www.hboehm.info/gc/
- Linux `sbrk()` and memory layout documentation
- Systems programming textbooks and OS internals

---

## License

This project is intended for educational and research purposes only.
