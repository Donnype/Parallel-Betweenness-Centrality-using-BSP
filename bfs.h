#ifndef BSF_H_
#define BSF_H_

extern long NR_VERTICES;
extern long SPARSITY;

long *bfs_linked(short **adjacency, long source);
long *bfs_vec(short **adjacency, long source);
short **generate_symmetric_matrix();
short **fill_buffer(short graph[NR_VERTICES][NR_VERTICES]);
void print_matrix(short **matrix);

#endif