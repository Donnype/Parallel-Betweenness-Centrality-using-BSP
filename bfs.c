#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>

const long NR_VERTICES = 100;
const long NBH_INIT_SIZE = 10;

struct Node {
    long data;
    struct Node *next;
};


void print_matrix(short **matrix) {
    printf("Two Dimensional array elements:\n");

    for (long i = 0; i < NR_VERTICES; i++) {
        for (long j = 0; j < NR_VERTICES; j++) {
            printf("%i ", matrix[i][j]);
            if (j == NR_VERTICES - 1) {
                printf("\n");
            }
        }
    }
}


short **generate_symmetric_matrix() {
    srand(time(NULL));
    short **matrix = (short **) malloc(NR_VERTICES * sizeof(short *));

    for (long i = 0; i < NR_VERTICES; ++i) {
        matrix[i] = (short *) malloc(NR_VERTICES * sizeof(short));
    }

    for (long i = 0; i < NR_VERTICES; ++i) {
        for (long j = i; j < NR_VERTICES; ++j) {
            short val = rand() % 2;

            matrix[i][j] = val;
            matrix[j][i] = val;
        }
    }

    return matrix;
}


long *bfs_linked(short **adjacency, long source) {
    long *distances = malloc(NR_VERTICES * sizeof(long));
    memset(distances, -1, NR_VERTICES * sizeof(long));

    distances[source] = 0;

    struct Node *head = NULL, *next_neighbour = NULL;
    head = (struct Node *) malloc(sizeof(struct Node));
    head->data = source;

    for (long level = 1; level < NR_VERTICES; ++level) {
        struct Node *neighbour_head = NULL, *neighbour = NULL, *next_neighbour = NULL, *node = head;

        while (node != NULL) {
            for (long i = 0; i < NR_VERTICES; ++i) {
                if (adjacency[i][node->data] > 0 && distances[i] < 0) {
                    distances[i] = level;

                    if (neighbour_head == NULL) {
                        neighbour_head = (struct Node *) malloc(sizeof(struct Node));
                        neighbour_head->data = i;
                        neighbour = neighbour_head;

                        continue;
                    }

                    next_neighbour = (struct Node *) malloc(sizeof(struct Node));

                    next_neighbour->data = i;
                    neighbour->next = next_neighbour;
                    neighbour = next_neighbour;
                }
            }

            node = node->next;
        }

        head = neighbour_head;
    }

    if (head != NULL) {
        free(head);
    }

    if (next_neighbour != NULL) {
        free(next_neighbour);
    }

    return distances;
}


long *bfs_vec(short **adjacency, long source) {
    long *distances = malloc(NR_VERTICES * sizeof(long));
    long *neighborhood = malloc(NBH_INIT_SIZE * sizeof(long));
    long *new_neighborhood = malloc(NBH_INIT_SIZE * sizeof(long));

    memset(distances, -1, NR_VERTICES * sizeof(long));

    distances[source] = 0;
    neighborhood[0] = source;
    neighborhood[1] = -1;
    long size_nbh = NBH_INIT_SIZE;

    for (long level = 1; level < NR_VERTICES; ++level) {
        long counter = 0;

        for (long i = 0; neighborhood[i] >= 0; ++i) {
            long vertex = neighborhood[i];

            for (long j = 0; j < NR_VERTICES; ++j) {
                if (adjacency[j][vertex] > 0 && distances[j] < 0) {
                    distances[j] = level;
                    new_neighborhood[counter] = j;
                    counter++;
                }

                if (counter >= size_nbh - 1) {
                    size_nbh = 2 * size_nbh;
                    new_neighborhood = realloc(new_neighborhood, size_nbh * sizeof(long));
                    neighborhood = realloc(neighborhood, size_nbh * sizeof(long));
                }
            }
        }

        new_neighborhood[counter + 1] = -1;
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
