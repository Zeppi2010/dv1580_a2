#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

typedef struct Block {
    size_t size;         // Size of the block
    struct Block* next;  // Pointer to the next block in the free list
} Block;

static Block* free_list = NULL; // Head of the free list
static size_t total_memory = 0; // Total allocated memory

void mem_init(size_t size) {
    if (free_list != NULL) {
        fprintf(stderr, "Memory manager already initialized.\n");
        return;
    }

    total_memory = size;
    // Allocate memory for the pool and the initial block
    free_list = (Block*)malloc(size);
    if (free_list == NULL) {
        perror("Failed to initialize memory manager");
        exit(EXIT_FAILURE);
    }

    free_list->size = size - sizeof(Block); // Set the size of the initial block
    free_list->next = NULL; // No other blocks in the free list
}

void mem_deinit(void) {
    if (free_list == NULL) {
        fprintf(stderr, "Memory manager is not initialized.\n");
        return;
    }

    free(free_list); // Free the entire memory pool
    free_list = NULL; // Reset the free list pointer
}

void* mem_alloc(size_t size) {
    if (free_list == NULL) {
        fprintf(stderr, "Memory manager is not initialized.\n");
        return NULL;
    }

    // Align size to a multiple of 8 bytes
    size = (size + sizeof(Block) + 7) & ~7;

    Block* current = free_list;
    Block* previous = NULL;

    // Traverse the free list to find a suitable block
    while (current) {
        if (current->size >= size) { // Found a block large enough
            if (current->size > size + sizeof(Block)) { // Split block if too large
                Block* new_block = (Block*)((char*)current + sizeof(Block) + size);
                new_block->size = current->size - size - sizeof(Block);
                new_block->next = current->next;
                current->next = new_block;
                current->size = size;
            }
            // Remove the current block from the free list
            if (previous) {
                previous->next = current->next;
            } else {
                free_list = current->next; // Update head of free list
            }
            return (void*)((char*)current + sizeof(Block)); // Return pointer to user space
        }
        previous = current;
        current = current->next;
    }

    fprintf(stderr, "Memory allocation failed: not enough space.\n");
    return NULL; // No suitable block found
}

void mem_free(void* ptr) {
    if (ptr == NULL) return; // No action on NULL pointer

    Block* block_to_free = (Block*)((char*)ptr - sizeof(Block)); // Get the block header
    block_to_free->next = free_list; // Add the block to the front of the free list
    free_list = block_to_free;
}

void* mem_resize(void* ptr, size_t new_size) {
    if (ptr == NULL) return mem_alloc(new_size); // If ptr is NULL, just allocate new memory

    Block* block_to_resize = (Block*)((char*)ptr - sizeof(Block)); // Get the block header
    if (new_size <= block_to_resize->size) return ptr; // No need to resize

    // Allocate a new block and copy the data
    void* new_block = mem_alloc(new_size);
    if (new_block == NULL) return NULL; // Allocation failed

    // Copy old data to the new block
    memcpy(new_block, ptr, block_to_resize->size);
    mem_free(ptr); // Free the old block

    return new_block; // Return the new block pointer
}
