#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "bfs.h"
#include "Node.h"


// Number of processors requested.
extern long P;

extern long NR_VERTICES;
long MAX_NR_VERTICES_PER_P;

// A matrix representation of the graph, vertex partitioned.
extern short **adjacency_matrix;
long source = 0;
short output;


short all_null(Node* Stacks[P]) {
    for (int i = 0; i < P; ++i) {
        if (Stacks[i]->data != -1) {
            return 0;
        }
    }

    return 1;
}


short all_null_vec(long vec[P]) {
    for (int i = 0; i < P; ++i) {
        if (vec[i] != 0) {
            return 0;
        }
    }

    return 1;
}



long get_index(long vertex) {
    long offset = vertex % P;

    return (vertex - offset) / P;
}


void parallel_bfs_linked() {
    bsp_begin(P);

    long current_process_id = bsp_pid();

    long *distances = malloc(NR_VERTICES * sizeof(long));
    memset(distances, -1, NR_VERTICES * sizeof(long));
    distances[source] = 0;

    // An array containing the linked lists (i.e. their heads) received from each processor.
    Node *Stacks[P], *NewStacks[P];
    short done[P];
    for (int i = 0; i < P; ++i) {
        bsp_push_reg(&done[i], sizeof(short));
    }

    bsp_sync();

    for (int i = 0; i < P; ++i) {
        Stacks[i] = create_node(-1);
        NewStacks[i] = create_node(-1);
    }

    if (current_process_id == source % P) {
        // For convenience we just say that the source vertex was received in the linked list from processor 0.
        push(&Stacks[0], source);
    }

    for (long level = 1; level < NR_VERTICES; ++level) {

        // We loop over the nodes received from each processor, starting with processor 0.
        for (int proc = 0; proc < P; ++proc) {
            long vertex = pop(&Stacks[proc]);

            while (vertex >= 0) {
                for (long neighbour = 0; neighbour < NR_VERTICES; ++neighbour) {
                    if (adjacency_matrix[neighbour][vertex] > 0 && distances[neighbour] < 0) {
                        distances[neighbour] = level;
                        short dest_proc = neighbour % P;

                        push(&NewStacks[dest_proc], neighbour);
                    }
                }

                vertex = pop(&Stacks[proc]);
            }
        }


        for (int i = 0; i < P; ++i) {
            free(Stacks[i]);
            Stacks[i] = create_node(-1);
//            Stacks[i] = free_linked(&Stacks[i]);
            bsp_push_reg(Stacks[i], sizeof(Node));
        }
        bsp_sync();

//        bsp_push_reg(Stacks, sizeof(Node));
//        bsp_sync();

        short tmp = all_null(NewStacks);
        for (int i = 0; i < P; ++i) {
            bsp_put(i, NewStacks[i], Stacks[current_process_id], 0, sizeof(Node));
            bsp_put(i, &tmp, &done[current_process_id], 0, sizeof(short));
//            NewStacks[i] = free_linked(&NewStacks[i]);
            free(NewStacks[i]);
            NewStacks[i] = create_node(-1);
//            bsp_sync();

        }

        bsp_sync();

        short all_done = 1;

        for (int i = 0; i < P; ++i) {
            if (done[i] == 0) {
                all_done = 0;
            }
        }

//        for (int i = 0; i < P; ++i) {
//            printf("done %i in processor %ld is %i \n",i, current_process_id, done[i]);
//        }
//        printf(" all done in processor %ld is %i \n", current_process_id, all_done);
//        print_stack(*Stacks[(current_process_id + 1) % P]);

        if (all_done == 1) {
//            printf("%ld is done\n", current_process_id);
//            bsp_sync();
            break;
        }
    }

    bsp_sync();

    if (current_process_id == 0) {
        for (int i = 0; i < NR_VERTICES; ++i) {
            printf("%ld ", distances[i]);
        }
    }
//    bsp_sync();

    free(distances);
    bsp_end();
}

void parallel_bfs_vec() {
    bsp_begin(P);

    long current_process_id = bsp_pid();


    // Variable that keeps track of if processors have anything left to send.
    short done[P];
    // Variable that keeps track of the length of the message to send.
    long counters[P];

    // Allocate and register all the relevant variables.
    long **neighbourhood = (long **) malloc(P * sizeof(long *));
    long **next_neighbourhoods = (long **) malloc(P * sizeof(long *));
    long **distances = (long **) malloc(P * sizeof(long *));

    for (int i = 0; i < P; ++i) {
        neighbourhood[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        next_neighbourhoods[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        distances[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        memset(neighbourhood[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));
        memset(next_neighbourhoods[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));
        memset(distances[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));

        done[i] = 0;

        bsp_push_reg(&done[i], sizeof(short));
        bsp_push_reg(neighbourhood[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(next_neighbourhoods[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(distances[i], MAX_NR_VERTICES_PER_P * sizeof(long));
    }

    bsp_sync();

    // It is convenient to keep track of the distances received by each processor separately.
    long *own_distances = (long *) malloc(MAX_NR_VERTICES_PER_P * sizeof(long));
    memset(own_distances, -1, MAX_NR_VERTICES_PER_P * sizeof(long));

    if (current_process_id == source % P) {
        // We assume that the source vertex was received from processor 0.
        neighbourhood[current_process_id][0] = source;
    }

    for (long level = 1; level < NR_VERTICES; ++level) {
        memset(counters, 0, P * sizeof(long));

        // We loop over the nodes received from each processor.
        for (int proc = 0; proc < P; ++proc) {
            for (long index = 0; index < MAX_NR_VERTICES_PER_P; index++) {
                long vertex = neighbourhood[proc][index];

                // Go to the next processor vertices when we reach the end of the list, i.e. when vertex = -1.
                if (vertex < 0) {
                    break;
                }

                // Skip the vertex if we have seen it before.
                if (own_distances[get_index(vertex)] >= 0) {
                    continue;
                }

                // Keep track of the distances.
                own_distances[get_index(vertex)] = level - 1;

                // Collect all neighbours of the vector and to which processor they should be sent.
                for (long neighbour = 0; neighbour < NR_VERTICES; ++neighbour) {
                    if (adjacency_matrix[neighbour][vertex] > 0 && distances[neighbour % P][get_index(neighbour)] < 0) {
                        distances[neighbour % P][get_index(neighbour)] = level;
                        short dest_proc = neighbour % P;

                        next_neighbourhoods[dest_proc][counters[dest_proc]] = neighbour;

                        // Keep track of the length of the vector that will be sent to dest_proc.
                        counters[dest_proc]++;
                    }
                }
            }
        }

        // Does the current processor have anything to send, or is it done at this level?
        done[current_process_id] = all_null_vec(counters);

        for (int i = 0; i < P; ++i) {
            // Append the vectors with a -1 to denote the end if they are smaller than the maximum size.
            if (counters[i] + 1 < MAX_NR_VERTICES_PER_P) {
                next_neighbourhoods[i][counters[i]] = -1;
                counters[i]++;
            }

            // Send the new neighbourhood to the relevant processor, and if this processor is done in this level.
            bsp_put(i, next_neighbourhoods[i], neighbourhood[current_process_id], 0, counters[i] * sizeof(long));

            // If we want to aggregate the distances over all processors, this is a way to do it.
            if (output) {
                bsp_put(i, own_distances, distances[current_process_id], 0, MAX_NR_VERTICES_PER_P * sizeof(long));
            }

            bsp_put(i, &done[current_process_id], &done[current_process_id], 0, sizeof(short));
        }

        bsp_sync();

        // Check if all the processors are done and if so, move on.
        short all_done = 1;

        for (int i = 0; i < P; ++i) {
            if (done[i] == 0) {
                all_done = 0;
            }
        }

        if (all_done == 1 || level == NR_VERTICES - 1) {
            break;
        }
    }

    // Depending on a CLI argument, print the distances found in processor 0.
    if (output && current_process_id == 0) {
        for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
            for (int j = 0; j < P; ++j) {
                printf("%ld ", distances[j][i]);
            }
        }
        printf("\n");
    }


    // Free the variables.
    free(distances);
    free_matrix_long(&neighbourhood, P);
    free_matrix_long(&next_neighbourhoods, P);

    bsp_end();
}


void vertex_partition(short ***M) {
    short** matrix = *M;

    for (long i = 0; i < NR_VERTICES; ++i) {
        // For now we use a cyclic distribution of the vertices.
        short val = (i % P) + 1;

        for (long j = 0; j < NR_VERTICES; ++j) {
            if (matrix[i][j] == 1) {

                matrix[i][j] = val;
            }
        }
    }
}


void parallel_wrap(int argc, char **argv) {
    bsp_init(parallel_bfs_vec, argc, argv);

    parallel_bfs_vec();
}


/*
int main(int argc, char **argv) {
    bsp_init(parallel_bfs_vec, argc, argv);

    int c, mat = 0, test = 0;
    long n, sparsity = SPARSITY;

    // Scan the optional CLI arguments using getopt.
    while ((c = getopt(argc, argv, ":p:n:s:o:m:t:")) != -1) {
        switch (c) {
            case 'p':
                P = strtol(optarg, NULL, 10);
                break;
            case 'n':
                n = strtoul(optarg, NULL, 10);
                break;
            case 's':
                sparsity = strtoul(optarg, NULL, 10);
                break;
            case 'o':
                output = 1;
                break;
            case 'm':
                mat = 1;
                break;
            case 't':
                test = 1;
                break;
        }
    }

    // If not given, ask for the required parameters.
    if (!P) {
        printf("How many processors do you want to use?\n");
        fflush(stdout);
        scanf("%ld", &P);

        if (P > bsp_nprocs()) {
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

    NR_VERTICES = n;
    SPARSITY = sparsity;
    source = 0;
    MAX_NR_VERTICES_PER_P = NR_VERTICES / P;


    if (test == 1) {
        NR_VERTICES = 10;

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
    } else {
        adjacency_matrix = generate_symmetric_matrix();
    }

    vertex_partition(&adjacency_matrix);

    if (mat == 1) {
        print_matrix(adjacency_matrix);
    }

    parallel_bfs_vec();

    free_matrix(&adjacency_matrix, NR_VERTICES);
}
*/
