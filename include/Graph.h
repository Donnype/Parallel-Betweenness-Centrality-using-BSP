#ifndef _GRAPH_H
#define _GRAPH_H

#include <stdbool.h>
#include "../include/Args.h"

extern Args *args;

// A structure that holds the graph information.
typedef struct Graph {
    short ** adjacency_matrix;
    long ** distances;
    long ** sigmas;
    long double ** deltas;
} Graph;

void generate_graph();
void construct_graph(short matrix[args->nr_vertices][args->nr_vertices]);
void clean_graph_data();
void free_graph();
void print_graph_values(long **matrix);
void print_graph_values_LF(long double **matrix);
void print_graph();

#endif
