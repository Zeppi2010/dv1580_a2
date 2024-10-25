#define _GNU_SOURCE
#include "memory_manager.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// Struct representing a block of memory
typedef struct memory_block {
    void *start;             // Start address of the memory block
    void *end;               // End address of the memory block
    struct memory_block *next; // Pointer to the next memory block in the linked list
} memory_block;

// Factory function for creating a new memory block
memory_block *memory_block_factory(void *start, void *end, memory_block *next) {
    // Allocate space for a new memory_block structure
    memory_block *new_block = malloc(sizeof(*new_block));
    if (!new_block) return NULL; // Return NULL if allocation fails

    // Initialize block's properties
    new_block->start = start;
    new_block->end = end;
    new_block->next = next;
    return new_block;
}

// Mutex for managing concurrent access to the memory manager
pthread_mutex_t allocation_lock;

// Head of the linked list of memory blocks
memory_block *head;
void *memory_;     // Pointer to the start of the managed memory
size_t size_;      // Total size of the managed memory

// Initialize the memory manager with a given size
void mem_init(size_t size) {
    head = NULL;             // Initialize the list as empty
    memory_ = malloc(size);  // Allocate the memory pool
    if (!memory_) return;    // Handle failure if malloc fails
    size_ = size;            // Set the size of the memory pool
    pthread_mutex_init(&allocation_lock, NULL); // Initialize the mutex
}

// Core allocation function, shared by mem_alloc and mem_alloc__nolock__
void *mem_alloc_core(size_t size, int lock_needed) {
    if (size > size_) return NULL; // If requested size is larger than available memory, return NULL
    if (size == 0) return memory_; // Special case: if size is 0, return the base memory address

    if (lock_needed) pthread_mutex_lock(&allocation_lock); // Lock if needed for thread-safety

    // Check if the block can fit at the start of memory
    if (head == NULL || (head->start - memory_) >= size) {
        // Create a new block and set it as the head
        memory_block *new_block = memory_block_factory(memory_, memory_ + size, head);
        if (!new_block) {
            if (lock_needed) pthread_mutex_unlock(&allocation_lock); // Unlock and return NULL if allocation fails
            return NULL;
        }
        head = new_block; // Update head
        if (lock_needed) pthread_mutex_unlock(&allocation_lock); // Unlock if needed
        return memory_;
    }

    // Traverse linked list to find available space between or after blocks
    memory_block *walker = head;
    while (walker != NULL) {
        size_t space = (walker->next) ? walker->next->start - walker->end : memory_ + size_ - walker->end;
        if (space >= size) { // Found space for the new block
            // Create and insert a new memory block in the free space
            memory_block *new_block = memory_block_factory(walker->end, walker->end + size, walker->next);
            if (!new_block) {
                if (lock_needed) pthread_mutex_unlock(&allocation_lock);
                return NULL;
            }
            walker->next = new_block;
            void *ret_val = walker->end;
            if (lock_needed) pthread_mutex_unlock(&allocation_lock); // Unlock if needed
            return ret_val;
        }
        walker = walker->next; // Move to the next block
    }

    if (lock_needed) pthread_mutex_unlock(&allocation_lock); // Unlock if needed
    return NULL; // Return NULL if no suitable space was found
}

// Thread-safe memory allocation function
void *mem_alloc(size_t size) {
    return mem_alloc_core(size, 1); // Call core function with lock
}

// No-lock allocation function (used internally in mem_resize)
void *mem_alloc__nolock__(size_t size) {
    return mem_alloc_core(size, 0); // Call core function without lock
}

// Frees a block of allocated memory
void mem_free(void *block) {
    if (!block) return; // Do nothing if block is NULL

    pthread_mutex_lock(&allocation_lock); // Lock to ensure thread-safety

    if (!head) { // No blocks allocated, nothing to free
        pthread_mutex_unlock(&allocation_lock);
        return;
    }

    if (head->start == block) { // Free the head node if it matches
        memory_block *temp = head;
        head = head->next;
        free(temp); // Free the block memory
    } else { // Traverse to find and free the matching block
        memory_block *walker = head;
        while (walker->next != NULL) {
            if (walker->next->start == block) {
                memory_block *temp = walker->next;
                walker->next = temp->next;
                free(temp); // Free the found block
                break;
            }
            walker = walker->next;
        }
    }
    pthread_mutex_unlock(&allocation_lock); // Unlock after freeing
}

// Resizes an allocated memory block, allocating new space if needed
void *mem_resize(void *block, size_t size) {
    if (size > size_ || !block) return mem_alloc(size); // Handle large size or NULL block
    if (size == 0) {
        mem_free(block); // Free if size is zero
        return NULL;
    }

    pthread_mutex_lock(&allocation_lock); // Lock for thread-safety

    // Traverse list to find the block and the previous node
    memory_block *before_node = NULL;
    memory_block *node = head;
    while (node != NULL && node->start != block) {
        before_node = node;
        node = node->next;
    }

    if (!node) { // If block isn't found, return NULL
        pthread_mutex_unlock(&allocation_lock);
        return NULL;
    }

    if (before_node) before_node->next = node->next; // Unlink node
    else head = head->next;

    // Allocate new block with the specified size
    void *newblock = mem_alloc__nolock__(size);
    if (!newblock) { // If allocation failed, restore original linkage and return NULL
        if (before_node) before_node->next = node;
        else head = node;
        pthread_mutex_unlock(&allocation_lock);
        return NULL;
    }

    // Copy data from old block to new block, free old block, and unlock
    size_t old_size = node->end - node->start;
    memcpy(newblock, block, (old_size < size) ? old_size : size); // Copy minimum of old and new sizes
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
        free(temp); // Free each memory block in the linked list
    }
    free(memory_); // Free the main memory pool
    size_ = 0;     // Reset size to 0
    head = NULL;   // Set head to NULL, indicating empty memory
    pthread_mutex_destroy(&allocation_lock); // Destroy mutex
}
