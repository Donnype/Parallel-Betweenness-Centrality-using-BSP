#ifndef BSF_H_
#define BSF_H_

#include "Node.h"

extern long NR_VERTICES;
extern long SPARSITY;

long *bfs_linked(short **adjacency, long source);
long *bfs_vec(short **adjacency, long source);
short **generate_symmetric_matrix();
short **fill_buffer(short graph[NR_VERTICES][NR_VERTICES]);
void free_variables(struct Node *first, struct Node *second, struct Node *third, struct Node *fourth);
void print_matrix(short **matrix);


#endif