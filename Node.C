#include <stdlib.h>
#include <stdio.h>
#include "Node.h"


Node* create_node(long data) {
    Node* node = (Node*) malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;

    return node;
}


void push(Node** head, long data) {
    Node* node = create_node(data);
    node->next = *head;
    *head = node;
}


long pop(Node** stack) {
    if (*stack == NULL) {
        return -1;
    }

    Node* head = *stack;
    long data = head->data;

    (*stack) = head->next;

    free(head);

    return data;
}


void print_stack(Node* stack) {
    printf("\n(");
    Node* tmp = stack;

    while (tmp != NULL) {
        printf("%ld ", tmp->data);

        tmp = tmp->next;
    }

    printf(")\n");
}