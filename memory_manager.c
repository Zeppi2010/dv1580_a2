#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "memory_manager.h"

#define MEMORY_SIZE 5000 // Size for initial memory allocation

typedef struct MemoryBlock {
    size_t size;               // Size of the allocated memory block
    struct MemoryBlock *next;  // Pointer to the next memory block in the list
    void *ptr;                // Pointer to the allocated memory
} MemoryBlock;

MemoryBlock *head = NULL; // Head of the linked list of memory blocks

// Initialize memory manager
void mem_init() {
    head = NULL; // Reset the linked list
}

// Deinitialize memory manager
void mem_deinit() {
    MemoryBlock *current = head;
    while (current != NULL) {
        MemoryBlock *next = current->next;
        munmap(current->ptr, current->size); // Free each allocated memory block
        // The following line is optional; it's good practice to ensure
        // you're not holding onto a pointer to freed memory
        free(current);
        current = next; // Move to the next block
    }
    head = NULL; // Clear the list
}

// Custom memory allocation function
void *mem_alloc(size_t size) {
    if (size <= 0) return NULL; // Invalid size

    // Allocate space for the memory block structure
    MemoryBlock *block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
    if (block == NULL) return NULL; // Allocation failed

    // Allocate the requested memory using mmap
    block->ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block->ptr == MAP_FAILED) {
        free(block); // Free the block structure if mmap fails
        return NULL; // mmap failed
    }

    block->size = size;         // Set the size of the block
    block->next = head;         // Link to the previous head
    head = block;               // Update head to the new block

    return block->ptr;          // Return pointer to allocated memory
}

// Custom memory free function
void mem_free(void *ptr) {
    if (ptr == NULL) return; // Nothing to free

    MemoryBlock *current = head;
    MemoryBlock *prev = NULL;

    // Traverse the list to find the block to free
    while (current != NULL) {
        if (current->ptr == ptr) {
            if (prev) {
                prev->next = current->next; // Link the previous block to the next
            } else {
                head = current->next; // Update head if freeing the first block
            }
            munmap(current->ptr, current->size); // Free the allocated memory
            free(current); // Free the memory block structure
            return;
        }
        prev = current; // Move to the next block
        current = current->next;
    }
}

// Optional: Print currently allocated memory blocks for debugging
void print_allocations() {
    MemoryBlock *current = head;
    while (current != NULL) {
        printf("Allocated block of size %zu at %p\n", current->size, current->ptr);
        current = current->next;
    }
}
