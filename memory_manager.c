#define _GNU_SOURCE
#include "memory_manager.h"

typedef struct memory_block {
    void *start;
    void *end;
    struct memory_block *next;
} memory_block;

memory_block *memory_block_factory(void *start, void *end, memory_block *next) {
    memory_block *new_block = malloc(sizeof(*new_block));
    new_block->start = start;
    new_block->end = end;
    new_block->next = next;
    return new_block;
}

pthread_mutex_t allocation_lock;

memory_block *head;
void *memory_;
size_t size_;

/// @brief Initiates the memory mannager with @p size bytes of memory
/// @param size bytes that will be available in the memory manager
void mem_init(size_t size) {
    head = NULL;
    memory_ = malloc(size);
    size_ = size;
    pthread_mutex_init(&allocation_lock, NULL);
}

/// @brief Allocates @p size bytes of memory
/// @param @p size number of bytes that will be allocated
/// @return pointer to the allocated memory
void *mem_alloc(size_t size) {
    if (size > size_) return NULL;
    if (size == 0) return memory_;
    pthread_mutex_lock(&allocation_lock);

    // Insertion first
    if (head == NULL || head->start - memory_ >= size) {
        memory_block *new_block =
            memory_block_factory(memory_, memory_ + size, head);
        head = new_block;
        pthread_mutex_unlock(&allocation_lock);
        return memory_;
    }

    // Insertion between blocks or last
    memory_block *walker = head;
    while (walker != NULL) {
        size_t space = (walker->next) ? walker->next->start - walker->end
                                      : memory_ + size_ - walker->end;
        if (space >= size) {
            memory_block *new_block = memory_block_factory(
                walker->end, walker->end + size, walker->next);
            walker->next = new_block;
            void *ret_val = walker->end;
            pthread_mutex_unlock(&allocation_lock);
            return ret_val;
        }
        walker = walker->next;
    }
    pthread_mutex_unlock(&allocation_lock);
    return NULL;
}

/// @brief A nolock copy of the mem_alloc function that can be used for alloc
/// when a lock has already been aquired
/// @param size bytes that should be allocated
/// @return pointer to allocated memory
void *mem_alloc__nolock__(size_t size) {
    if (size > size_) return NULL;
    if (size == 0) return memory_;

    // insertion first
    if (head == NULL || head->start - memory_ >= size) {
        memory_block *new_block =
            memory_block_factory(memory_, memory_ + size, head);
        head = new_block;
        return memory_;
    }

    // Insertion between blocks or last
    memory_block *walker = head;
    while (walker != NULL) {
        size_t available_space = (walker->next)
                                     ? walker->next->start - walker->end
                                     : memory_ + size_ - walker->end;
        if (available_space >= size) {
            memory_block *new_block = memory_block_factory(
                walker->end, walker->end + size, walker->next);
            walker->next = new_block;
            walker->next = new_block;
            void *ret_val = walker->end;
            return ret_val;
        }
        walker = walker->next;
    }
    return NULL;
}

/// @brief Frees @p block preventing memory leaks
/// @param block
void mem_free(void *block) {
    pthread_mutex_lock(&allocation_lock);
    // No nodes
    if (!head) {
        pthread_mutex_unlock(&allocation_lock);
        return;
    }
    // block is first, change head
    if (head->start == block) {
        memory_block *temp = head;
        head = head->next;
        free(temp);
        pthread_mutex_unlock(&allocation_lock);
        return;
    }
    // block is not first, find block
    memory_block *walker = head;
    while (walker->next != NULL) {
        if (walker->next->start == block) {
            memory_block *temp = walker->next;
            walker->next = temp->next;
            free(temp);
            pthread_mutex_unlock(&allocation_lock);
            return;
        }
        walker = walker->next;
    }
    pthread_mutex_unlock(&allocation_lock);
    return;
}
/// @brief Changes the size of the allocated block, return NULL if failed
/// @param block pointer to your allocated memory, if NULL allocates new memory
/// of @p size
/// @param size The new size of your allocated memory, if 0 mem_free is called
/// for @p block
/// @return
void *mem_resize(void *block, size_t size) {
    // Edge cases
    if (size > size_) return NULL;
    if (!block) return mem_alloc(size);
    if (size == 0) {
        mem_free(block);
        return NULL;
    }
    pthread_mutex_lock(&allocation_lock);

    // Find the node and the previous node incase we want to replace the old one
    // after bypass
    memory_block *before_node = NULL;
    memory_block *node = head;
    while (node != NULL && node->start != block) {
        before_node = node;
        node = node->next;
    }

    // invalid block, return
    if (!node) {
        pthread_mutex_unlock(&allocation_lock);
        return NULL;
    }

    // Bypass old node so mem_alloc can properly return a new block
    if (before_node)
        before_node->next = node->next;
    else
        head = head->next;

    // get new block
    void *newblock = mem_alloc__nolock__(size);

    // if allocation failed replace the old node and return NULL
    if (!newblock) {
        if (before_node)
            before_node->next = node;
        else
            head = node;
        pthread_mutex_unlock(&allocation_lock);
        return NULL;
    }

    // Free old block no longer in use and copy over memory to new block, then
    // return new block
    size_t old_size = node->end - node->start;
    free(node);
    memcpy(newblock, block, (old_size < size) ? old_size : size);
    pthread_mutex_unlock(&allocation_lock);
    return newblock;
}

/// @brief gives back the memory used by the memory manager
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