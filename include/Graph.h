#ifndef _GRAPH_H
#define _GRAPH_H

#include <stdbool.h>
#include "../include/Args.h"

extern Args *args;

// A structure that holds the graph information.
typedef struct Graph {
    short ** adjacency_matrix;
    long source;
    long ** distances;
    long ** sigmas;
    long double ** deltas;
    bool is_sparse; // A flag checking if the graph uses the sparse representation of its adjacent neighbours.
    long ** adjacency_lists; // The sparser list of adjacency lists of the vertices
    long * degrees; // Array counting the degrees of each vertex, for looping over the adjacency list.
    long double ** betweennesses; // Array to sum the dependencies in to get the betweennesses.
} Graph;

void initialize_properties(Graph* g);
void generate_graph();
void generate_long_graph(long factor);
void construct_graph(short matrix[args->nr_vertices][args->nr_vertices]);
long get_max_distance(long index);
void to_sparse();
void create_batch();
void clean_batch_data();
void free_batch();
void free_graph(Graph* g);
void print_graph_values(long **matrix);
void print_graph_values_LF(long double **matrix);
void print_graph();

#endif
