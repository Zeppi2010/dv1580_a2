#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MEMORY_SIZE 1024 * 1024 // 1 MB

static unsigned char memory[MEMORY_SIZE];
static size_t used_memory = 0;
static pthread_mutex_t memory_mutex;
static int initialized = 0;

// Structure to keep track of allocated memory blocks
typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

static Block* allocated_blocks = NULL;

void mem_init() {
    pthread_mutex_init(&memory_mutex, NULL);
    initialized = 1;
}

void* mem_alloc(size_t size) {
    if (!initialized) {
        fprintf(stderr, "Memory manager not initialized. Call mem_init() first.\n");
        return NULL;
    }

    if (size <= 0 || used_memory + size > MEMORY_SIZE) {
        fprintf(stderr, "Allocation error: Invalid size or not enough memory.\n");
        return NULL;
    }

    pthread_mutex_lock(&memory_mutex);

    void* ptr = memory + used_memory;
    Block* block = (Block*)ptr;
    block->size = size;
    block->next = allocated_blocks;
    allocated_blocks = block;

    used_memory += size + sizeof(Block);

    pthread_mutex_unlock(&memory_mutex);

    return (void*)((unsigned char*)ptr + sizeof(Block)); // Return pointer after the Block structure
}

void mem_free(void* ptr) {
    if (!initialized || ptr == NULL) {
        fprintf(stderr, "Free error: Memory manager not initialized or pointer is NULL.\n");
        return;
    }

    pthread_mutex_lock(&memory_mutex);

    Block* block = (Block*)((unsigned char*)ptr - sizeof(Block));
    Block* current = allocated_blocks;
    Block* prev = NULL;

    // Search for the block to free
    while (current != NULL) {
        if (current == block) {
            if (prev != NULL) {
                prev->next = current->next;
            } else {
                allocated_blocks = current->next;
            }
            used_memory -= current->size + sizeof(Block);
            pthread_mutex_unlock(&memory_mutex);
            return;
        }
        prev = current;
        current = current->next;
    }

    fprintf(stderr, "Free error: Pointer not found in allocated blocks.\n");
    pthread_mutex_unlock(&memory_mutex);
}

void mem_cleanup() {
    if (initialized) {
        pthread_mutex_destroy(&memory_mutex);
        initialized = 0;
    }
}
