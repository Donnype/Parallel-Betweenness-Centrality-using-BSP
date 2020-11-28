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

    // We simultaneously read from and build a linked list representing the current and next "layer"
    // of constant distance respectively.
    Node *head = NULL, *next_neighbour = NULL, *neighbour_head = NULL, *neighbour = NULL;

    // An array containing the linked lists (i.e. their heads) received from each processor.
    Node *Heads[P];

    for (int i = 0; i < P; ++i) {
        Heads[i] = NULL;
    }

    // Set the neighborhood and distance in the processor containing the source.
    if (current_process_id == source % P) {
        head = (Node *) malloc(sizeof(Node));
        head->data = source;

        long index = (source - current_process_id) / P;
        distances[index] = 0;

        // For convenience we just say that the source vertex was received in the linked list  from processor 0.
        Heads[0] = head;
    }

    for (long level = 1; level < NR_VERTICES; ++level) {
        if (head == NULL) {
            break;
        }

        neighbour_head = NULL;
        next_neighbour = NULL;
        neighbour = NULL;

        // We loop over the nodes received from each processor, starting with processor 0.
        for (int proc = 0; proc < P; ++proc) {
            Node *node = Heads[proc];

            if (node != NULL) {
                printf("node data is %ld\n", node->data);
            }
            /*while (node != NULL) {
                for (long i = 0; i < NR_VERTICES; ++i) {
                    if (adjacency_matrix[i][node->data] > 0 && distances[i] < 0) {
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
            }*/
        }

//        bsp_sync();
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