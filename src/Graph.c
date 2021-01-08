#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "../include/Args.h"
#include "../include/Graph.h"
#include "../include/bfs.h"


extern Args* args;
Graph* graph;


short **generate_symmetric_mat() {
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


short **fill_buf(short graph[args->nr_vertices][args->nr_vertices]) {
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


void clean_graph_data() {
    if (graph->distances != NULL) {
        free_matrix_long(&(graph->distances), args->nr_processors);
    }

    if (graph->sigmas != NULL) {
        free_matrix_long(&(graph->sigmas), args->nr_processors);
    }

    if (graph->deltas != NULL) {
        free_matrix_double(&(graph->deltas), args->nr_processors);
    }
}


void initialize_properties() {
    graph->distances = NULL;
    graph->sigmas = NULL;
    graph->deltas = NULL;
    graph->is_sparse = NULL;
    graph->adjacency_lists = NULL;
    graph->degrees = NULL;
}


void generate_graph() {
    graph = (Graph*) malloc(sizeof(Graph));
    graph->adjacency_matrix = generate_symmetric_mat();
    initialize_properties();
}


void construct_graph(short matrix[args->nr_vertices][args->nr_vertices]) {
    graph = (Graph*) malloc(sizeof(Graph));
    graph->adjacency_matrix = fill_buf(matrix);
    initialize_properties();
}


long get_max_distance() {
    long max_distance = 0;

    for (int i = 0; i < args->nr_processors; ++i) {
        for (long j = 0; j < args->vertices_per_proc; j++) {
            if (graph->distances[i][j] > max_distance) {
                max_distance = graph->distances[i][j];
            }
        }
    }

    return max_distance;
}


void to_sparse() {
    graph->adjacency_lists = (long **) malloc(args->nr_vertices * sizeof(long *));
    graph->degrees = (long *) calloc(args->nr_vertices, sizeof(long));
    int step = 5;

    for (int j = 0; j < args->nr_vertices; ++j) {
        for (int i = 0; i < args->nr_vertices; ++i) {
            if (graph->adjacency_matrix[i][j] == 0) {
                continue;
            }

            if (graph->degrees[j] % step == 0) {
                graph->adjacency_lists[j] = realloc(graph->adjacency_lists[j], (graph->degrees[j] + step) * sizeof(long));
            }

            graph->adjacency_lists[j][graph->degrees[j]] = i;
            graph->degrees[j] += 1;
        }
    }

    graph->is_sparse = true;
}


void free_graph() {
    if (graph == NULL) {
        return;
    }

    if (graph->adjacency_matrix != NULL) {
        free_matrix(&(graph->adjacency_matrix), args->nr_vertices);
    }

    if (graph->adjacency_lists != NULL) {
        free_matrix_long(&(graph->adjacency_lists), args->nr_vertices);
    }

    free(graph);
}


void print_graph_values(long **matrix) {
    for (int i = 0; i < args->vertices_per_proc; ++i) {
        for (int j = 0; j < args->nr_processors; ++j) {
            printf("%ld ", matrix[j][i]);
        }
    }
    printf("\n");
}


void print_graph_values_LF(long double **matrix) {
    for (int i = 0; i < args->vertices_per_proc; ++i) {
        for (int j = 0; j < args->nr_processors; ++j) {
            printf("%LF ", matrix[j][i]);
        }
    }
    printf("\n");
}


void print_graph() {
    printf("distances \n");
    print_graph_values(graph->distances);

    printf("sigmas \n");
    print_graph_values(graph->sigmas);

    printf("deltas \n");
    print_graph_values_LF(graph->deltas);
}
