#include <stdlib.h>
#include "../include/Args.h"
#include "../include/Graph.h"


extern Args* args;
Graph* graph;


short **generate_symmetric_matrix() {
    srand(time(NULL));
    short **matrix = (short **) malloc(args->nr_vertices * sizeof(short *));

    for (long i = 0; i < args->nr_vertices; ++i) {
        matrix[i] = (short *) malloc(args->nr_vertices * sizeof(short));
    }

    for (long i = 0; i < args->nr_vertices; ++i) {
        for (long j = i; j < args->nr_vertices; ++j) {
            // The args->sparsity constant defines that on average 1 out of args->sparsity edges are present.
            short val = (rand() % args->sparsity) == 0;

            matrix[i][j] = val;
            matrix[j][i] = val;
        }
    }

    return matrix;
}


short **fill_buffer(short graph[args->nr_vertices][args->nr_vertices]) {
    short **matrix = (short **) malloc(args->nr_vertices * sizeof(short *));

    for (long i = 0; i < args->nr_vertices; ++i) {
        matrix[i] = (short *) malloc(args->nr_vertices * sizeof(short));
    }

    for (long i = 0; i < args->nr_vertices; ++i) {
        for (long j = i; j < args->nr_vertices; ++j) {
            short val = graph[i][j];

            matrix[i][j] = val;
            matrix[j][i] = val;
        }
    }

    return matrix;
}


void free_matrix(short ***M, long nr_rows) {
    short **matrix = *M;

    for (int i = 0; i < nr_rows; ++i) {
        if (matrix[i] != NULL) {
            free(matrix[i]);
        }
    }

    if (matrix != NULL) {
        free(matrix);
    }
}


void free_matrix_long(long ***M, long nr_rows) {
    long **matrix = *M;

    for (int i = 0; i < nr_rows; ++i) {
        if (matrix[i] != NULL) {
            free(matrix[i]);
        }
    }

    if (matrix != NULL) {
        free(matrix);
    }
}


void free_matrix_double(long double ***M, long nr_rows) {
    long double **matrix = *M;

    for (int i = 0; i < nr_rows; ++i) {
        if (matrix[i] != NULL) {
            free(matrix[i]);
        }
    }

    if (matrix != NULL) {
        free(matrix);
    }
}


void generate_graph() {
    graph = (Graph*) malloc(sizeof(Graph));
    graph->adjacency_matrix = generate_symmetric_matrix();
}


void construct_graph(short matrix[args->nr_vertices][args->nr_vertices]) {
    graph = (Graph*) malloc(sizeof(Graph));
    graph->adjacency_matrix = fill_buffer(matrix);
}


void free_graph() {
    if (graph == NULL) {
        return;
    }

    if (graph->adjacency_matrix != NULL) {
        free_matrix(&(graph->adjacency_matrix), args->nr_vertices);
    }

    if (graph->distances != NULL) {
        free_matrix_long(&(graph->distances), args->nr_vertices);
    }

    if (graph->sigmas != NULL) {
        free_matrix_long(&(graph->sigmas), args->nr_vertices);
    }

    if (graph->deltas != NULL) {
        free_matrix_double(&(graph->deltas), args->nr_vertices);
    }
}
