#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "memory_manager.h"

#define MAX_BLOCKS 1024 // Maximum number of blocks in the pool

typedef struct MemoryBlock {
    size_t size;               // Size of the block
    int free;                 // 1 if free, 0 if allocated
    struct MemoryBlock *next;  // Pointer to the next block
} MemoryBlock;

static MemoryBlock *memory_pool = NULL; // Start of the memory pool
static pthread_mutex_t memory_lock;      // Mutex for thread safety

// Initialize the memory manager with a specified size
void mem_init(size_t size) {
    pthread_mutex_init(&memory_lock, NULL); // Initialize the mutex

    memory_pool = (MemoryBlock *)malloc(size);
    if (!memory_pool) {
        fprintf(stderr, "Memory allocation for the pool failed\n");
        exit(EXIT_FAILURE);
    }

    memory_pool->size = size - sizeof(MemoryBlock); // Size for user
    memory_pool->free = 1; // Mark the block as free
    memory_pool->next = NULL; // No next block
}

// Allocate a block of memory
void *mem_alloc(size_t size) {
    pthread_mutex_lock(&memory_lock); // Lock memory operations

    MemoryBlock *current = memory_pool;

    // Find a suitable block
    while (current) {
        if (current->free && current->size >= size) {
            // If the block is large enough, split it if necessary
            if (current->size > size + sizeof(MemoryBlock)) {
                MemoryBlock *new_block = (MemoryBlock *)((char *)current + sizeof(MemoryBlock) + size);
                new_block->size = current->size - size - sizeof(MemoryBlock);
                new_block->free = 1; // Mark new block as free
                new_block->next = current->next; // Link to next block
                current->next = new_block; // Update current block
                current->size = size; // Resize current block
            }
            current->free = 0; // Mark block as allocated
            pthread_mutex_unlock(&memory_lock); // Unlock memory operations
            return (char *)current + sizeof(MemoryBlock); // Return user pointer
        }
        current = current->next; // Move to the next block
    }

    pthread_mutex_unlock(&memory_lock); // Unlock memory operations
    return NULL; // Allocation failed
}

// Free a block of memory
void mem_free(void *block) {
    if (!block) return; // Handle null pointer

    pthread_mutex_lock(&memory_lock); // Lock memory operations

    MemoryBlock *current = (MemoryBlock *)((char *)block - sizeof(MemoryBlock));
    current->free = 1; // Mark block as free

    // Coalesce adjacent free blocks
    MemoryBlock *prev = NULL;
    MemoryBlock *next = current->next;

    if (next && next->free) {
        current->size += sizeof(MemoryBlock) + next->size; // Merge with next
        current->next = next->next; // Link to next of next
    }

    if (prev && prev->free) {
        prev->size += sizeof(MemoryBlock) + current->size; // Merge with previous
        prev->next = current->next; // Link to current's next
    }

    pthread_mutex_unlock(&memory_lock); // Unlock memory operations
}

// Resize an existing memory block
void *mem_resize(void *block, size_t size) {
    if (!block) return mem_alloc(size); // If block is NULL, just allocate new size

    pthread_mutex_lock(&memory_lock); // Lock memory operations

    MemoryBlock *current = (MemoryBlock *)((char *)block - sizeof(MemoryBlock));

    if (current->size >= size) {
        // If current block is large enough, resize in place
        if (current->size > size + sizeof(MemoryBlock)) {
            // Split the block if possible
            MemoryBlock *new_block = (MemoryBlock *)((char *)current + sizeof(MemoryBlock) + size);
            new_block->size = current->size - size - sizeof(MemoryBlock);
            new_block->free = 1;
            new_block->next = current->next;
            current->next = new_block;
            current->size = size; // Update size
        }
        pthread_mutex_unlock(&memory_lock); // Unlock memory operations
        return block; // Return the original block
    }

    // Allocate a new block and copy data
    void *new_block = mem_alloc(size);
    if (new_block) {
        memcpy(new_block, block, current->size); // Copy old data
        mem_free(block); // Free old block
    }

    pthread_mutex_unlock(&memory_lock); // Unlock memory operations
    return new_block; // Return new block
}

// Deinitialize the memory manager
void mem_deinit() {
    pthread_mutex_lock(&memory_lock); // Lock memory operations
    free(memory_pool); // Free the entire memory pool
    pthread_mutex_unlock(&memory_lock); // Unlock memory operations
    pthread_mutex_destroy(&memory_lock); // Destroy the mutex
}
