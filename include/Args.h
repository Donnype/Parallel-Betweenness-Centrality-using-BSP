#ifndef ARGS_H_
#define ARGS_H_

#include <stdbool.h>

typedef struct Args {
    long neighbourhood_size;
    long nr_vertices;
    long sparsity;
    long nr_processors;
    bool output;
    bool print_matrix;
    bool test;
} Args;

void read_args(int argc, char **argv);

#endif