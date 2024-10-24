#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "linked_list.h"

// Initialize the linked list and allocate memory for the head node
void list_init(Node **head, size_t size) {
    *head = NULL; // Start with an empty list
}

// Insert a new node at the beginning of the list
void list_insert(Node **head, uint16_t data) {
    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        return; // Handle allocation failure
    }

    new_node->data = data;
    new_node->next = NULL;
    pthread_mutex_init(&new_node->lock, NULL); // Initialize node mutex

    pthread_mutex_lock(&new_node->lock); // Lock the new node
    pthread_mutex_lock(&(*head)->lock);   // Lock the head node if it exists

    new_node->next = *head;
    *head = new_node;

    pthread_mutex_unlock(&(*head)->lock); // Unlock the head node
    pthread_mutex_unlock(&new_node->lock); // Unlock the new node
}

// Insert a new node after the specified node
void list_insert_after(Node *prev_node, uint16_t data) {
    if (prev_node == NULL) return; // Handle null pointer

    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        return; // Handle allocation failure
    }

    new_node->data = data;
    new_node->next = NULL;
    pthread_mutex_init(&new_node->lock, NULL); // Initialize node mutex

    pthread_mutex_lock(&prev_node->lock); // Lock the previous node

    new_node->next = prev_node->next;
    prev_node->next = new_node;

    pthread_mutex_unlock(&prev_node->lock); // Unlock the previous node
}

// Insert a new node before the specified node
void list_insert_before(Node **head, Node *next_node, uint16_t data) {
    if (next_node == NULL) return; // Handle null pointer

    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        return; // Handle allocation failure
    }

    new_node->data = data;
    new_node->next = NULL;
    pthread_mutex_init(&new_node->lock, NULL); // Initialize node mutex

    if (*head == next_node) {
        // Insert at the head
        new_node->next = *head;
        *head = new_node;
        return;
    }

    Node *current = *head;
    while (current != NULL) {
        pthread_mutex_lock(&current->lock); // Lock current node
        if (current->next == next_node) {
            new_node->next = next_node;
            current->next = new_node;
            pthread_mutex_unlock(&current->lock); // Unlock current node
            return;
        }
        pthread_mutex_unlock(&current->lock); // Unlock current node
        current = current->next;
    }

    mem_free(new_node); // Free the new node if insertion fails
}

// Delete a node with the specified data from the list
void list_delete(Node **head, uint16_t data) {
    if (*head == NULL) return; // Handle empty list

    Node *current = *head, *prev = NULL;

    while (current != NULL) {
        pthread_mutex_lock(&current->lock); // Lock current node

        if (current->data == data) {
            if (prev) {
                pthread_mutex_lock(&prev->lock); // Lock previous node
                prev->next = current->next;
                pthread_mutex_unlock(&prev->lock); // Unlock previous node
            } else {
                *head = current->next; // Update head if deleting first node
            }

            mem_free(current); // Free the node
            pthread_mutex_unlock(&current->lock); // Unlock current node
            return;
        }

        prev = current;
        current = current->next;
        if (prev) pthread_mutex_unlock(&prev->lock); // Unlock previous node if not null
    }

    pthread_mutex_unlock(&current->lock); // Unlock current node
}

// Search for a node with the specified data in the list
Node *list_search(Node **head, uint16_t data) {
    Node *current = *head;

    while (current != NULL) {
        pthread_mutex_lock(&current->lock); // Lock current node

        if (current->data == data) {
            pthread_mutex_unlock(&current->lock); // Unlock current node
            return current; // Node found
        }

        pthread_mutex_unlock(&current->lock); // Unlock current node
        current = current->next;
    }
    return NULL; // Node not found
}

// Display the linked list
void list_display(Node **head) {
    Node *current = *head;

    while (current != NULL) {
        printf("%u -> ", current->data);
        current = current->next;
    }
    printf("NULL\n");
}

// Count the number of nodes in the list
int list_count_nodes(Node **head) {
    int count = 0;
    Node *current = *head;

    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count; // Return total node count
}

// Cleanup the linked list
void list_cleanup(Node **head) {
    Node *current = *head;
    Node *next;

    while (current != NULL) {
        next = current->next;
        mem_free(current); // Free the current node
        current = next;
    }
    *head = NULL; // Reset head to NULL
}
