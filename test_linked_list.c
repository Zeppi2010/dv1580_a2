#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "linked_list.h"
#include "memory_manager.h"

// Initialize the linked list
void list_init(Node **head) {
    *head = NULL; // Set head to NULL to indicate an empty list
}

// Insert a node at the end of the list
void list_insert(Node **head, uint16_t data) {
    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    new_node->data = data;
    new_node->next = NULL;
    pthread_mutex_init(&new_node->lock, NULL); // Initialize the mutex for the new node

    if (*head == NULL) {
        *head = new_node; // Insert at the head if list is empty
    } else {
        Node *current = *head;
        while (current->next) {
            current = current->next; // Traverse to the end of the list
        }
        current->next = new_node; // Insert the new node
    }
}

// Insert a node after a given node
void list_insert_after(Node *prev_node, uint16_t data) {
    if (!prev_node) return;

    Node *new_node = (Node *)mem_alloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    new_node->data = data;
    new_node->next = prev_node->next;
    prev_node->next = new_node;
}

// Insert a node before a given node
void list_insert_before(Node **head, Node *next_node, uint16_t data) {
    if (!*head || !next_node) return;

    if (*head == next_node) { // Insert before head
        list_insert(head, data);
        return;
    }

    Node *current = *head;
    while (current && current->next != next_node) {
        current = current->next; // Traverse until we find the previous node
    }

    if (current) {
        Node *new_node = (Node *)mem_alloc(sizeof(Node));
        if (!new_node) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
        new_node->data = data;
        new_node->next = next_node;
        current->next = new_node;
    }
}

// Delete a node with the specified data
void list_delete(Node **head, uint16_t data) {
    if (*head == NULL) return; // Nothing to delete

    Node *current = *head;
    Node *prev = NULL;

    while (current && current->data != data) {
        prev = current;
        current = current->next; // Traverse the list
    }

    if (!current) return; // Node not found

    if (prev) {
        prev->next = current->next; // Bypass the current node
    } else {
        *head = current->next; // Update head if necessary
    }

    mem_free(current); // Free the memory
}

// Search for a node with the specified data
Node *list_search(Node **head, uint16_t data) {
    Node *current = *head;
    while (current) {
        if (current->data == data) return current; // Node found
        current = current->next;
    }
    return NULL; // Node not found
}

// Display the linked list
void list_display(Node **head) {
    Node *current = *head;
    printf("[");
    while (current) {
        printf("%d", current->data);
        current = current->next;
        if (current) printf(", ");
    }
    printf("]\n");
}

// Display a range of nodes
void list_display_range(Node **head, Node *start_node, Node *end_node) {
    Node *current = (start_node == NULL) ? *head : start_node;
    printf("[");
    while (current && (end_node == NULL || current != end_node->next)) {
        printf("%d", current->data);
        current = current->next;
        if (current && (end_node == NULL || current != end_node->next)) {
            printf(", ");
        }
    }
    printf("]\n");
}

// Count the number of nodes
int list_count_nodes(Node **head) {
    int count = 0;
    Node *current = *head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// Cleanup the linked list
void list_cleanup(Node **head) {
    Node *current = *head;
    Node *next;
    while (current) {
        next = current->next;
        mem_free(current); // Free each node
        current = next;
    }
    *head = NULL; // Set head to NULL
}
