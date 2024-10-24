#define _GNU_SOURCE
#include "linked_list.h"

pthread_rwlock_t lock;

/// @brief Initializes the list
/// @param head list head
void list_init(Node** head, size_t size) {
    mem_init(size);
    *head = NULL;
    int init_result = pthread_rwlock_init(&lock, NULL);
    if (init_result != 0) {
        perror("pthread_rwlock_init failed");
        exit(EXIT_FAILURE);
    }
}

/// @brief inserts last in linked list
/// @param head list head
/// @param data data for the new node
void list_insert(Node** head, uint16_t data) {
    Node* new_node = mem_alloc(sizeof(Node));
    if (!new_node) {
        return;
    }
    new_node->data = data;
    new_node->next = NULL;
    pthread_rwlock_wrlock(&lock);
    if (*head == NULL) {
        *head = new_node;
        pthread_rwlock_unlock(&lock);
        return;
    }
    Node* walker = *head;
    while (walker->next) {
        walker = walker->next;
    }
    walker->next = new_node;
    pthread_rwlock_unlock(&lock);
    return;
}

/// @brief Inserts a node after prev_node
/// @param prev_nodenode that will be before new node
/// @param data data for the new node
void list_insert_after(Node* prev_node, uint16_t data) {
    if (prev_node == NULL) return;
    Node* new_node = mem_alloc(sizeof(Node));
    if (!new_node) return;
    pthread_rwlock_wrlock(&lock);
    new_node->next = prev_node->next;
    new_node->data = data;
    prev_node->next = new_node;
    pthread_rwlock_unlock(&lock);
}

/// @brief inserts before a node
/// @param head list head
/// @param next_node node that will be after new node
/// @param data data for the new node
void list_insert_before(Node** head, Node* next_node, uint16_t data) {
    pthread_rwlock_wrlock(&lock);
    if (*head == NULL) {
        return;  // ERROR
        pthread_rwlock_unlock(&lock);
    }
    Node* new_node = mem_alloc(sizeof(Node));
    if (!new_node) {
        pthread_rwlock_unlock(&lock);
        return;
    }

    if (next_node == *head) {
        new_node->data = data;
        new_node->next = *head;
        *head = new_node;
        pthread_rwlock_unlock(&lock);
        return;
    }

    Node* walker = *head;
    while (walker->next != next_node && walker->next != NULL) {
        walker = walker->next;
    }
    if (walker->next == NULL) {
        pthread_rwlock_unlock(&lock);
        mem_free(new_node);
        return;  // ERRROR
    }
    walker->next = new_node;
    walker->next->next = next_node;
    walker->next->data = data;
    pthread_rwlock_unlock(&lock);
    return;
}

/// @brief deletes the Node with data
/// @param head list head
/// @param data
void list_delete(Node** head, uint16_t data) {
    pthread_rwlock_wrlock(&lock);
    if (*head == NULL){
        pthread_rwlock_unlock(&lock);
        return;
    }
    if ((*head)->data == data) {
        Node* temp = *head;
        *head = (*head)->next;
        mem_free(temp);
        pthread_rwlock_unlock(&lock);
        return;
    }
    Node* walker = *head;
    while (walker->next != NULL && walker->next->data != data) {
        walker = walker->next;
    }
    if (walker->next == NULL) {
        pthread_rwlock_unlock(&lock);
        return;
    }
    Node* temp = walker->next;
    walker->next = temp->next;
    mem_free(temp);
    pthread_rwlock_unlock(&lock);
    return;
}

/// @brief return the pointer to node with data or NULL if not found
/// @param head list head
/// @param data value to search for
/// @return Node* or NULL if node not found
Node* list_search(Node** head, uint16_t data) {
    pthread_rwlock_rdlock(&lock);
    Node* walker = *head;
    while (walker != NULL) {
        if (walker->data == data) {
            pthread_rwlock_unlock(&lock);
            return walker;
        }
        walker = walker->next;
    }
    pthread_rwlock_unlock(&lock);
    return NULL;
}

/// @brief displays all nodes
/// @param head list head
void list_display(Node** head) { list_display_range(head, NULL, NULL); }

/// @brief Displays the nodes in the range, including start and end
/// @param head list head
/// @param start_node first node to display
/// @param end_node last node to display
void list_display_range(Node** head, Node* start_node, Node* end_node) {
    pthread_rwlock_rdlock(&lock);
    if (end_node) end_node = end_node->next;
    if (!start_node) start_node = *head;
    printf("[");
    while (start_node != NULL && start_node != end_node) {
        printf("%d", start_node->data);
        start_node = start_node->next;
        if (start_node && start_node != end_node) printf(", ");
    }
    printf("]");
    pthread_rwlock_unlock(&lock);
}

/// @brief returns the number of nodes
/// @param head list head
/// @return int
int list_count_nodes(Node** head) {
    pthread_rwlock_rdlock(&lock);
    Node* walker = *head;
    int counter = 0;
    while (walker != NULL) {
        counter++;
        walker = walker->next;
    }
    pthread_rwlock_unlock(&lock);
    return counter;
}

/// @brief frees all used memory
/// @param head list head
void list_cleanup(Node** head) {
    *head = NULL;
    mem_deinit();
    pthread_rwlock_destroy(&lock);
}