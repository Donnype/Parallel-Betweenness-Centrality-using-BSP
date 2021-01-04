#include <stdlib.h>
#include <stdio.h>
#include "../include/Node.h"


Node* create_node(long data) {
    Node* node = (Node*) malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;

    return node;
}


void push(Node** head, long data) {
    Node* node = create_node(data);
    node->data = data;
    node->next = *head;

    *head = node;
}


long pop(Node** stack) {
    Node* head = *stack;

    if (!head || head->data == -1) {
        return -1;
    }

    long data = head->data;
    *stack = head->next;

    if (head != NULL) {
        head->next = NULL;
        free(head);
    }

    return data;
}


Node* free_linked(Node** stack) {
    long tmp = 1;

    while (tmp != -1) {
        tmp = pop(stack);
    }

    free(stack);

    return create_node(-1);
}