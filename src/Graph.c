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
Graph** batch;


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


void clean_graph_data(Graph* g) {
    if (g->distances != NULL) {
        free_matrix_long(&(g->distances), args->nr_processors);
        g->distances = NULL;
    }

    if (g->sigmas != NULL) {
        free_matrix_long(&(g->sigmas), args->nr_processors);
        g->sigmas = NULL;
    }

    if (g->deltas != NULL) {
        free_matrix_double(&(g->deltas), args->nr_processors);
        g->deltas = NULL;
    }
}


void initialize_properties(Graph* g) {
    g->source = 0;
    g->distances = NULL;
    g->sigmas = NULL;
    g->deltas = NULL;
    g->is_sparse = NULL;
    g->adjacency_lists = NULL;
    g->degrees = NULL;
}


void generate_graph() {
    graph = (Graph*) malloc(sizeof(Graph));
    graph->adjacency_matrix = generate_symmetric_mat();
    initialize_properties(graph);
}


void construct_graph(short matrix[args->nr_vertices][args->nr_vertices]) {
    graph = (Graph*) malloc(sizeof(Graph));
    graph->adjacency_matrix = fill_buf(matrix);
    initialize_properties(graph);
}


void create_batch() {
    batch = malloc(args->batch_size * sizeof(Graph*));

    for (int i = 0; i < args->batch_size; ++i) {
        batch[i] = (Graph*) malloc(sizeof(Graph));
        initialize_properties(batch[i]);
        batch[i]->adjacency_matrix = graph->adjacency_matrix;
        batch[i]->adjacency_lists = graph->adjacency_lists;
        batch[i]->degrees = graph->degrees;
        batch[i]->is_sparse = graph->is_sparse;
    }
}


long get_max_distance(long index) {
    long max_distance = 0;

    for (int i = 0; i < args->nr_processors; ++i) {
        for (long j = 0; j < args->vertices_per_proc; j++) {
            if (batch[index]->distances[i][j] > max_distance) {
                max_distance = batch[index]->distances[i][j];
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
        graph->adjacency_lists[j] = NULL;

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


void free_graph(Graph *g) {
    if (g == NULL) {
        return;
    }

    if (g->adjacency_matrix != NULL) {
        free_matrix(&(g->adjacency_matrix), args->nr_vertices);
    }

    if (g->adjacency_lists != NULL) {
        free_matrix_long(&(g->adjacency_lists), args->nr_vertices);
    }

    if (g->degrees != NULL) {
        free(g->degrees);
    }

    free(g);
}


void free_batch() {
    free_graph(graph);

    for (int i = 0; i < args->batch_size; ++i) {
        clean_graph_data(batch[i]);
    }

    free(batch);
}


void clean_batch_data() {
    for (int j = 0; j < args->batch_size; ++j) {
        clean_graph_data(batch[j]);
    }
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
