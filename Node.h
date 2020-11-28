#ifndef NODE_H_
#define NODE_H_

struct Node {
    long data;
    struct Node *next;
};

typedef struct Node Node;

#endif