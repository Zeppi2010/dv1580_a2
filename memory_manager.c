#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>   // For uintptr_t
#include <stdalign.h> // For alignof (C11 feature, fallback will be handled)
#include <stddef.h>   // For size_t

#define MIN_SIZE 32    // Minimum block size

typedef struct {
    size_t size;  // Size of the block
    int isFree;   // Is the block free (1) or allocated (0)
} BlockMeta;

void* memoryPool = NULL;
size_t pool_size = 0;
BlockMeta blockMetaArray[1000];  // Array to hold block metadata (modify the size as needed)
size_t blockCount = 0;

pthread_mutex_t memory_lock;  // Mutex for thread safety

// Fallback for max_align_t if not available (C11)
#ifndef max_align_t
typedef struct { alignas(16) char c[16]; } max_align_t;
#endif

// Helper function to ensure proper alignment
void* align_ptr(void* ptr, size_t alignment) {
    return (void*)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
}

// Initialize the memory manager and the pool
void mem_init(size_t size) {
    memoryPool = malloc(size);
    if (!memoryPool) {
        printf("Failed to initialize memory pool.\n");
        exit(1);
    }

    memoryPool = align_ptr(memoryPool, alignof(max_align_t));  // Ensure alignment for all types
    pool_size = size;

    // Initialize the mutex
    if (pthread_mutex_init(&memory_lock, NULL) != 0) {
        printf("Mutex init failed.\n");
        free(memoryPool);
        exit(1);
    }

    // Initialize the metadata array with a single large free block
    blockMetaArray[0].size = size;
    blockMetaArray[0].isFree = 1;
    blockCount = 1;

    printf("Memory pool initialized with size: %zu\n", size);
}

// Allocate memory from the pool
void* mem_alloc(size_t size) {
    if (size == 0) return NULL;  // No allocation for zero size

    pthread_mutex_lock(&memory_lock);  // Lock

    for (size_t i = 0; i < blockCount; ++i) {
        if (blockMetaArray[i].isFree && blockMetaArray[i].size >= size) {
            size_t remainingSize = blockMetaArray[i].size - size;

            if (remainingSize >= MIN_SIZE + sizeof(BlockMeta)) {
                blockMetaArray[i].size = size;
                blockMetaArray[i].isFree = 0;

                blockMetaArray[blockCount].size = remainingSize;
                blockMetaArray[blockCount].isFree = 1;
                blockCount++;
            } else {
                blockMetaArray[i].isFree = 0;
            }

            pthread_mutex_unlock(&memory_lock);  // Unlock
            printf("Allocating memory at block %zu, size: %zu\n", i, size);
            return (char*)memoryPool + (i * MIN_SIZE);
        }
    }

    printf("Error: No suitable block found for size %zu\n", size);
    pthread_mutex_unlock(&memory_lock);  // Unlock
    return NULL;
}

// Free memory and mark the block as free
void mem_free(void* ptr) {
    if (ptr == NULL) return;

    pthread_mutex_lock(&memory_lock);  // Lock

    size_t blockIndex = ((char*)ptr - (char*)memoryPool) / MIN_SIZE;
    if (blockIndex >= blockCount) {
        printf("Error: Invalid pointer passed to mem_free.\n");
        pthread_mutex_unlock(&memory_lock);
        return;
    }

    printf("Freeing memory at block %zu\n", blockIndex);
    blockMetaArray[blockIndex].isFree = 1;

    // Merge with next block if free
    if (blockIndex + 1 < blockCount && blockMetaArray[blockIndex + 1].isFree) {
        blockMetaArray[blockIndex].size += blockMetaArray[blockIndex + 1].size;
        for (size_t i = blockIndex + 1; i < blockCount - 1; ++i) {
            blockMetaArray[i] = blockMetaArray[i + 1];
        }
        blockCount--;
    }

    // Merge with previous block if free
    if (blockIndex > 0 && blockMetaArray[blockIndex - 1].isFree) {
        blockMetaArray[blockIndex - 1].size += blockMetaArray[blockIndex].size;
        for (size_t i = blockIndex; i < blockCount - 1; ++i) {
            blockMetaArray[i] = blockMetaArray[i + 1];
        }
        blockCount--;
    }

    pthread_mutex_unlock(&memory_lock);  // Unlock
}

// Resize an allocated memory block
void* mem_resize(void* ptr, size_t newSize) {
    if (ptr == NULL) return mem_alloc(newSize);

    pthread_mutex_lock(&memory_lock);  // Lock

    size_t blockIndex = ((char*)ptr - (char*)memoryPool) / MIN_SIZE;
    BlockMeta* block = &blockMetaArray[blockIndex];

    // If current block can accommodate the new size, return the same pointer
    if (block->size >= newSize) {
        pthread_mutex_unlock(&memory_lock);  // Unlock
        return ptr;
    }

    // Try to merge with the next block if it's free
    if (blockIndex + 1 < blockCount && blockMetaArray[blockIndex + 1].isFree) {
        BlockMeta* nextBlock = &blockMetaArray[blockIndex + 1];
        if (block->size + nextBlock->size >= newSize) {
            block->size += nextBlock->size;
            nextBlock->isFree = 0;  // Mark the next block as used
            pthread_mutex_unlock(&memory_lock);  // Unlock
            return ptr;
        }
    }

    // Allocate a new block if resize in place is not possible
    void* new_block = mem_alloc(newSize);
    if (new_block != NULL) {
        memcpy(new_block, ptr, block->size);  // Copy existing data
        mem_free(ptr);  // Free the old block
    }

    pthread_mutex_unlock(&memory_lock);  // Unlock
    return new_block;
}

// Deinitialize the memory pool and destroy the mutex
void mem_deinit() {
    pthread_mutex_lock(&memory_lock);  // Lock before cleanup

    pthread_mutex_destroy(&memory_lock);  // Destroy the mutex
    free(memoryPool);  // Free the memory pool
    memoryPool = NULL;
    pool_size = 0;
    blockCount = 0;

    pthread_mutex_unlock(&memory_lock);  // Unlock after cleanup

    printf("Memory pool deinitialized.\n");
}
