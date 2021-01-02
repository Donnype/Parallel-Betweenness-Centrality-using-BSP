#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/bfs.h"
#include "../include/parallel_bfs.h"


extern long NR_VERTICES;
long MAX_NR_VERTICES_PER_P;
extern long SPARSITY;
extern long P;
short output = 0;

extern short **adjacency_matrix;
extern long source;

long **all_sigmas;
long double **all_deltas;


long*** parallel_bfs() {
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
    long *own_sigmas = (long *) malloc(MAX_NR_VERTICES_PER_P * sizeof(long));
    memset(own_distances, -1, MAX_NR_VERTICES_PER_P * sizeof(long));
    memset(own_sigmas, 0, MAX_NR_VERTICES_PER_P * sizeof(long));

    if (current_process_id == source % P) {
        // We assume that the src vertex was received from the processor containing it.
        neighbourhood[current_process_id][0] = source;
        next_sigmas[current_process_id][0] = 1;
    }

    for (long level = 1; level < NR_VERTICES; ++level) {
        memset(counters, 0, P * sizeof(long));

        for (int proc = 0; proc < P; ++proc) {
            for (long index = 0; index < MAX_NR_VERTICES_PER_P; index++) {
                long vertex = neighbourhood[proc][index];
                long frequency = next_sigmas[proc][index];

                // Go to the next processor vertices when we reach the end of the list, i.e. when vertex = -1.
                if (vertex < 0) {
                    break;
                }

                if (own_distances[get_index(vertex)] >= 0) {
                    continue;
                }

                // Collect the number of shortest paths to the vertex.
                own_sigmas[get_index(vertex)] += frequency;
            }
        }

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
                    if (adjacency_matrix[neighbour][vertex] > 0) {
                        short dest_proc = neighbour % P;

                        // We count how many times we send a vertex the first time for the sigmas.
                        if (distances[dest_proc][get_index(neighbour)] == level) {
                            sigmas[dest_proc][get_index(neighbour)] += own_sigmas[get_index(vertex)];

                            continue;
                        }

                        if (distances[dest_proc][get_index(neighbour)] < 0) {
                            sigmas[dest_proc][get_index(neighbour)] = own_sigmas[get_index(vertex)];
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
            if (counters[i] < MAX_NR_VERTICES_PER_P) {
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
        bsp_put(i, own_sigmas, sigmas[current_process_id], 0, MAX_NR_VERTICES_PER_P * sizeof(long));
    }

    bsp_sync();

    for (int i = 0; i < P; ++i) {
        bsp_pop_reg(&done[i]);
        bsp_pop_reg(neighbourhood[i]);
        bsp_pop_reg(next_neighbourhoods[i]);
        bsp_pop_reg(distances[i]);
        bsp_pop_reg(sigmas[i]);
        bsp_pop_reg(next_sigmas[i]);
    }

    // Free the variables.
    free_matrix_long(&neighbourhood, P);
    free_matrix_long(&next_neighbourhoods, P);
    free_matrix_long(&next_sigmas, P);
    free(own_distances);
    free(own_sigmas);


    long*** values = malloc(2 * sizeof(long**));

    values[0] = distances;
    values[1] = sigmas;

    return values;
}


long double** parallel_dependency(long **distances, long **sigmas) {
    long current_process_id = bsp_pid();
    long counters[P];

    long double **deltas = (long double **) malloc(P * sizeof(long double*));
    long double **next_deltas = (long double **) malloc(P * sizeof(long double*));
    long **next_layer = (long **) malloc(P * sizeof(long *));
    long **layer = (long **) malloc(P * sizeof(long *));

    for (int i = 0; i < P; ++i) {
        layer[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        next_layer[i] = (long *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long));
        deltas[i] = (long double *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long double));
        next_deltas[i] = (long double *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long double));
        memset(layer[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));
        memset(next_layer[i], -1, MAX_NR_VERTICES_PER_P * sizeof(long));

        bsp_push_reg(layer[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(next_layer[i], MAX_NR_VERTICES_PER_P * sizeof(long));
        bsp_push_reg(deltas[i], MAX_NR_VERTICES_PER_P * sizeof(long double));
        bsp_push_reg(next_deltas[i], MAX_NR_VERTICES_PER_P * sizeof(long double));
    }

    bsp_sync();

    // finding the first vertex with the largest distance
    long max_distance = 0;

    for (int i = 0; i < P; ++i) {
        for (long j = 0; j < MAX_NR_VERTICES_PER_P; j++) {
            if (distances[i][j] > max_distance) {
                max_distance = distances[i][j];
            }
        }
    }

    long double* own_deltas = (long double *) calloc(MAX_NR_VERTICES_PER_P, sizeof(long double));
    short* own_checked = (short *) calloc(MAX_NR_VERTICES_PER_P, sizeof(short));

    // The processor identifies its vertices with the longest distance.
    counters[current_process_id] = 0;

    for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
        if (distances[current_process_id][i] == max_distance) {
            layer[current_process_id][counters[current_process_id]] = i * P + current_process_id;
            counters[current_process_id]++;
        }
    }

    layer[current_process_id][counters[current_process_id]] = -1;

    // iterate over the levels, beginning at the back
    for (long d = max_distance; d > 0; d--) {
        memset(counters, 0, P * sizeof(long));

        for (long proc = 0; proc < P; ++proc) {
            for (long index = 0; index < MAX_NR_VERTICES_PER_P; index++) {
                long vertex = layer[proc][index];
                long double delta_part = next_deltas[proc][index];

                // Go to the next processor vertices when we reach the end of the list, i.e. when vertex = -1.
                if (vertex < 0) {
                    break;
                }

                // Collect the number of shortest paths to the vertex.
                own_deltas[get_index(vertex)] += delta_part;
            }
        }

        for (long proc = 0; proc < P; ++proc) {
            for (long index = 0; index < MAX_NR_VERTICES_PER_P; index++) {
                long vertex = layer[proc][index];

                // Go to the next processor vertices when we reach the end of the list, i.e. when vertex = -1.
                if (vertex < 0) {
                    break;
                }

                // Don't fill the predecessors more than once.
                if (own_checked[get_index(vertex)] == 1) {
                    continue;
                }

                own_checked[get_index(vertex)] = 1;

                for (long neighbour = 0; neighbour < NR_VERTICES; ++neighbour) {
                    short dest_proc = neighbour % P;

                    if (adjacency_matrix[neighbour][vertex] > 0 && distances[dest_proc][get_index(neighbour)] == d - 1) {
                        if (deltas[dest_proc][get_index(neighbour)] == 0) {
                            next_layer[dest_proc][counters[dest_proc]] = neighbour;
                            counters[dest_proc]++;
                        }

                        long double enumerator = (long double) sigmas[dest_proc][get_index(neighbour)];
                        long double denominator = (long double) sigmas[current_process_id][get_index(vertex)];
                        long double frac = enumerator / denominator;
                        deltas[dest_proc][get_index(neighbour)] += frac * (own_deltas[get_index(vertex)] + 1);
                    }
                }
            }
        }

        // Communication of the next layer.
        for (int i = 0; i < P; ++i) {
            // Aggregate the deltas.
            for (int j = 0; j < counters[i]; ++j) {
                long neighbour = next_layer[i][j];
                next_deltas[i][j] = deltas[i][get_index(neighbour)];
            }

            // Append the vectors with -1 to denote the end if they are smaller than the maximum size.
            if (counters[i] < MAX_NR_VERTICES_PER_P) {
                next_layer[i][counters[i]] = -1;
                counters[i]++;
            }

            // Send the new neighbourhood to the relevant processor, and if this processor is done in this level.
            bsp_put(i, next_layer[i], layer[current_process_id], 0, counters[i] * sizeof(long));
            bsp_put(i, next_deltas[i], next_deltas[current_process_id], 0, counters[i] * sizeof(long double));
        }

        bsp_sync();
    }

    // Distribute the distances of the current processor, that are complete.
    for (int i = 0; i < P; ++i) {
        bsp_put(i, own_deltas, deltas[current_process_id], 0, MAX_NR_VERTICES_PER_P * sizeof(long double));
    }

    bsp_sync();

    deltas[source % P][get_index(source)] = 0.0;

    bsp_sync();

    for (int i = 0; i < P; ++i) {
        bsp_pop_reg(deltas[i]);
        bsp_pop_reg(next_deltas[i]);
        bsp_pop_reg(layer[i]);
        bsp_pop_reg(next_layer[i]);
    }

    bsp_sync();

    // Free the variables.
    free_matrix_long(&layer, P);
    free_matrix_long(&next_layer, P);
    free_matrix_double(&next_deltas, P);
    free(own_deltas);
    free(own_checked);

    return deltas;
}


void parallel_betweenness() {
    bsp_begin(P);

    long*** values = parallel_bfs();
    long** distances = values[0];
    long** sigmas = values[1];
    long double **deltas = parallel_dependency(distances, sigmas);

    long double *totals = calloc(P, sizeof(long double));
    bsp_push_reg(totals, P * sizeof(long double));
    bsp_sync();

    long current_processor_id = bsp_pid();

    for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
        totals[current_processor_id] += deltas[current_processor_id][i];
    }

    for (int i = 0; i < P; ++i) {
        bsp_put(i, &totals[current_processor_id], totals, current_processor_id * sizeof(long double), sizeof(long double));
    }

    bsp_sync();

    bsp_end();

    all_sigmas = sigmas;
    all_deltas = deltas;

    long double total = 0.0;

    for (int i = 0; i < P; ++i) {
        total += totals[i];
    }

    if (output) {
        printf("distances \n");

        for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
            for (int j = 0; j < P; ++j) {
                printf("%ld ", distances[j][i]);
            }
        }
        printf("\n");
        printf("sigmas \n");
        for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
            for (int j = 0; j < P; ++j) {
                printf("%ld ", sigmas[j][i]);
            }
        }
        printf("\n");

        printf("deltas \n");

        for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
            for (int j = 0; j < P; ++j) {
                printf("%Lf ", deltas[j][i]);
            }
        }
        printf("\n");

        printf("total is %Lf. \n", total);
    }

    free_matrix_long(&distances, P);

    free(values);
    free(totals);
}

void parallel_betweenness_wrap(int argc, char **argv) {
    bsp_init(parallel_betweenness, argc, argv);

    parallel_betweenness();
}

/*
int main(int argc, char **argv) {
    bsp_init(parallel_betweenness, argc, argv);

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
//        NR_VERTICES = 7;
//
//        short graph[7][7] = {
//                {0, 1, 1, 1, 0, 0, 0},
//                {1, 0, 0, 0, 1, 0, 0},
//                {1, 0, 0, 0, 0, 1, 0},
//                {1, 0, 0, 0, 0, 1, 0},
//                {0, 1, 0, 0, 0, 0, 1},
//                {0, 0, 1, 1, 0, 0, 1},
//                {0, 0, 0, 0, 1, 1, 0},
//        };

        adjacency_matrix = fill_buffer(graph);
    } else {
        adjacency_matrix = generate_symmetric_matrix();
    }

    MAX_NR_VERTICES_PER_P = NR_VERTICES / P;
    if (mat == 1) {
        print_matrix(adjacency_matrix);
    }

//    long src = 0;
//    distances = bfs_vec(adjacency_matrix, src);

//    for (long i = 0; i < NR_VERTICES; ++i) {
//        printf(" %ld", distances[i]);
//    }
    printf("\n");

    parallel_betweenness();

//    for (long i = 0; i < NR_VERTICES; ++i) {
//        printf(" %Lf", all_deltas[i]);
//    }
//    printf("\n");

    free_matrix(&adjacency_matrix, NR_VERTICES);

    return 0;
}*/
