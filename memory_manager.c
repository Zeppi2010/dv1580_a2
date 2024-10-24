#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

#define MEMORY_POOL_SIZE 5000 // Size of the memory pool

// Static variables for memory management
static char *memory_pool = NULL; // Pointer to the memory pool
static size_t pool_size = 0;      // Size of the memory pool

typedef struct Block {
    size_t size;                // Size of the block
    struct Block *next;         // Pointer to the next block
} Block;

static Block *free_list = NULL; // Pointer to the start of the free list

// Initialize the memory manager
void mem_init(size_t size) {
    pool_size = size;

    // Allocate the memory pool using malloc
    memory_pool = (char *)malloc(pool_size);
    if (memory_pool == NULL) {
        fprintf(stderr, "Failed to allocate memory pool with malloc\n");
        exit(EXIT_FAILURE);
    }

    // Set up the initial block
    free_list = (Block *)memory_pool; // Initially, the whole pool is free
    free_list->size = pool_size - sizeof(Block); // Set the size of the block
    free_list->next = NULL; // No next block
}

// Allocate a block of memory
void *mem_alloc(size_t size) {
    Block *current = free_list;
    Block *prev = NULL;

    // Align size to the nearest multiple of sizeof(size_t)
    size_t aligned_size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
    aligned_size += sizeof(Block); // Include space for block header

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
            } else {
                // If not splitting, we just allocate the whole block
                if (prev) {
                    prev->next = current->next; // Bypass the current block
                } else {
                    free_list = current->next; // Move free list head
                }
            }
            // Return pointer to the usable memory
            return (char *)current + sizeof(Block);
        }
        prev = current;
        current = current->next;
    }
    // No suitable block found
    return NULL;
}

// Free the allocated block of memory
void mem_free(void *block) {
    if (!block) return; // Ignore null pointers

    // Get the block header
    Block *returned_block = (Block *)((char *)block - sizeof(Block));

    // Inserting into the free list without overlaps
    Block *current = free_list;
    Block *prev = NULL;

    // Iterate to find the right position to insert the returned block
    while (current && current < returned_block) {
        prev = current;
        current = current->next;
    }

    // Check for overlaps with existing blocks
    Block *iterator = free_list;
    while (iterator) {
        if ((char *)iterator < (char *)returned_block + returned_block->size + sizeof(Block) &&
            (char *)iterator + iterator->size + sizeof(Block) > (char *)returned_block) {
            fprintf(stderr, "Error: Overlapping blocks detected!\n");
            return; // Overlap detected, do not free
        }
        iterator = iterator->next;
    }

    // Insert returned block into the free list
    returned_block->next = current; 
    if (prev) {
        prev->next = returned_block; // Insert in the middle or end
    } else {
        free_list = returned_block; // Update free list head
    }
}

// Resize the allocated memory block
void *mem_resize(void *block, size_t size) {
    if (!block) return mem_alloc(size);
    if (size == 0) {
        mem_free(block);
        return NULL;
    }

    Block *old_block = (Block *)((char *)block - sizeof(Block));
    size_t old_size = old_block->size + sizeof(Block); // Get the size of the current block

    // If the requested size is less than or equal to the old size, return the same block
    if (size + sizeof(Block) <= old_size) return block;

    // Allocate new block of requested size
    void *new_block = mem_alloc(size);
    if (new_block) {
        memcpy(new_block, block, old_size - sizeof(Block)); // Copy old data to new block
        mem_free(block); // Free the old block
    }
    return new_block; // Return pointer to the new block
}

// Free the entire memory pool
void mem_deinit() {
    free(memory_pool); // Free the memory pool using free
    memory_pool = NULL;
    pool_size = 0;
    free_list = NULL; // Clear the free list
}
