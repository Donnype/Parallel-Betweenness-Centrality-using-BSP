#ifndef ARGS_H_
#define ARGS_H_

#include <stdbool.h>

// A structure that holds all the global arguments and flags.
typedef struct Args {
    long neighbourhood_size;
    long nr_vertices;
    long sparsity;
    long nr_processors;
    double runs;
    bool output;
    bool print_matrix;
    bool test;
} Args;

void read_args(int argc, char **argv);

#endif