#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

#define MEMORY_POOL_SIZE 10240 // Example size of the memory pool (10 KB)

static char *memory_pool = NULL; // Pointer to the memory pool
static size_t pool_size = 0;      // Size of the memory pool
static char *free_list;            // Pointer to the start of the free memory list

typedef struct Block {
    size_t size;                    // Size of the block
    struct Block *next;             // Pointer to the next block
} Block;

// Initialize the memory manager
void mem_init(size_t size) {
    pool_size = size;
    memory_pool = (char *)malloc(pool_size);
    if (!memory_pool) {
        fprintf(stderr, "Failed to allocate memory pool\n");
        exit(EXIT_FAILURE);
    }
    free_list = memory_pool; // Initially, the whole pool is free

    // Set the initial block size
    Block *initial_block = (Block *)free_list;
    initial_block->size = pool_size - sizeof(Block);
    initial_block->next = NULL;
}

// Allocate a block of memory
void *mem_alloc(size_t size) {
    Block *current = (Block *)free_list;
    Block *prev = NULL;

    // Align size to the nearest multiple of sizeof(size_t)
    size_t aligned_size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
    aligned_size += sizeof(Block); // Account for the block header

    // Search for a suitable block
    while (current) {
        if (current->size >= aligned_size) {
            // Split the block if there's enough space left
            if (current->size > aligned_size + sizeof(Block)) {
                Block *new_block = (Block *)((char *)current + aligned_size);
                new_block->size = current->size - aligned_size;
                new_block->next = current->next;
                current->next = new_block;
                current->size = aligned_size;
            }

            // Remove the block from the free list
            if (prev) {
                prev->next = current->next;
            } else {
                free_list = current->next;
            }
            return (char *)current + sizeof(Block);
        }
        prev = current;
        current = current->next;
    }
    return NULL; // No suitable block found
}

// Free the allocated block of memory
void mem_free(void *block) {
    if (!block) return;

    Block *returned_block = (Block *)((char *)block - sizeof(Block));
    returned_block->next = (Block *)free_list; // Add the block to the free list
    free_list = (char *)returned_block;         // Update the free list pointer
}

// Resize the allocated memory block
void *mem_resize(void *block, size_t size) {
    if (!block) return mem_alloc(size);
    if (size == 0) {
        mem_free(block);
        return NULL;
    }

    Block *old_block = (Block *)((char *)block - sizeof(Block));
    size_t old_size = old_block->size - sizeof(Block);
    if (size <= old_size) return block; // No need to resize

    void *new_block = mem_alloc(size);
    if (new_block) {
        memcpy(new_block, block, old_size);
        mem_free(block);
    }
    return new_block;
}

// Free the entire memory pool
void mem_deinit() {
    free(memory_pool);
    memory_pool = NULL;
    pool_size = 0;
    free_list = NULL;
}
