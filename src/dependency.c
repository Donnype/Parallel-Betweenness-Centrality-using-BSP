#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/bfs.h"
#include "../include/parallel_bfs.h"
#include "../include/Args.h"
#include "../include/Graph.h"


extern Args* args;
extern Graph* graph;
extern long source;


long double ** allocate_and_register_matrix_double(long double value, bool push_register) {
    long double ** matrix = (long double **) malloc(args->nr_processors * sizeof(long double *));

    for (int i = 0; i < args->nr_processors; ++i) {
        matrix[i] = (long double *) calloc(args->vertices_per_proc, sizeof(long double));

        if (value != 0) {
            memset(matrix[i], value, args->vertices_per_proc * sizeof(long double));
        }

        if (push_register) {
            bsp_push_reg(matrix[i], args->vertices_per_proc * sizeof(long double));
        }
    }

    return matrix;
}


void update_sigmas(long *own_sigmas, long *own_distances, long **neighbourhood, long **next_sigmas) {
    for (int proc = 0; proc < args->nr_processors; ++proc) {
        for (long index = 0; index < args->vertices_per_proc; index++) {
            long vertex = neighbourhood[proc][index];

            if (vertex < 0) {
                break;
            }

            long frequency = next_sigmas[proc][index];

            // Skip the vertex if we have seen it before.
            if (own_distances[get_index(vertex)] >= 0) {
                continue;
            }

            // The number of shortest paths to a vertex is the sum of shortest paths to its predecessors.
            own_sigmas[get_index(vertex)] += frequency;
        }
    }
}


void collect_neighbours(long **distances, long **sigmas, long *own_sigmas, long **next_neighbourhoods, long counters[], long vertex, long level) {
    // Collect all neighbours of the vector and to which processor they should be sent.
    for (long neighbour = 0; neighbour < args->nr_vertices; ++neighbour) {
        if (graph->adjacency_matrix[neighbour][vertex] <= 0) {
            continue;
        }

        short dest_proc = neighbour % args->nr_processors;

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


void parallel_sigmas() {
    bsp_begin(args->nr_processors);

    long current_process_id = bsp_pid();

    // Variable that keeps track of if processors have anything left to send.
    long done[args->nr_processors];
    // Variable that keeps track of the length of the message to send.
    long counters[args->nr_processors];

    // Allocate and register all the relevant variables.
    long **neighbourhood = allocate_and_register_matrix(-1, true);
    long **next_neighbourhoods = allocate_and_register_matrix(-1, false);
    long **distances = allocate_and_register_matrix(-1, true);
    long **sigmas = allocate_and_register_matrix(0, true);
    long **next_sigmas = allocate_and_register_matrix(0, true);

    for (int i = 0; i < args->nr_processors; ++i) {
        done[i] = 0;
        bsp_push_reg(&done[i], sizeof(long));
    }

    bsp_sync();

    // It is convenient to keep track of the distances received by each processor separately.
    long *own_distances = (long *) malloc(args->vertices_per_proc * sizeof(long));
    memset(own_distances, -1, args->vertices_per_proc * sizeof(long));
    long *own_sigmas = (long *) calloc(args->vertices_per_proc, sizeof(long));

    if (current_process_id == source % args->nr_processors) {
        // We assume that the source vertex was received from the processor containing it.
        neighbourhood[current_process_id][0] = source;
        next_sigmas[current_process_id][0] = 1;
    }

    for (long level = 1; level < args->nr_vertices; ++level) {
        memset(counters, 0, args->nr_processors * sizeof(long));
        update_sigmas(own_sigmas, own_distances, neighbourhood, next_sigmas);

        // We loop over the nodes received from each processor.
        for (int proc = 0; proc < args->nr_processors; ++proc) {
            for (long index = 0; index < args->vertices_per_proc; index++) {
                long vertex = neighbourhood[proc][index];

                if (vertex < 0) {
                    break;
                }

                if (own_distances[get_index(vertex)] >= 0) {
                    continue;
                }

                own_distances[get_index(vertex)] = level - 1;
                collect_neighbours(distances, sigmas, own_sigmas, next_neighbourhoods, counters, vertex, level);
            }
        }

        if (level == args->nr_vertices - 1) {
            break;
        }

        // Does the current processor have anything to send, or is it done at this level?
        done[current_process_id] = all(counters, 0);

        for (int i = 0; i < args->nr_processors; ++i) {
            // Aggregate the frequencies.
            for (int j = 0; j < counters[i]; ++j) {
                long neighbour = next_neighbourhoods[i][j];
                next_sigmas[i][j] = sigmas[i][get_index(neighbour)];
            }

            // Append the vectors with -1 to denote the end if they are smaller than the maximum size.
            if (counters[i] < args->vertices_per_proc) {
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
        if (all(done, 1)) {
            break;
        }
    }

    // Distribute the distances of the current processor, that are complete.
    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_put(i, own_distances, distances[current_process_id], 0, args->vertices_per_proc * sizeof(long));
        bsp_put(i, own_sigmas, sigmas[current_process_id], 0, args->vertices_per_proc * sizeof(long));
    }

    bsp_sync();

    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_pop_reg(&done[i]);
        bsp_pop_reg(neighbourhood[i]);
        bsp_pop_reg(distances[i]);
        bsp_pop_reg(sigmas[i]);
        bsp_pop_reg(next_sigmas[i]);
    }

    // Free the variables.
    free_matrix_long(&neighbourhood, args->nr_processors);
    free_matrix_long(&next_neighbourhoods, args->nr_processors);
    free_matrix_long(&next_sigmas, args->nr_processors);
    free(own_distances);
    free(own_sigmas);

    graph->distances = distances;
    graph->sigmas = sigmas;
}


void parallel_dependency() {
    long current_process_id = bsp_pid();
    long counters[args->nr_processors];

    long double **deltas = allocate_and_register_matrix_double(0, true);
    long double **next_deltas = allocate_and_register_matrix_double(0, true);
    long **layer = allocate_and_register_matrix(-1, true);
    long **next_layer = allocate_and_register_matrix(-1, false);

    bsp_sync();

    // finding the first vertex with the largest distance
    long max_distance = get_max_distance();

    long double* own_deltas = (long double *) calloc(args->vertices_per_proc, sizeof(long double));
    short* own_checked = (short *) calloc(args->vertices_per_proc, sizeof(short));

    // The processor identifies its vertices with the longest distance.
    counters[current_process_id] = 0;

    for (int i = 0; i < args->vertices_per_proc; ++i) {
        if (graph->distances[current_process_id][i] == max_distance) {
            layer[current_process_id][counters[current_process_id]] = i * args->nr_processors + current_process_id;
            counters[current_process_id]++;
        }
    }

    layer[current_process_id][counters[current_process_id]] = -1;

    // iterate over the levels, beginning at the back
    for (long d = max_distance; d > 0; d--) {
        memset(counters, 0, args->nr_processors * sizeof(long));

        for (long proc = 0; proc < args->nr_processors; ++proc) {
            for (long index = 0; index < args->vertices_per_proc; index++) {
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

        for (long proc = 0; proc < args->nr_processors; ++proc) {
            for (long index = 0; index < args->vertices_per_proc; index++) {
                long vertex = layer[proc][index];

                if (vertex < 0) {
                    break;
                }

                // Don't fill the predecessors more than once.
                if (own_checked[get_index(vertex)] == 1) {
                    continue;
                }

                own_checked[get_index(vertex)] = 1;

                for (long neighbour = 0; neighbour < args->nr_vertices; ++neighbour) {
                    short dest_proc = neighbour % args->nr_processors;
                    long nb_index = get_index(neighbour);

                    if (graph->adjacency_matrix[neighbour][vertex] <= 0 || graph->distances[dest_proc][nb_index] != d - 1) {
                        continue;
                    }

                    if (deltas[dest_proc][nb_index] == 0) {
                        next_layer[dest_proc][counters[dest_proc]] = neighbour;
                        counters[dest_proc]++;
                    }

                    long double enumerator = (long double) graph->sigmas[dest_proc][nb_index];
                    long double denominator = (long double) graph->sigmas[current_process_id][get_index(vertex)];
                    long double frac = enumerator / denominator;
                    deltas[dest_proc][nb_index] += frac * (own_deltas[get_index(vertex)] + 1);
                }
            }
        }

        // Communication of the next layer.
        for (int i = 0; i < args->nr_processors; ++i) {
            // Aggregate the deltas.
            for (int j = 0; j < counters[i]; ++j) {
                long neighbour = next_layer[i][j];
                next_deltas[i][j] = deltas[i][get_index(neighbour)];
            }

            // Append the vectors with -1 to denote the end if they are smaller than the maximum size.
            if (counters[i] < args->vertices_per_proc) {
                next_layer[i][counters[i]] = -1;
                counters[i]++;
            }

            // Send the new neighbourhood to the relevant processor, and if this processor is done in this level.
            bsp_put(i, next_layer[i], layer[current_process_id], 0, counters[i] * sizeof(long));
            bsp_put(i, next_deltas[i], next_deltas[current_process_id], 0, counters[i] * sizeof(long double));
        }

        bsp_sync();
    }

    // Distribute the deltas of the current processor, that are complete.
    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_put(i, own_deltas, deltas[current_process_id], 0, args->vertices_per_proc * sizeof(long double));
    }

    bsp_sync();

    deltas[source % args->nr_processors][get_index(source)] = 0.0;

    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_pop_reg(deltas[i]);
        bsp_pop_reg(next_deltas[i]);
        bsp_pop_reg(layer[i]);
    }

    // Free the variables.
    free_matrix_long(&layer, args->nr_processors);
    free_matrix_long(&next_layer, args->nr_processors);
    free_matrix_double(&next_deltas, args->nr_processors);
    free(own_deltas);
    free(own_checked);

    graph->deltas = deltas;
}


void parallel_betweenness() {
    bsp_begin(args->nr_processors);

    parallel_sigmas();
    parallel_dependency();

    long double *totals = calloc(args->nr_processors, sizeof(long double));
    bsp_push_reg(totals, args->nr_processors * sizeof(long double));
    bsp_sync();

    long current_processor_id = bsp_pid();

    for (int i = 0; i < args->vertices_per_proc; ++i) {
        totals[current_processor_id] += graph->deltas[current_processor_id][i];
    }

    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_put(i, &totals[current_processor_id], totals, current_processor_id * sizeof(long double), sizeof(long double));
    }

    bsp_end();

    long double total = 0.0;

    for (int i = 0; i < args->nr_processors; ++i) {
        total += totals[i];
    }

    if (args->output) {
        print_graph();
        printf("total is %Lf. \n", total);
    }

    free(totals);
}


void parallel_betweenness_wrap(int argc, char **argv) {
    bsp_init(parallel_betweenness, argc, argv);

    parallel_betweenness();
}
