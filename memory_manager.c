#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include "memory_manager.h"

#define MIN_SIZE sizeof(size_t) // Minimum size for allocation
#define MEMORY_POOL_SIZE 5000 // Define the size of the memory pool as needed

// Block metadata structure
typedef struct BlockMeta {
    size_t size; // Size of the block
    int isFree; // Is the block free?
} BlockMeta;

static char *memoryPool;           // Pointer to the memory pool
static BlockMeta *blockMetaArray;  // Array to hold metadata for blocks
static size_t blockCount;           // Total number of blocks
static pthread_mutex_t memory_lock; // Mutex for thread safety

// Function to align a pointer
void *align_ptr(void *ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    return (void *)(((addr + alignment - 1) / alignment) * alignment);
}

// Function to initialize the memory manager
void mem_init(size_t size) {
    // Allocate memory pool using mmap
    memoryPool = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memoryPool == MAP_FAILED) {
        fprintf(stderr, "Error: Unable to allocate memory pool using mmap.\n");
        exit(EXIT_FAILURE);
    }

    // Calculate the number of blocks based on minimum size
    blockCount = size / (MIN_SIZE + sizeof(BlockMeta)); 
    blockMetaArray = (BlockMeta *)malloc(blockCount * sizeof(BlockMeta));
    if (!blockMetaArray) {
        fprintf(stderr, "Error: Unable to allocate metadata array.\n");
        munmap(memoryPool, size);
        exit(EXIT_FAILURE);
    }

    // Initialize the blocks
    for (size_t i = 0; i < blockCount; ++i) {
        blockMetaArray[i].size = (size / blockCount); // Each block has an equal size
        blockMetaArray[i].isFree = 1;                // All blocks are initially free
    }

    // Align the memory pool
    memoryPool = (char *)align_ptr(memoryPool, 16); // Use 16 for alignment
    pthread_mutex_init(&memory_lock, NULL); // Initialize the mutex
}

// Function to allocate memory
void *mem_alloc(size_t size) {
    if (size == 0) return NULL; // Avoid zero-size allocation

    pthread_mutex_lock(&memory_lock); // Lock

    // Ensure size is at least minimum size
    size = (size < MIN_SIZE) ? MIN_SIZE : size;

    // Find a suitable block
    for (size_t i = 0; i < blockCount; ++i) {
        if (blockMetaArray[i].isFree && blockMetaArray[i].size >= size) {
            // Allocate the block
            blockMetaArray[i].isFree = 0; // Mark as used
            pthread_mutex_unlock(&memory_lock); // Unlock
            return (char *)memoryPool + (i * (MIN_SIZE + sizeof(BlockMeta))); // Return the pointer to the allocated block
        }
    }

    pthread_mutex_unlock(&memory_lock); // Unlock
    return NULL; // No suitable block found
}

// Function to free allocated memory
void mem_free(void *ptr) {
    if (ptr == NULL) return;

    pthread_mutex_lock(&memory_lock); // Lock
    size_t blockIndex = ((char *)ptr - (char *)memoryPool) / (MIN_SIZE + sizeof(BlockMeta));

    if (blockIndex < blockCount) {
        blockMetaArray[blockIndex].isFree = 1; // Mark the block as free
    } else {
        fprintf(stderr, "Error: Attempted to free invalid pointer.\n");
    }

    pthread_mutex_unlock(&memory_lock); // Unlock
}

// Function to resize allocated memory
void *mem_resize(void *block, size_t size) {
    if (block == NULL) return mem_alloc(size); // If block is NULL, allocate new block
    if (size == 0) {
        mem_free(block);
        return NULL; // Free block and return NULL
    }

    pthread_mutex_lock(&memory_lock); // Lock

    size_t oldSize = ((BlockMeta *)((char *)block - sizeof(BlockMeta)))->size; // Get the old block size
    void *newBlock = mem_alloc(size); // Allocate a new block of the new size

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
    munmap(memoryPool, MEMORY_POOL_SIZE); // Free mmaped memory
    free(blockMetaArray);
    pthread_mutex_destroy(&memory_lock);
}
