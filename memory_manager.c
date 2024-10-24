#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>  // Include this for uintptr_t
#include <stddef.h>  // Include for size_t
#include "memory_manager.h"

#define MIN_SIZE sizeof(size_t)  // Minimum size for allocation
#define MEMORY_POOL_SIZE 1024 * 1024  // Size of the memory pool (1MB)

// Block metadata structure
typedef struct BlockMeta {
    size_t size;        // Size of the block
    int isFree;        // Is the block free?
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
void mem_init() {
    memoryPool = (char*)malloc(MEMORY_POOL_SIZE);
    if (!memoryPool) {
        fprintf(stderr, "Error: Unable to allocate memory pool.\n");
        exit(EXIT_FAILURE);
    }

    blockCount = MEMORY_POOL_SIZE / MIN_SIZE;
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
    memoryPool = (char*)align_ptr(memoryPool, alignof(max_align_t));
    pthread_mutex_init(&memory_lock, NULL); // Initialize the mutex
}

// Function to allocate memory
void* mem_alloc(size_t size) {
    if (size == 0) return NULL; // Avoid zero-size allocation

    pthread_mutex_lock(&memory_lock); // Lock

    // Ensure size is at least minimum size
    size = (size < MIN_SIZE) ? MIN_SIZE : size;
    for (size_t i = 0; i < blockCount; ++i) {
        if (blockMetaArray[i].isFree && blockMetaArray[i].size >= size) {
            // Check if there's enough space to split the block
            size_t remainingSize = blockMetaArray[i].size - size;
            if (remainingSize >= MIN_SIZE + sizeof(BlockMeta)) {
                blockMetaArray[i].size = size;
                blockMetaArray[i].isFree = 0;

                // Create a new free block
                blockMetaArray[blockCount].size = remainingSize;
                blockMetaArray[blockCount].isFree = 1;
                blockCount++;

                pthread_mutex_unlock(&memory_lock); // Unlock
                return (char*)memoryPool + (i * MIN_SIZE);
            } else {
                blockMetaArray[i].isFree = 0; // Allocate the whole block
                pthread_mutex_unlock(&memory_lock); // Unlock
                return (char*)memoryPool + (i * MIN_SIZE);
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

// Function to clean up the memory manager
void mem_destroy() {
    free(memoryPool);
    free(blockMetaArray);
    pthread_mutex_destroy(&memory_lock);
}
