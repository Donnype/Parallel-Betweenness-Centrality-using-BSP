#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "bfs.h"
#include "Node.h"


// Number of processors requested.
long P;

extern long NR_VERTICES;
long NR_VERTICES_PER_P;

// A matrix representation of the graph, vertex partitioned.
short **adjacency_matrix;
long source;


void parallel_bfs() {
    bsp_begin(P);

    long current_process_id = bsp_pid();

    long *distances = malloc(NR_VERTICES_PER_P * sizeof(long));
    memset(distances, -1, NR_VERTICES_PER_P * sizeof(long));
/*
    bool *ints = ;

    bool *Ints = calloc(n, sizeof(bool));
    bsp_push_reg(Ints, n * sizeof(bool));
    bsp_sync();

    bsp_put(0, ints, Ints, start_block * sizeof(bool), length * sizeof(bool));
    bsp_sync();

    distances[source] = 0;

    struct Node *head = NULL, *next_neighbour = NULL, *neighbour_head = NULL, *neighbour = NULL;
    head = (struct Node *) malloc(sizeof(struct Node));
    head->data = source;

    for (long level = 1; level < NR_VERTICES; ++level) {
        struct Node *neighbour_head = NULL, *neighbour = NULL, *next_neighbour = NULL, *node = head;

        while (node != NULL) {
            for (long i = 0; i < NR_VERTICES; ++i) {
                if (adjacency_matrix[i][node->data] > 0 && distances[i] < 0) {
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

    free_variables(head, next_neighbour, neighbour_head, neighbour);
*/
    bsp_end();
}


long **vertex_partition(short ***M) {
    short ** matrix = *M;

    for (long i = 0; i < NR_VERTICES; ++i) {
        // For now we use a cyclic distribution of the vertices.
        short val = i % (P + 1);

        for (long j = i; j < NR_VERTICES; ++j) {
            if (matrix[i][j] == 1) {

                matrix[i][j] = val;
                matrix[j][i] = val;
            }
        }
    }
}


int main(int argc, char **argv) {
    bsp_init(parallel_bfs, argc, argv);

    int c;
    long n, sparsity = SPARSITY;

    // Scan the optional CLI arguments using getopt.
    while ((c = getopt (argc, argv, ":p:n:s:")) != -1) {
        switch (c){
            case 'p':
                P = strtol(optarg, NULL, 10);
                break;
            case 'n':
                n = strtoul(optarg, NULL, 10);
                break;
            case 's':
                sparsity = strtoul(optarg, NULL, 10);
                break;
        }
    }

    // If not given, ask for the required parameters.
    if (!P) {
        printf("How many processors do you want to use?\n");
        fflush(stdout);
        scanf("%ld",&P);

        if (P > bsp_nprocs()){
            printf("Sorry, only %u processors available.\n",
                   bsp_nprocs());
            fflush(stdout);
            return 1;
        }
    }

    if (!n) {
        printf("Please provide a number of vertices: \n");
        scanf("%lu", &n);
        fflush(stdin);
    }

    if (n % P != 0) {
        printf("Please make sure P divides n.");
        return 1;
    }

    NR_VERTICES = n;
    SPARSITY = sparsity;
    source = 0;
    NR_VERTICES_PER_P = NR_VERTICES / P;
    adjacency_matrix = generate_symmetric_matrix();

    vertex_partition(&adjacency_matrix);
    print_matrix(adjacency_matrix);

    if (adjacency_matrix != NULL) {
        free(adjacency_matrix);
    }
}