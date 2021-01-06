#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "../include/Node.h"
#include "../include/Args.h"
#include "../include/Graph.h"


extern Args* args;
extern Graph* graph;


void print_matrix(short **matrix) {
    for (long i = 0; i < args->nr_vertices; i++) {
        for (long j = 0; j < args->nr_vertices; j++) {
            printf("%i ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
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


long *bfs_linked(long source) {
    long *distances = malloc(args->nr_vertices * sizeof(long));
    memset(distances, -1, args->nr_vertices * sizeof(long));

    distances[source] = 0;

    Node *new_stack = create_node(-1);
    Node *stack = create_node(-1);
    push(&stack, source);

    for (long level = 1; level < args->nr_vertices; ++level) {
        long vertex = pop(&stack);

        if (vertex == -1) {
            break;
        }

        while (vertex >= 0) {
            for (long neighbour = 0; neighbour < args->nr_vertices; ++neighbour) {
                if (graph->adjacency_matrix[neighbour][vertex] > 0 && distances[neighbour] < 0) {
                    distances[neighbour] = level;

                    push(&new_stack, neighbour);
                }
            }

            vertex = pop(&stack);
        }

        *stack = *new_stack;
        new_stack = free_linked(&new_stack);
    }

    free_linked(&stack);

    return distances;
}


void bfs_vec(long source) {
    long *distances = malloc(args->nr_vertices * sizeof(long));
    memset(distances, -1, args->nr_vertices * sizeof(long));

    long *neighborhood = calloc(args->neighbourhood_size, sizeof(long));
    long *new_neighborhood = calloc(args->neighbourhood_size, sizeof(long));

    distances[source] = 0;
    neighborhood[0] = source;
    neighborhood[1] = -1;

    long size_nbh = args->neighbourhood_size;

    for (long level = 1; level < args->nr_vertices; ++level) {
        if (neighborhood[0] == -1) {
            break;
        }

        long counter = 0;

        for (long i = 0; neighborhood[i] >= 0; ++i) {
            long vertex = neighborhood[i];

            for (long j = 0; j < args->nr_vertices; ++j) {
                if (graph->adjacency_matrix[j][vertex] > 0 && distances[j] < 0) {
                    distances[j] = level;
                    new_neighborhood[counter] = j;
                    counter++;
                }

                if (counter + 1 >= size_nbh) {
                    new_neighborhood = realloc(new_neighborhood, 2 * size_nbh * sizeof(long));
                    neighborhood = realloc(neighborhood, 2 * size_nbh * sizeof(long));
                    size_nbh = 2 * size_nbh;
                }
            }
        }

        new_neighborhood[counter] = -1;
        memcpy(neighborhood, new_neighborhood, size_nbh * sizeof(long));
    }

    if (neighborhood != NULL) {
        free(neighborhood);
    }

    if (new_neighborhood != NULL) {
        free(new_neighborhood);
    }

    graph->distances = malloc(sizeof(long *));
    graph->distances[0] = distances;
}
