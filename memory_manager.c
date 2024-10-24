#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>  // For POSIX threads
#include <string.h>   // For memcpy

#define MIN_SIZE 32   // Minimum block size

typedef struct {
    size_t size;  // Size of the block
    int isFree;   // Is the block free (1) or allocated (0)
} BlockMeta;

void* memoryPool = NULL;
size_t pool_size = 0;
BlockMeta blockMetaArray[1000];  // Array to hold block metadata (modify the size as needed)
size_t blockCount = 0;           // Number of blocks

pthread_mutex_t memory_lock;  // Mutex for thread safety

// Function to align a pointer
void* align_ptr(void* ptr, size_t alignment) {
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }
    return (void*)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
}

// Initialize the memory manager and the pool
void mem_init(size_t size) {
    memoryPool = malloc(size);
    pool_size = size;

    if (!memoryPool) {
        printf("Failed to initialize memory pool.\n");
        exit(1);
    }

    // Initialize the mutex
    pthread_mutex_init(&memory_lock, NULL);

    // Align memory pool
    memoryPool = align_ptr(memoryPool, alignof(max_align_t));  // Ensure alignment for all types

    // Initialize the metadata array with a single large free block
    blockMetaArray[0].size = size;
    blockMetaArray[0].isFree = 1;
    blockCount = 1;
    printf("Memory pool initialized with size: %zu\n", size);
}

// Allocate memory from the pool
void* mem_alloc(size_t size) {
    if (size == 0) {
        return NULL;  // Handle zero allocation request
    }

    pthread_mutex_lock(&memory_lock);  // Lock

    for (size_t i = 0; i < blockCount; ++i) {
        if (blockMetaArray[i].isFree && blockMetaArray[i].size >= size) {
            size_t remainingSize = blockMetaArray[i].size - size;

            // If there's enough space to split the block
            if (remainingSize >= MIN_SIZE + sizeof(BlockMeta)) {
                blockMetaArray[i].size = size;
                blockMetaArray[i].isFree = 0;

                // Add new free block
                blockMetaArray[blockCount].size = remainingSize;
                blockMetaArray[blockCount].isFree = 1;
                blockCount++;
                printf("Allocating memory at block %zu, size: %zu (splitting)\n", i, size);
            } else {
                blockMetaArray[i].isFree = 0; // Allocate the whole block
                printf("Allocating memory at block %zu, size: %zu (no split)\n", i, size);
            }

            pthread_mutex_unlock(&memory_lock);  // Unlock
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
    printf("Freeing memory at block %zu\n", blockIndex);

    blockMetaArray[blockIndex].isFree = 1;

    // Merge with next block if free
    if (blockIndex + 1 < blockCount && blockMetaArray[blockIndex + 1].isFree) {
        blockMetaArray[blockIndex].size += blockMetaArray[blockIndex + 1].size;
        for (size_t i = blockIndex + 1; i < blockCount - 1; ++i) {
            blockMetaArray[i] = blockMetaArray[i + 1];
        }
        blockCount--;
        printf("Merged with next block, new size: %zu\n", blockMetaArray[blockIndex].size);
    }

    // Merge with previous block if free
    if (blockIndex > 0 && blockMetaArray[blockIndex - 1].isFree) {
        blockMetaArray[blockIndex - 1].size += blockMetaArray[blockIndex].size;
        for (size_t i = blockIndex; i < blockCount - 1; ++i) {
            blockMetaArray[i] = blockMetaArray[i + 1];
        }
        blockCount--;
        printf("Merged with previous block, new size: %zu\n", blockMetaArray[blockIndex - 1].size);
    }

    pthread_mutex_unlock(&memory_lock);  // Unlock
}

// Resize an allocated memory block
void* mem_resize(void* ptr, size_t newSize) {
    if (ptr == NULL) {
        return mem_alloc(newSize);
    }

    if (newSize == 0) {
        mem_free(ptr);
        return NULL;
    }

    pthread_mutex_lock(&memory_lock);  // Lock

    size_t blockIndex = ((char*)ptr - (char*)memoryPool) / MIN_SIZE;
    BlockMeta* block = &blockMetaArray[blockIndex];

    if (block->size >= newSize) {
        pthread_mutex_unlock(&memory_lock);  // Unlock
        return ptr;  // No need to resize
    }

    // Try to expand with next block if free
    if (blockIndex + 1 < blockCount && blockMetaArray[blockIndex + 1].isFree) {
        BlockMeta* nextBlock = &blockMetaArray[blockIndex + 1];
        if (block->size + nextBlock->size >= newSize) {
            block->size += nextBlock->size;
            nextBlock->isFree = 0;
            printf("Resized block %zu to new size: %zu (merged with next block)\n", blockIndex, block->size);
            pthread_mutex_unlock(&memory_lock);  // Unlock
            return ptr;
        }
    }

    // Allocate new block
    void* new_block = mem_alloc(newSize);
    if (new_block != NULL) {
        memcpy(new_block, ptr, block->size);  // Copy old data
        mem_free(ptr);  // Free old block
        printf("Allocated new block for resizing, old block freed\n");
    }

    pthread_mutex_unlock(&memory_lock);  // Unlock
    return new_block;
}

// Deinitialize the memory pool and destroy the mutex
void mem_deinit() {
    pthread_mutex_destroy(&memory_lock);  // Destroy the mutex
    free(memoryPool);
    memoryPool = NULL;
    pool_size = 0;
    blockCount = 0;
    printf("Memory pool deinitialized.\n");
}
