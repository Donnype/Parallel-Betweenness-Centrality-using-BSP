#ifndef NODE_H_
#define NODE_H_

typedef struct Node {
    long data;
    struct Node *next;
} Node;

Node* create_node(long data);
void push(struct Node** head, long data);
long pop(Node** stack);
void print_stack(Node stack);
Node* free_linked(Node** stack);

#endif