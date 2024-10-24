#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "memory_manager.h"

#define MIN_SIZE sizeof(size_t) // Minimum size for allocation
#define MEMORY_POOL_SIZE 1024 * 1024 // Size of the memory pool (1MB)

// Block metadata structure
typedef struct BlockMeta {
    size_t size; // Size of the block
    int isFree; // Is the block free?
} BlockMeta;

static char* memoryPool;           // Pointer to the memory pool
static BlockMeta* blockMetaArray;  // Array to hold metadata for blocks
static size_t blockCount;           // Total number of blocks
static pthread_mutex_t memory_lock; // Mutex for thread safety

// Function to align a pointer
void* align_ptr(void* ptr, size_t alignment) {
    if (alignment == 0) return ptr; // Avoid division by zero
    uintptr_t addr = (uintptr_t)ptr;
    return (void*)(((addr + alignment - 1) & ~(alignment - 1)));
}

// Function to initialize the memory manager
void mem_init(size_t size) {
    memoryPool = (char*)malloc(size);
    if (!memoryPool) {
        fprintf(stderr, "Error: Unable to allocate memory pool.\n");
        exit(EXIT_FAILURE);
    }

    blockCount = size / MIN_SIZE;
    blockMetaArray = (BlockMeta*)malloc(blockCount * sizeof(BlockMeta));
    if (!blockMetaArray) {
        fprintf(stderr, "Error: Unable to allocate metadata array.\n");
        free(memoryPool);
        exit(EXIT_FAILURE);
    }

    // Initialize the blocks
    for (size_t i = 0; i < blockCount; ++i) {
        blockMetaArray[i].size = MIN_SIZE; // Each block has a minimum size
        blockMetaArray[i].isFree = 1;      // All blocks are initially free
    }

    // Align the memory pool
    memoryPool = (char*)align_ptr(memoryPool, 16); // Use 16 for alignment
    pthread_mutex_init(&memory_lock, NULL); // Initialize the mutex
}

// Function to allocate memory
void* mem_alloc(size_t size) {
    if (size == 0) return NULL; // Avoid zero-size allocation

    pthread_mutex_lock(&memory_lock); // Lock

    // Ensure size is at least minimum size
    size = (size < MIN_SIZE) ? MIN_SIZE : size;

    // Find a suitable block
    for (size_t i = 0; i < blockCount; ++i) {
        if (blockMetaArray[i].isFree && blockMetaArray[i].size >= size) {
            // Check if we can split the block
            size_t remainingSize = blockMetaArray[i].size - size;

            // If the remaining size is enough for a new block
            if (remainingSize >= MIN_SIZE + sizeof(BlockMeta)) {
                // Allocate the requested block
                blockMetaArray[i].isFree = 0; // Mark as used
                blockMetaArray[i].size = size; // Set new size

                // Create a new free block
                blockMetaArray[blockCount].size = remainingSize; // Size of the remaining block
                blockMetaArray[blockCount].isFree = 1; // Mark new block as free
                blockCount++; // Increase the block count

                pthread_mutex_unlock(&memory_lock); // Unlock
                return (char*)memoryPool + (i * MIN_SIZE); // Return the pointer to the allocated block
            } else {
                // Allocate the whole block
                blockMetaArray[i].isFree = 0; // Mark as used
                pthread_mutex_unlock(&memory_lock); // Unlock
                return (char*)memoryPool + (i * MIN_SIZE); // Return the pointer
            }
        }
    }

    pthread_mutex_unlock(&memory_lock); // Unlock
    return NULL; // No suitable block found
}

// Function to free allocated memory
void mem_free(void* ptr) {
    if (ptr == NULL) return;

    pthread_mutex_lock(&memory_lock); // Lock
    size_t blockIndex = ((char*)ptr - (char*)memoryPool) / MIN_SIZE;

    if (blockIndex < blockCount) {
        blockMetaArray[blockIndex].isFree = 1; // Mark the block as free
    } else {
        fprintf(stderr, "Error: Attempted to free invalid pointer.\n");
    }

    pthread_mutex_unlock(&memory_lock); // Unlock
}

// Function to resize allocated memory
void* mem_resize(void* block, size_t size) {
    if (block == NULL) return mem_alloc(size); // If block is NULL, allocate new block
    if (size == 0) {
        mem_free(block);
        return NULL; // Free block and return NULL
    }

    pthread_mutex_lock(&memory_lock); // Lock

    size_t oldSize = ((BlockMeta*)((char*)block - sizeof(BlockMeta)))->size; // Get the old block size
    void* newBlock = mem_alloc(size); // Allocate a new block of the new size

    if (newBlock) {
        // Copy old data to new block
        memcpy(newBlock, block, (oldSize < size) ? oldSize : size);
        mem_free(block); // Free old block
    }

    pthread_mutex_unlock(&memory_lock); // Unlock
    return newBlock; // Return new block
}

// Function to clean up the memory manager
void mem_deinit() {
    free(memoryPool);
    free(blockMetaArray);
    pthread_mutex_destroy(&memory_lock);
}
