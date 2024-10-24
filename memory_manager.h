#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h> // For size_t

#ifdef __cplusplus
extern "C" {
#endif

    void mem_init(size_t size);
    void* mem_alloc(size_t size);
    void mem_free(void* block);
    void* mem_resize(void* block, size_t size);
    void mem_deinit();

#ifdef __cplusplus
}
#endif

#endif // MEMORY_MANAGER_H
