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

    // Initialize the metadata array with a single large free block
    blockMetaArray[0].size = size;
    blockMetaArray[0].isFree = 1;
    blockCount = 1;
    printf("Memory pool initialized with size: %zu\n", size);
}

// Allocate memory from the pool
void* mem_alloc(size_t size) {
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

            printf("Allocating memory at block %zu, size: %zu\n", i, size);
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

    if (blockIndex + 1 < blockCount && blockMetaArray[blockIndex + 1].isFree) {
        blockMetaArray[blockIndex].size += blockMetaArray[blockIndex + 1].size;
        for (size_t i = blockIndex + 1; i < blockCount - 1; ++i) {
            blockMetaArray[i] = blockMetaArray[i + 1];
        }
        blockCount--;
    }

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
    if (ptr == NULL) {
        return mem_alloc(newSize);
    }

    pthread_mutex_lock(&memory_lock);  // Lock

    size_t blockIndex = ((char*)ptr - (char*)memoryPool) / MIN_SIZE;
    BlockMeta* block = &blockMetaArray[blockIndex];

    if (block->size >= newSize) {
        pthread_mutex_unlock(&memory_lock);  // Unlock
        return ptr;
    }

    if (blockIndex + 1 < blockCount && blockMetaArray[blockIndex + 1].isFree) {
        BlockMeta* nextBlock = &blockMetaArray[blockIndex + 1];
        if (block->size + nextBlock->size >= newSize) {
            block->size += nextBlock->size;
            nextBlock->isFree = 0;
            pthread_mutex_unlock(&memory_lock);  // Unlock
            return ptr;
        }
    }

    void* new_block = mem_alloc(newSize);
    if (new_block != NULL) {
        memcpy(new_block, ptr, block->size);
        mem_free(ptr);
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
