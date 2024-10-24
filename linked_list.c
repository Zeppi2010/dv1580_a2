#include "linked_list.h"
#include "memory_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void list_init(Node **head, size_t size) {
    *head = NULL; // Initialize head to NULL
}

void list_insert(Node **head, uint16_t data) {
    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    pthread_mutex_init(&new_node->lock, NULL); // Initialize node mutex
    new_node->data = data;
    new_node->next = NULL;

    pthread_mutex_lock(&new_node->lock); // Lock the new node

    if (*head == NULL) {
        *head = new_node; // Insert as the head if the list is empty
    } else {
        pthread_mutex_lock(&(*head)->lock); // Lock the head node if it exists
        new_node->next = *head; // Insert before the current head
        *head = new_node; // Update head to the new node
        pthread_mutex_unlock(&(*head)->lock); // Unlock the head node
    }

    pthread_mutex_unlock(&new_node->lock); // Unlock the new node
}

void list_insert_after(Node *prev_node, uint16_t data) {
    if (prev_node == NULL) {
        fprintf(stderr, "The given previous node cannot be NULL\n");
        return;
    }

    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    pthread_mutex_init(&new_node->lock, NULL); // Initialize node mutex
    new_node->data = data;
    new_node->next = NULL;

    pthread_mutex_lock(&new_node->lock); // Lock the new node
    pthread_mutex_lock(&prev_node->lock); // Lock the previous node

    new_node->next = prev_node->next; // Insert after the previous node
    prev_node->next = new_node;

    pthread_mutex_unlock(&prev_node->lock); // Unlock the previous node
    pthread_mutex_unlock(&new_node->lock); // Unlock the new node
}

void list_insert_before(Node **head, Node *next_node, uint16_t data) {
    if (next_node == NULL) {
        fprintf(stderr, "The given next node cannot be NULL\n");
        return;
    }

    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    pthread_mutex_init(&new_node->lock, NULL); // Initialize node mutex
    new_node->data = data;

    pthread_mutex_lock(&new_node->lock); // Lock the new node
    Node *current = *head;
    Node *prev = NULL;

    while (current != NULL && current != next_node) {
        pthread_mutex_lock(&current->lock); // Lock current node
        if (prev != NULL) {
            pthread_mutex_unlock(&prev->lock); // Unlock previous node
        }
        prev = current;
        current = current->next;
    }

    if (prev != NULL) {
        new_node->next = prev->next; // Insert before the next node
        prev->next = new_node;
    } else {
        new_node->next = *head; // Insert as the new head
        *head = new_node;
    }

    if (current != NULL) {
        pthread_mutex_unlock(&current->lock); // Unlock next node
    }
    if (prev != NULL) {
        pthread_mutex_unlock(&prev->lock); // Unlock previous node
    }
    pthread_mutex_unlock(&new_node->lock); // Unlock the new node
}

void list_delete(Node **head, uint16_t data) {
    Node *current = *head;
    Node *prev = NULL;

    while (current != NULL && current->data != data) {
        pthread_mutex_lock(&current->lock); // Lock current node
        if (prev != NULL) {
            pthread_mutex_unlock(&prev->lock); // Unlock previous node
        }
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        fprintf(stderr, "Node with data %u not found\n", data);
        if (prev != NULL) {
            pthread_mutex_unlock(&prev->lock); // Unlock previous node if not null
        }
        return;
    }

    pthread_mutex_lock(&current->lock); // Lock current node
    if (prev != NULL) {
        prev->next = current->next; // Bypass the current node
    } else {
        *head = current->next; // Update head if necessary
    }

    pthread_mutex_unlock(&current->lock); // Unlock current node
    mem_free(current); // Free the memory of the deleted node
}

Node *list_search(Node **head, uint16_t data) {
    Node *current = *head;

    while (current != NULL) {
        pthread_mutex_lock(&current->lock); // Lock current node
        if (current->data == data) {
            pthread_mutex_unlock(&current->lock); // Unlock current node
            return current; // Return the node if found
        }
        pthread_mutex_unlock(&current->lock); // Unlock current node
        current = current->next;
    }
    return NULL; // Return NULL if not found
}

void list_display(Node **head) {
    Node *current = *head;
    while (current != NULL) {
        printf("%u -> ", current->data);
        current = current->next;
    }
    printf("NULL\n");
}

void list_display_range(Node **head, Node *start_node, Node *end_node) {
    Node *current = start_node;
    while (current != NULL && current != end_node) {
        printf("%u -> ", current->data);
        current = current->next;
    }
    if (current == end_node) {
        printf("%u -> ", current->data);
    }
    printf("NULL\n");
}

int list_count_nodes(Node **head) {
    int count = 0;
    Node *current = *head;

    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count; // Return the total number of nodes
}

void list_cleanup(Node **head) {
    Node *current = *head;
    while (current != NULL) {
        Node *temp = current;
        current = current->next;
        mem_free(temp); // Free each node
    }
    *head = NULL; // Set head to NULL after cleanup
}
