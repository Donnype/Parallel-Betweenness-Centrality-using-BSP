#include <stdlib.h>
#include <stdio.h>
#include "Node.h"


Node* create_node(long data) {
    Node* node = (struct Node*) malloc(sizeof(struct Node));
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
    Node* head = *stack;

    if (head == NULL || head->data == -1) {
        return -1;
    }

    long data = head->data;

    (*stack) = head->next;

    if (head != NULL) {
        free(head);
    }

    return data;
}


void print_stack(Node stack) {
    printf("(");
    Node tmp = stack;

    while (tmp.next != NULL) {
        printf("%ld ", tmp.data);

        tmp = *tmp.next;
    }

    printf("%ld )\n", tmp.data);
}