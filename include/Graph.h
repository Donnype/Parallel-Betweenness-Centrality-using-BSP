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
    bool is_sparse; // A flag checking if the graph uses the sparse representation of its adjacent neighbours.
    long ** adjacency_lists; // The sparser list of adjacency lists of the vertices
    long * degrees; // Array counting the degrees of each vertex, for looping over the adjacency list.
} Graph;

void generate_graph();
void construct_graph(short matrix[args->nr_vertices][args->nr_vertices]);
long get_max_distance();
void to_sparse();
void clean_graph_data();
void free_graph();
void print_graph_values(long **matrix);
void print_graph_values_LF(long double **matrix);
void print_graph();

#endif
