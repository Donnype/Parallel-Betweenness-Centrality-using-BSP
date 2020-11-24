#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

const long NR_VERTICES = 5;

struct Node {
    long data;
    struct Node *next;
};

void print_matrix(long matrix[NR_VERTICES][NR_VERTICES]) {
    printf("Two Dimensional array elements:\n");

    for (long i = 0; i < NR_VERTICES; i++) {
        for (long j = 0; j < NR_VERTICES; j++) {
            printf("%ld ", matrix[i][j]);
            if (j == NR_VERTICES - 1) {
                printf("\n");
            }
        }
    }
}


long *bfs(long adjacency[NR_VERTICES][NR_VERTICES], long source) {
    long *distances = malloc(NR_VERTICES * sizeof(long));

    for (long i = 0; i < NR_VERTICES; ++i) {
        distances[i] = -1;
    }

    distances[source] = 0;
    struct Node *head = NULL;
    head = (struct Node *) malloc(sizeof(struct Node));
    head->data = source;

    for (long level = 1; level < NR_VERTICES; ++level) {
        struct Node *neighbour_head = NULL;
        struct Node *neighbour = NULL;
        struct Node *node = head;

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

                    struct Node *next_neighbour = NULL;
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

    return distances;
}


int main(int argc, char **argv) {
    long graph[5][5] = {
            {0, 1, 1, 1, 0},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 1},
            {0, 0, 0, 1, 0},
    };

    long s = 0;
    long *distances = bfs(graph, s);

    for (long i = 0; i < NR_VERTICES; ++i) {
        printf(" %ld \n", distances[i]);
    }

    if (distances != NULL) {
        free(distances);
    }

    return 0;
}
