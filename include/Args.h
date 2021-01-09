#ifndef ARGS_H_
#define ARGS_H_

#include <stdbool.h>

// A structure that holds all the global arguments and flags.
typedef struct Args {
    long neighbourhood_size;
    long nr_vertices;
    long sparsity;
    long nr_processors;
    long vertices_per_proc;
    double runs;
    long batch_size;
    bool set_sparse;
    bool output;
    bool print_matrix;
    bool test;
} Args;

void read_args(int argc, char **argv);

#endif