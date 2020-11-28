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


int all_null(Node* Stacks[P]) {
    for (int i = 0; i < P; ++i) {
        if (Stacks[i]->data != -1) {
            return 0;
        }
    }

    return 1;
}


void free_linked(Node** stack) {
    long tmp = 1;

    while (tmp != -1) {
        tmp = pop(stack);
    }
}


void free_matrix(short*** M) {
    short** matrix = *M;

    for (int i = 0; i < NR_VERTICES; ++i) {
        if (matrix[i] != NULL) {
            free(matrix[i]);
        }
    }

    if (matrix != NULL) {
        free(matrix);
    }
}


void parallel_bfs() {
    bsp_begin(P);

    long current_process_id = bsp_pid();

    long *distances = malloc(NR_VERTICES * sizeof(long));
    memset(distances, -1, NR_VERTICES * sizeof(long));
    distances[source] = 0;

    // An array containing the linked lists (i.e. their heads) received from each processor.
    Node *Stacks[P], *NewStacks[P], *Tmps[P];

    for (int i = 0; i < P; ++i) {
        Stacks[i] = create_node(-1);
        NewStacks[i] = create_node(-1);
    }

    if (current_process_id == source % P) {
        // For convenience we just say that the source vertex was received in the linked list from processor 0.
        push(&Stacks[0], source);
    }

    for (long level = 1; level < NR_VERTICES && all_null(Stacks) != 1; ++level) {

        // We loop over the nodes received from each processor, starting with processor 0.
        for (int proc = 0; proc < P; ++proc) {
            long vertex = pop(&Stacks[proc]);

            while (vertex >= 0) {
                for (long neighbour = 0; neighbour < NR_VERTICES; ++neighbour) {
                    if (adjacency_matrix[neighbour][vertex] > 0 && distances[neighbour] < 0) {
                        distances[neighbour] = level;
                        short dest_proc = adjacency_matrix[neighbour][vertex] - 1;

                        push(&NewStacks[dest_proc], neighbour);
                    }
                }

                vertex = pop(&Stacks[proc]);
            }
        }

        bsp_sync();

        for (int i = 0; i < P; ++i) {
            bsp_push_reg(Stacks[i], sizeof(Node));
        }

        bsp_sync();

        for (int i = 0; i < P; ++i) {
            bsp_put(i, NewStacks[i], Stacks[current_process_id], 0, sizeof(Node));
            print_stack(*NewStacks[i]);
//            free_linked(&NewStacks[i]);
            free(NewStacks[i]);
            NewStacks[i] = create_node(-1);
        }

        bsp_sync();
    }

    bsp_sync();

    for (int i = 0; i < NR_VERTICES; ++i) {
        printf("%ld ", distances[i]);
    }

    free(distances);

    bsp_end();
}


void vertex_partition(short ***M) {
    short** matrix = *M;

    for (long i = 0; i < NR_VERTICES; ++i) {
        // For now we use a cyclic distribution of the vertices.
        short val = (i % P) + 1;

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
        printf("Please make sure P divides n.\n");
        return 1;
    }

    NR_VERTICES = n;
    SPARSITY = sparsity;
    source = 0;
    NR_VERTICES_PER_P = NR_VERTICES / P;

    short graph[10][10] = {
            {0, 0, 1, 1, 1, 0, 1, 0, 1, 0},
            {0, 1, 1, 0, 1, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 0, 1, 1, 0, 1, 0},
            {1, 0, 1, 1, 1, 0, 0, 1, 0, 0},
            {1, 1, 0, 1, 1, 1, 1, 0, 0, 1},
            {0, 0, 1, 0, 1, 0, 0, 0, 0, 1},
            {1, 0, 1, 0, 1, 0, 0, 1, 1, 1},
            {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
            {1, 0, 1, 0, 0, 0, 1, 0, 1, 1},
            {0, 1, 0, 0, 1, 1, 1, 0, 1, 0},
    };

    adjacency_matrix = fill_buffer(graph);

//    adjacency_matrix = generate_symmetric_matrix();
    printf("p is %ld\n", P);

    vertex_partition(&adjacency_matrix);
    print_matrix(adjacency_matrix);

    parallel_bfs();

    free_matrix(&adjacency_matrix);
}