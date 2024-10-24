#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// @brief Initiates the memory mannager with @p size bytes of memory
/// @param size bytes that will be available in the memory manager
void mem_init(size_t size);

/// @brief Allocates @p size bytes of memory
/// @param @p size number of bytes that will be allocated
/// @return pointer to the allocated memory
void* mem_alloc(size_t size);

/// @brief Frees @p block preventing memory leaks
/// @param block
void mem_free(void* block);

/// @brief Changes the size of the allocated block, return NULL if failed
/// @param block pointer to your allocated memory, if NULL allocates new memory
/// of @p size
/// @param size The new size of your allocated memory, if 0 mem_free is called
/// for @p block
/// @return
void* mem_resize(void* block, size_t size);

/// @brief gives back the memory used by the memory manager, makes the memory
/// mannager unusable until new init
void mem_deinit();

#endif