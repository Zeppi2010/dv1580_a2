#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "memory_manager.h"

#define MEMORY_POOL_SIZE 5000 // Define the memory pool size

static unsigned char *memory_pool = NULL; // Pointer to the allocated memory pool
static size_t pool_size = 0; // Size of the memory pool
static size_t next_free = 0; // Pointer to the next free location in the pool

void mem_init(size_t size) {
    if (memory_pool != NULL) {
        fprintf(stderr, "Memory pool is already initialized.\n");
        return;
    }

    // Allocate memory using mmap
    memory_pool = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_pool == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    memset(memory_pool, 0, size); // Initialize memory to zero
    pool_size = size;
    next_free = 0; // Start allocation from the beginning
}

void mem_deinit(void) {
    if (memory_pool == NULL) {
        fprintf(stderr, "Memory pool is not initialized.\n");
        return;
    }

    munmap(memory_pool, pool_size);
    memory_pool = NULL; // Reset the pointer
}

void* mem_alloc(size_t size) {
    if (memory_pool == NULL) {
        fprintf(stderr, "Memory pool is not initialized.\n");
        return NULL;
    }

    // Align size to a multiple of 8 for better memory alignment
    size = (size + 7) & ~7;

    // Check if there's enough space
    if (next_free + size > pool_size) {
        fprintf(stderr, "Memory allocation failed: not enough space.\n");
        return NULL;
    }

    // Allocate the memory
    void* block = memory_pool + next_free;
    next_free += size; // Move the pointer for the next allocation
    return block;
}

void mem_free(void* ptr) {
    // In this simple implementation, we do not support free operation,
    // since we are not keeping track of allocated blocks.
    // You can extend this to manage a free list if needed.
}

void* mem_resize(void* ptr, size_t new_size) {
    // For simplicity, resizing is not implemented in this version.
    return NULL;
}
