#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bfs.h"


extern long NR_VERTICES;
long MAX_NR_VERTICES_PER_P;
extern long SPARSITY;
long P = 1;
short output = 0;

extern short **adjacency_matrix;
long double *all_deltas;
long **all_distances;
long source = 0;


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


long** parallel_bfs_vec() {
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
    long **sigmas = (long **) malloc(P * sizeof(long *));
    long **next_sigmas = (long **) malloc(P * sizeof(long *));

    for (int i = 0; i < P; ++i) {
        neighbourhood[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        next_neighbourhoods[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        distances[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        sigmas[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        next_sigmas[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        memset(neighbourhood[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));
        memset(next_neighbourhoods[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));
        memset(distances[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));
        memset(sigmas[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));
        memset(next_sigmas[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));

        done[i] = 0;

        bsp_push_reg(&done[i], sizeof(short));
        bsp_push_reg(neighbourhood[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(next_neighbourhoods[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(distances[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(sigmas[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(next_sigmas[i], MAX_NR_VERTICES_PER_P * sizeof(long));
    }

    bsp_sync();

    // It is convenient to keep track of the distances received by each processor separately.
    long *own_distances = (long *) malloc(MAX_NR_VERTICES_PER_P * sizeof(long));
    memset(own_distances, -1, MAX_NR_VERTICES_PER_P * sizeof(long));

    if (current_process_id == source % P) {
        // We assume that the source vertex was received from processor 0.
        neighbourhood[current_process_id][0] = source;
        next_sigmas[current_process_id][0] = 1;
    }

    for (long level = 1; level < NR_VERTICES; ++level) {
        memset(counters, 0, P * sizeof(long));

        // We loop over the nodes received from each processor.
        for (int proc = 0; proc < P; ++proc) {

            for (long index = 0; index < MAX_NR_VERTICES_PER_P; index++) {
                long vertex = neighbourhood[proc][index];
                long frequency = next_sigmas[proc][index];

                // Go to the next processor vertices when we reach the end of the list, i.e. when vertex = -1.
                if (vertex < 0) {
                    break;
                }

                long own_distance = own_distances[get_index(vertex)];

                // Keep track of how many shortest paths there are to the vertex.
                if (own_distance == level - 1) {
                    sigmas[current_process_id][get_index(vertex)] += frequency;
                }

                // Skip the vertex if we have seen it before.
                if (own_distance >= 0) {
                    continue;
                }

                // Keep track of the distances.
                own_distances[get_index(vertex)] = level - 1;

                // Collect all neighbours of the vector and to which processor they should be sent.
                for (long neighbour = 0; neighbour < NR_VERTICES; ++neighbour) {
                    if (adjacency_matrix[neighbour][vertex] > 0) {
                        short dest_proc = neighbour % P;

                        // We count how many times we send a vertex the first time for the sigmas.
                        if (distances[dest_proc][get_index(neighbour)] == level) {
                            sigmas[dest_proc][get_index(neighbour)] += sigmas[current_process_id][get_index(vertex)];

                            continue;
                        }

                        if (distances[dest_proc][get_index(neighbour)] < 0) {
                            sigmas[dest_proc][get_index(neighbour)] = 1;
                            distances[dest_proc][get_index(neighbour)] = level;
                            next_neighbourhoods[dest_proc][counters[dest_proc]] = neighbour;

                            // Keep track of the length of the vector that will be sent to dest_proc.
                            counters[dest_proc]++;
                        }
                    }
                }
            }
        }

        // Does the current processor have anything to send, or is it done at this level?
        done[current_process_id] = all_null_vec(counters);

        for (int i = 0; i < P; ++i) {
            // Aggregate the frequencies.
            for (int j = 0; j < counters[i]; ++j) {
                long neighbour = next_neighbourhoods[i][j];
                next_sigmas[i][j] = sigmas[i][get_index(neighbour)];
            }

            // Append the vectors with -1 to denote the end if they are smaller than the maximum size.
            if (counters[i] + 1 < MAX_NR_VERTICES_PER_P) {
                next_neighbourhoods[i][counters[i]] = -1;
                counters[i]++;
            }

            // Send the new neighbourhood to the relevant processor, and if this processor is done in this level.
            bsp_put(i, next_neighbourhoods[i], neighbourhood[current_process_id], 0, counters[i] * sizeof(long));
            bsp_put(i, next_sigmas[i], next_sigmas[current_process_id], 0, counters[i] * sizeof(long));
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

    // Distribute the distances of the current processor, that are complete.
    for (int i = 0; i < P; ++i) {
        bsp_put(i, own_distances, distances[current_process_id], 0, MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_put(i, sigmas[current_process_id], sigmas[current_process_id], 0, MAX_NR_VERTICES_PER_P * sizeof(long));
    }

    bsp_sync();


    // Free the variables.
    free_matrix_long(&neighbourhood, P);
    free_matrix_long(&next_neighbourhoods, P);

//    return distances;
    return sigmas;
}


void parallel_dependency() {
    bsp_begin(P);

    long current_process_id = bsp_pid();

    long double *deltas = malloc(NR_VERTICES * sizeof(long double));
    memset(deltas, 0, NR_VERTICES * sizeof(long double));

    // finding the first vertex with the largest distance
    long max_distance = 0;

    for (long i = 0; i < NR_VERTICES; i++) {
        if (all_distances[i] > max_distance) {
            max_distance = all_distances[i];
        }
    }

    // iterate over the levels, beginning at the back
    for (long d = max_distance; d > 0; d--) {
        // for each vertex.
        for (long i = 0; i < NR_VERTICES; i++) {
            if (all_distances[i] == d) {
                // first count the predecessors, then add the right fraction to delta.
                // TODO: make a list?
                long counter = 0;
                for (long j = 0; j < NR_VERTICES; j++) {
                    if (adjacency_matrix[j][i] > 0 && all_distances[j] == all_distances[i] - 1) {
                        counter++;
                    }
                }

                // printf(" %ld\t%ld\n", i, counter);
                for (long j = 0; j < NR_VERTICES; j++) {
                    if (adjacency_matrix[j][i] > 0 && all_distances[j] == all_distances[i] - 1) {
                        deltas[j] += ((long double) 1 / counter) * (deltas[i] + 1);
                    }
                }
            }
        }
    }

    bsp_end();

    all_deltas = deltas;
}


void parallel_wrap() {
    bsp_begin(P);

    long** distances = parallel_bfs_vec();

    bsp_end();

    if (output) {
        for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
            for (int j = 0; j < P; ++j) {
                printf("%ld ", distances[j][i]);
            }
        }
        printf("\n");
    }

    free(distances);
}


int main(int argc, char **argv) {
    bsp_init(parallel_wrap, argc, argv);

    int c, mat = 0, test = 0;

//    Scan the optional CLI arguments using getopt.
    while ((c = getopt (argc, argv, ":n:s:p:o:m:t:")) != -1) {
        switch (c){
            case 'n':
                NR_VERTICES = strtoul(optarg, NULL, 10);
                break;
            case 's':
                SPARSITY = strtoul(optarg, NULL, 10);
                break;
            case 'p':
                P = strtol(optarg, NULL, 10);
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

    if (test == 1) {
//        NR_VERTICES = 10;
//
//        short graph[10][10] = {
//                {0, 0, 1, 1, 1, 0, 1, 0, 1, 0},
//                {0, 1, 1, 0, 1, 0, 0, 0, 0, 1},
//                {1, 1, 1, 1, 0, 1, 1, 0, 1, 0},
//                {1, 0, 1, 1, 1, 0, 0, 1, 0, 0},
//                {1, 1, 0, 1, 1, 1, 1, 0, 0, 1},
//                {0, 0, 1, 0, 1, 0, 0, 0, 0, 1},
//                {1, 0, 1, 0, 1, 0, 0, 1, 1, 1},
//                {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
//                {1, 0, 1, 0, 0, 0, 1, 0, 1, 1},
//                {0, 1, 0, 0, 1, 1, 1, 0, 1, 0},
//        };
        NR_VERTICES = 7;

        short graph[7][7] = {
                {0, 1, 1, 1, 0, 0, 0},
                {1, 0, 0, 0, 1, 0, 0},
                {1, 0, 0, 0, 0, 1, 0},
                {1, 0, 0, 0, 0, 1, 0},
                {0, 1, 0, 0, 0, 0, 1},
                {0, 0, 1, 1, 0, 0, 1},
                {0, 0, 0, 0, 1, 1, 0},
        };

        adjacency_matrix = fill_buffer(graph);
    } else {
        adjacency_matrix = generate_symmetric_matrix();
    }

    MAX_NR_VERTICES_PER_P = NR_VERTICES / P;
    if (mat == 1) {
        print_matrix(adjacency_matrix);
    }

//    long source = 0;
//    distances = bfs_vec(adjacency_matrix, source);

//    for (long i = 0; i < NR_VERTICES; ++i) {
//        printf(" %ld", all_distances[i]);
//    }
    printf("\n");

    parallel_wrap();

//    for (long i = 0; i < NR_VERTICES; ++i) {
//        printf(" %Lf", all_deltas[i]);
//    }
//    printf("\n");

    free_matrix(&adjacency_matrix, NR_VERTICES);

    return 0;
}