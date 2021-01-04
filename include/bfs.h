#ifndef BSF_H_
#define BSF_H_

#include "Node.h"
#include "Args.h"

extern Args *args;

long *bfs_linked(long source);
void bfs_vec(long source);
void free_matrix_long(long*** M, long nr_rows);
void free_matrix_double(long double*** M, long nr_rows);
void print_matrix(short **matrix);

#endif
