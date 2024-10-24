#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "memory_manager.h"

#define MEMORY_SIZE 5000

typedef struct MemoryBlock {
    size_t size;               // Size of the allocated memory block
    struct MemoryBlock *next;  // Pointer to the next memory block in the list
    void *ptr;                // Pointer to the allocated memory
} MemoryBlock;

MemoryBlock *head = NULL;      // Head of the linked list of memory blocks

// Custom memory allocation function
void *my_malloc(size_t size) {
    if (size <= 0) return NULL; // Invalid size

    // Allocate space for the memory block structure
    MemoryBlock *block = (MemoryBlock *)sbrk(sizeof(MemoryBlock));
    if (block == (void *)-1) return NULL; // Allocation failed

    // Allocate the requested memory using mmap
    block->ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block->ptr == MAP_FAILED) return NULL; // mmap failed

    block->size = size;         // Set the size of the block
    block->next = head;         // Link to the previous head
    head = block;               // Update head to the new block

    return block->ptr;          // Return pointer to allocated memory
}

// Custom memory free function
void my_free(void *ptr) {
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
            // Free the memory block structure
            brk(current); // Adjust program break to free the block metadata
            return;
        }
        prev = current; // Move to the next block
        current = current->next;
    }
    // Pointer was not found
}

// Optional: Print currently allocated memory blocks for debugging
void print_allocations() {
    MemoryBlock *current = head;
    while (current != NULL) {
        printf("Allocated block of size %zu at %p\n", current->size, current->ptr);
        current = current->next;
    }
}
