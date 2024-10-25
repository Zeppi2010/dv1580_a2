#define _GNU_SOURCE
#include "memory_manager.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct memory_block {
    void *start;
    void *end;
    struct memory_block *next;
} memory_block;

memory_block *memory_block_factory(void *start, void *end, memory_block *next) {
    memory_block *new_block = malloc(sizeof(*new_block));
    if (!new_block) return NULL; // Check if malloc fails
    new_block->start = start;
    new_block->end = end;
    new_block->next = next;
    return new_block;
}

pthread_mutex_t allocation_lock;
memory_block *head;
void *memory_;
size_t size_;

// Initialize memory manager with a given size
void mem_init(size_t size) {
    head = NULL;
    memory_ = malloc(size);
    if (!memory_) return; // Handle failure if malloc fails
    size_ = size;
    pthread_mutex_init(&allocation_lock, NULL);
}

// Core allocation function, shared by mem_alloc and mem_alloc__nolock__
void *mem_alloc_core(size_t size, int lock_needed) {
    if (size > size_) return NULL;
    if (size == 0) return memory_;
    if (lock_needed) pthread_mutex_lock(&allocation_lock);

    // Check if we can place the block at the beginning
    if (head == NULL || (head->start - memory_) >= size) {
        memory_block *new_block = memory_block_factory(memory_, memory_ + size, head);
        if (!new_block) {
            if (lock_needed) pthread_mutex_unlock(&allocation_lock);
            return NULL;
        }
        head = new_block;
        if (lock_needed) pthread_mutex_unlock(&allocation_lock);
        return memory_;
    }

    // Traverse list to find sufficient space between blocks or at the end
    memory_block *walker = head;
    while (walker != NULL) {
        size_t space = (walker->next) ? walker->next->start - walker->end : memory_ + size_ - walker->end;
        if (space >= size) {
            memory_block *new_block = memory_block_factory(walker->end, walker->end + size, walker->next);
            if (!new_block) {
                if (lock_needed) pthread_mutex_unlock(&allocation_lock);
                return NULL;
            }
            walker->next = new_block;
            void *ret_val = walker->end;
            if (lock_needed) pthread_mutex_unlock(&allocation_lock);
            return ret_val;
        }
        walker = walker->next;
    }

    if (lock_needed) pthread_mutex_unlock(&allocation_lock);
    return NULL;
}

// Thread-safe memory allocation function
void *mem_alloc(size_t size) {
    return mem_alloc_core(size, 1);
}

// No-lock allocation function (used internally in mem_resize)
void *mem_alloc__nolock__(size_t size) {
    return mem_alloc_core(size, 0);
}

// Frees a block of allocated memory
void mem_free(void *block) {
    if (!block) return; // Null block check
    pthread_mutex_lock(&allocation_lock);
    
    if (!head) { // No nodes case
        pthread_mutex_unlock(&allocation_lock);
        return;
    }
    
    if (head->start == block) { // Free head node case
        memory_block *temp = head;
        head = head->next;
        free(temp);
    } else { // Free middle or last node
        memory_block *walker = head;
        while (walker->next != NULL) {
            if (walker->next->start == block) {
                memory_block *temp = walker->next;
                walker->next = temp->next;
                free(temp);
                break;
            }
            walker = walker->next;
        }
    }
    pthread_mutex_unlock(&allocation_lock);
}

// Resizes an allocated memory block, allocating new space if needed
void *mem_resize(void *block, size_t size) {
    if (size > size_ || !block) return mem_alloc(size); // Edge cases
    if (size == 0) {
        mem_free(block);
        return NULL;
    }
    pthread_mutex_lock(&allocation_lock);

    memory_block *before_node = NULL;
    memory_block *node = head;
    while (node != NULL && node->start != block) {
        before_node = node;
        node = node->next;
    }

    if (!node) { // Invalid block case
        pthread_mutex_unlock(&allocation_lock);
        return NULL;
    }

    if (before_node) before_node->next = node->next; // Unlink node
    else head = head->next;

    void *newblock = mem_alloc__nolock__(size); // Allocate new block
    if (!newblock) { // Failed allocation case
        if (before_node) before_node->next = node;
        else head = node;
        pthread_mutex_unlock(&allocation_lock);
        return NULL;
    }

    size_t old_size = node->end - node->start;
    memcpy(newblock, block, (old_size < size) ? old_size : size); // Copy data
    free(node);
    pthread_mutex_unlock(&allocation_lock);
    return newblock;
}

// Deinitializes the memory manager, freeing all allocated blocks and resources
void mem_deinit() {
    memory_block *walker = head;
    while (walker != NULL) {
        memory_block *temp = walker;
        walker = walker->next;
        free(temp);
    }
    free(memory_);
    size_ = 0;
    head = NULL;
    pthread_mutex_destroy(&allocation_lock);
}
