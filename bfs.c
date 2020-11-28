#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "Node.h"

long NR_VERTICES = 1000;
long NBH_INIT_SIZE = 10;
long SPARSITY = 3;

//Node;

void print_matrix(short **matrix) {
    printf("Two Dimensional array elements:\n\n");

    for (long i = 0; i < NR_VERTICES; i++) {
        for (long j = 0; j < NR_VERTICES; j++) {
            printf("%i ", matrix[i][j]);
            if (j == NR_VERTICES - 1) {
                printf("\n");
            }
        }
    }

    printf("\n");
}


void swap(long** a, long** b) {
    long* temp = *a;
    *a = *b;
    *b = temp;
}


short **generate_symmetric_matrix() {
    srand(time(NULL));
    short **matrix = (short **) malloc(NR_VERTICES * sizeof(short *));

    for (long i = 0; i < NR_VERTICES; ++i) {
        matrix[i] = (short *) malloc(NR_VERTICES * sizeof(short));
    }

    for (long i = 0; i < NR_VERTICES; ++i) {
        for (long j = i; j < NR_VERTICES; ++j) {
            // The SPARSITY constant defines that on average 1 out of SPARSITY edges are present.
            short val = (rand() % SPARSITY) == 0;

            matrix[i][j] = val;
            matrix[j][i] = val;
        }
    }

    return matrix;
}


short **fill_buffer(short graph[NR_VERTICES][NR_VERTICES]) {
    short **matrix = (short **) malloc(NR_VERTICES * sizeof(short *));

    for (long i = 0; i < NR_VERTICES; ++i) {
        matrix[i] = (short *) malloc(NR_VERTICES * sizeof(short));
    }

    for (long i = 0; i < NR_VERTICES; ++i) {
        for (long j = i; j < NR_VERTICES; ++j) {
            short val = graph[i][j];

            matrix[i][j] = val;
            matrix[j][i] = val;
        }
    }

    return matrix;
}


void free_variables(Node *first, Node *second, Node *third, Node *fourth) {
    if (first != NULL) {
        free(first);
    }

    if (second != NULL) {
        free(second);
    }

    if (third != NULL) {
        free(third);
    }

    if (fourth != NULL) {
        free(fourth);
    }
}


long *bfs_linked(short **adjacency, long source) {
    long *distances = malloc(NR_VERTICES * sizeof(long));
    memset(distances, -1, NR_VERTICES * sizeof(long));

    distances[source] = 0;

    Node *head = NULL, *next_neighbour = NULL, *neighbour_head = NULL, *neighbour = NULL;
    head = (Node *) malloc(sizeof(Node));
    head->data = source;

    for (long level = 1; level < NR_VERTICES; ++level) {
        if (head == NULL) {
            break;
        }

        Node *node = head;
        neighbour_head = NULL;
        next_neighbour = NULL;
        neighbour = NULL;

        while (node != NULL) {
            for (long i = 0; i < NR_VERTICES; ++i) {
                if (adjacency[i][node->data] > 0 && distances[i] < 0) {
                    distances[i] = level;

                    if (neighbour_head == NULL) {
                        neighbour_head = (Node *) malloc(sizeof(Node));
                        neighbour_head->data = i;
                        neighbour = neighbour_head;

                        continue;
                    }

                    next_neighbour = (Node *) malloc(sizeof(Node));

                    next_neighbour->data = i;
                    neighbour->next = next_neighbour;
                    neighbour = next_neighbour;
                }
            }

            node = node->next;
        }

        head = neighbour_head;
    }

    free_variables(head, next_neighbour, neighbour_head, neighbour);

    if (head != NULL) {
        free(head);
    }

    if (next_neighbour != NULL) {
        free(next_neighbour);
    }

    if (neighbour_head != NULL) {
        free(neighbour_head);
    }

    if (neighbour != NULL) {
        free(neighbour);
    }

    return distances;
}


long *bfs_vec(short **adjacency, long source) {
    long *distances = malloc(NR_VERTICES * sizeof(long));
    long *neighborhood = calloc(NBH_INIT_SIZE, sizeof(long));
    long *new_neighborhood = calloc(NBH_INIT_SIZE, sizeof(long));

    memset(distances, -1, NR_VERTICES * sizeof(long));

    distances[source] = 0;
    neighborhood[0] = source;
    neighborhood[1] = -1;

    long size_nbh = NBH_INIT_SIZE;

    for (long level = 1; level < NR_VERTICES; ++level) {
        if (neighborhood[0] == -1) {
            break;
        }

        long counter = 0;

        for (long i = 0; neighborhood[i] >= 0; ++i) {
            long vertex = neighborhood[i];

            for (long j = 0; j < NR_VERTICES; ++j) {
                if (adjacency[j][vertex] > 0 && distances[j] < 0) {
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

    return distances;
}
