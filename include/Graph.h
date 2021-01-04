#ifndef _GRAPH_H
#define _GRAPH_H

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
void free_graph();

#endif
