#include <sys/mman.h> // Required for mmap
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

#define MEMORY_POOL_SIZE 5000 // Example size of the memory pool (adjust as needed)

static char *memory_pool = NULL; // Pointer to the memory pool
static size_t pool_size = 0;      // Size of the memory pool
static char *free_list = NULL;     // Pointer to the start of the free memory list

typedef struct Block {
    size_t size;                    // Size of the block
    struct Block *next;             // Pointer to the next block
} Block;

// Initialize the memory manager
void mem_init(size_t size) {
    pool_size = size;
    memory_pool = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_pool == MAP_FAILED) {
        fprintf(stderr, "Failed to allocate memory pool with mmap\n");
        exit(EXIT_FAILURE);
    }

    // Set up the initial block
    free_list = memory_pool; // Initially, the whole pool is free
    Block *initial_block = (Block *)free_list;
    initial_block->size = pool_size - sizeof(Block); // Set size of the block
    initial_block->next = NULL; // No next block yet
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
                new_block->size = current->size - aligned_size - sizeof(Block);
                new_block->next = current->next;
                current->next = new_block;
                current->size = aligned_size - sizeof(Block);
            }

            // Remove the block from the free list
            if (prev) {
                prev->next = current->next;
            } else {
                free_list = current->next; // Update free list head
            }
            return (char *)current + sizeof(Block); // Return pointer to usable memory
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
    
    // Insert returned block into the free list
    returned_block->next = (Block *)free_list; 
    free_list = (char *)returned_block; // Update the free list pointer
}

// Resize the allocated memory block
void *mem_resize(void *block, size_t size) {
    if (!block) return mem_alloc(size);
    if (size == 0) {
        mem_free(block);
        return NULL;
    }

    Block *old_block = (Block *)((char *)block - sizeof(Block));
    size_t old_size = old_block->size; // Get the size of the current block

    // If the requested size is less than or equal to the old size, return the same block
    if (size <= old_size) return block;

    // Allocate new block of requested size
    void *new_block = mem_alloc(size);
    if (new_block) {
        memcpy(new_block, block, old_size); // Copy old data to new block
        mem_free(block); // Free the old block
    }
    return new_block; // Return pointer to the new block
}

// Free the entire memory pool
void mem_deinit() {
    munmap(memory_pool, pool_size); // Use munmap to free the memory pool
    memory_pool = NULL;
    pool_size = 0;
    free_list = NULL; // Clear the free list
}
