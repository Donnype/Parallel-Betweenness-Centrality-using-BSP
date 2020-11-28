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


void parallel_bfs() {
    bsp_begin(P);

    long current_process_id = bsp_pid();

    long *distances = malloc(NR_VERTICES_PER_P * sizeof(long));
    memset(distances, -1, NR_VERTICES_PER_P * sizeof(long));

    // An array containing the linked lists (i.e. their heads) received from each processor.
    Node *Stacks[P], *NewStacks[P];

    for (int i = 0; i < P; ++i) {
        Stacks[i] = create_node(-1);
        NewStacks[i] = create_node(-1);
    }

    // Set the neighborhood and distance in the processor containing the source.
    if (current_process_id == source % P) {
        long index = (source - current_process_id) / P;
        distances[index] = 0;

        // For convenience we just say that the source vertex was received in the linked list from processor 0.
        push(&Stacks[0], source);
    }

    for (long level = 1; level < NR_VERTICES && all_null(Stacks) != 1; ++level) {

        // We loop over the nodes received from each processor, starting with processor 0.
        for (int proc = 0; proc < P; ++proc) {
            long vertex = pop(&Stacks[proc]);
            long index = (vertex - current_process_id) / P;
            distances[index] = level;

            while (vertex >= 0) {
                for (long neighbour = 0; neighbour < NR_VERTICES; ++neighbour) {
                    if (adjacency_matrix[neighbour][vertex] > 0 && distances[neighbour] < 0) {
                        short dest_proc = adjacency_matrix[neighbour][vertex] - 1;

                        push(&NewStacks[dest_proc], neighbour);
                    }
                }

                vertex = pop(&Stacks[proc]);
            }
        }

        for (int i = 0; i < P; ++i) {
            bsp_push_reg(Stacks[i], sizeof(Node));
        }

        bsp_sync();

        for (int i = 0; i < P; ++i) {
            bsp_put(i, NewStacks[i], Stacks[i], 0, sizeof(Node));
        }

        bsp_sync();
    }

    bsp_end();

/*
    printf("Hello from processor %ld \n", current_process_id);

    Node *head = NULL, *next = NULL, *next_next = NULL;
    head = (Node *) malloc(sizeof(Node));
    next = (Node *) malloc(sizeof(Node));
    next_next = (Node *) malloc(sizeof(Node));

    if (current_process_id == 0){
        head->data = 1;
        next->data = 2;
        next_next->data = 3;
        head->next = next;
        next->next = next_next;
    }

    if (current_process_id == 1){
        head->data = 9;
        next->data = 8;
        next_next->data = 7;
        head->next = next;
        next->next = next_next;
    }

    Node *Head = (Node *) malloc(sizeof(Node));
    bsp_push_reg(Head, sizeof(Node));
    bsp_sync();

    bsp_put((current_process_id + 1) % P, head, Head, 0, sizeof(Node));
    bsp_sync();

    printf("Head data from processor %ld: %ld \n", current_process_id, Head->data);

    bsp_sync();

    Node Next = *Head->next;
    printf("Next data from processor %ld: %ld \n", current_process_id, Next.data);

    bsp_sync();

    Node Next_Next = *Next.next;
    printf("Head next next data from processor %ld: %ld \n", current_process_id, Next_Next.data);
    bsp_end();
    printf("Size of Node %ld \n", sizeof(Node));
*/
}


void vertex_partition(short ***M) {
    short ** matrix = *M;

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
    adjacency_matrix = generate_symmetric_matrix();

    vertex_partition(&adjacency_matrix);
    print_matrix(adjacency_matrix);

    parallel_bfs();

    if (adjacency_matrix != NULL) {
        free(adjacency_matrix);
    }
}