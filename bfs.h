#ifndef BSF_H_
#define BSF_H_

long *bfs_linked(short **adjacency, long source);
long *bfs_vec(short **adjacency, long source);
short **generate_symmetric_matrix();
void print_matrix(short **matrix);

#endif