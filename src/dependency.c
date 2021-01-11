#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "../include/bfs.h"
#include "../include/parallel_bfs.h"
#include "../include/Args.h"
#include "../include/Graph.h"


extern Args* args;
extern Graph* graph;
extern Graph** batch;


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

            // Skip the vertex if we have seen it before.
            if (own_distances[get_index(vertex)] >= 0) {
                continue;
            }

            // The number of shortest paths to a vertex is the sum of shortest paths to its predecessors.
            own_sigmas[get_index(vertex)] += next_sigmas[proc][index];
        }
    }
}


void collect_neighbours_sparse(long **distances, long **sigmas, long *own_sigmas, long **next_neighbourhoods, long counters[], long vertex, long level, long batch_nr) {
    // Collect all neighbours of the vector and to which processor they should be sent.
    for (long i = 0; i < batch[batch_nr]->degrees[vertex]; ++i) {
        long neighbour = batch[batch_nr]->adjacency_lists[vertex][i];
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


void collect_neighbours(long **distances, long **sigmas, long *own_sigmas, long **next_neighbourhoods, long counters[], long vertex, long level, long batch_nr) {
    if (batch[batch_nr]->is_sparse) {
        return collect_neighbours_sparse(distances, sigmas, own_sigmas, next_neighbourhoods, counters, vertex, level, batch_nr);
    }

    // Collect all neighbours of the vector and to which processor they should be sent.
    for (long neighbour = 0; neighbour < args->nr_vertices; ++neighbour) {
        if (batch[batch_nr]->adjacency_matrix[neighbour][vertex] <= 0) {
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


void collect_deltas_sparse(long current_process_id, long counters[], long double **deltas, long **next_layer, long double *own_deltas, long d, long vertex, long batch_nr) {
    for (long i = 0; i < batch[batch_nr]->degrees[vertex]; ++i) {
        long neighbour = batch[batch_nr]->adjacency_lists[vertex][i];
        short dest_proc = neighbour % args->nr_processors;
        long nb_index = get_index(neighbour);

        if (batch[batch_nr]->adjacency_matrix[neighbour][vertex] <= 0 || batch[batch_nr]->distances[dest_proc][nb_index] != d - 1) {
            continue;
        }

        if (deltas[dest_proc][nb_index] == 0) {
            next_layer[dest_proc][counters[dest_proc]] = neighbour;
            counters[dest_proc]++;
        }

        long double enumerator = (long double) batch[batch_nr]->sigmas[dest_proc][nb_index];
        long double denominator = (long double) batch[batch_nr]->sigmas[current_process_id][get_index(vertex)];
        long double frac = enumerator / denominator;
        deltas[dest_proc][nb_index] += frac * (own_deltas[get_index(vertex)] + 1);
    }
}


void collect_deltas(long current_process_id, long counters[], long double **deltas, long **next_layer, long double *own_deltas, long d, long vertex, long batch_nr) {
    if (batch[batch_nr]->is_sparse) {
        return collect_deltas_sparse(current_process_id, counters, deltas, next_layer, own_deltas, d, vertex, batch_nr);
    }

    for (long neighbour = 0; neighbour < args->nr_vertices; ++neighbour) {
        short dest_proc = neighbour % args->nr_processors;
        long nb_index = get_index(neighbour);

        if (batch[batch_nr]->adjacency_matrix[neighbour][vertex] <= 0 || batch[batch_nr]->distances[dest_proc][nb_index] != d - 1) {
            continue;
        }

        if (deltas[dest_proc][nb_index] == 0.0) {
            next_layer[dest_proc][counters[dest_proc]] = neighbour;
            counters[dest_proc]++;
        }

        long double enumerator = (long double) batch[batch_nr]->sigmas[dest_proc][nb_index];
        long double denominator = (long double) batch[batch_nr]->sigmas[current_process_id][get_index(vertex)];
        long double frac = enumerator / denominator;
        deltas[dest_proc][nb_index] += frac * (own_deltas[get_index(vertex)] + 1);
    }
}


void parallel_sigmas() {
    long current_process_id = bsp_pid();

    // Variable that keeps track of if processors have anything left to send.
    long done[args->nr_processors];
    // Variable that keeps track of the length of the message to send.
    long counters[args->nr_processors];

    // Allocate and register all the relevant variables.
    long **neighbourhood = allocate_and_register_matrix(-1, true);
    long **next_neighbourhoods = allocate_and_register_matrix(-1, false);
    long **next_sigmas = allocate_and_register_matrix(0, true);

    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_push_reg(&done[i], sizeof(long));
    }

    // It is convenient to keep track of the distances received by each processor separately.
    long *own_distances = (long *) malloc(args->vertices_per_proc * sizeof(long));
    long *own_sigmas = (long *) calloc(args->vertices_per_proc, sizeof(long));

    for (int batch_nr = 0; batch_nr < args->batch_size; ++batch_nr) {
        long **distances = allocate_and_register_matrix(-1, true);
        long **sigmas = allocate_and_register_matrix(0, true);
        memset(own_distances, -1, args->vertices_per_proc * sizeof(long));
        memset(own_sigmas, 0, args->vertices_per_proc * sizeof(long));

        for (int i = 0; i < args->nr_processors; ++i) {
            neighbourhood[i][0] = -1;
            next_neighbourhoods[i][0] = -1;
            done[i] = 0;
        }

        if (current_process_id == batch[batch_nr]->source % args->nr_processors) {
            // We assume that the source vertex was received from the processor containing it.
            neighbourhood[current_process_id][0] = batch[batch_nr]->source;
            neighbourhood[current_process_id][1] = -1;
            next_sigmas[current_process_id][0] = 1;
        }

        for (long level = 1; level < args->nr_vertices; ++level) {
            bsp_sync();

            // Check if all the processors are done and if so, move on.
            if (all(done, 1)) {
                break;
            }

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
                    collect_neighbours(distances, sigmas, own_sigmas, next_neighbourhoods, counters, vertex, level, batch_nr);
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
        }

        // Distribute the distances of the current processor, that are complete.
        for (int i = 0; i < args->nr_processors; ++i) {
            bsp_put(i, own_distances, distances[current_process_id], 0, args->vertices_per_proc * sizeof(long));
            bsp_put(i, own_sigmas, sigmas[current_process_id], 0, args->vertices_per_proc * sizeof(long));
        }

        bsp_sync();

        batch[batch_nr]->distances = distances;
        batch[batch_nr]->sigmas = sigmas;

        for (int i = 0; i < args->nr_processors; ++i) {
            bsp_pop_reg(distances[i]);
            bsp_pop_reg(sigmas[i]);
        }
    }

    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_pop_reg(&done[i]);
        bsp_pop_reg(neighbourhood[i]);
        bsp_pop_reg(next_sigmas[i]);
    }

    // Free the variables.
    free_matrix_long(&neighbourhood, args->nr_processors);
    free_matrix_long(&next_neighbourhoods, args->nr_processors);
    free_matrix_long(&next_sigmas, args->nr_processors);
    free(own_distances);
    free(own_sigmas);
}


void parallel_dependency() {
    long current_process_id = bsp_pid();
    long counters[args->nr_processors];

    long double **next_deltas = allocate_and_register_matrix_double(0, true);
    long **layer = allocate_and_register_matrix(-1, true);
    long **next_layer = allocate_and_register_matrix(-1, false);

    long double* own_deltas = (long double *) calloc(args->vertices_per_proc, sizeof(long double));
    short* own_checked = (short *) calloc(args->vertices_per_proc, sizeof(short));

    for (int batch_nr = 0; batch_nr < args->batch_size; ++batch_nr) {
        // finding the first vertex with the largest distance
        long max_distance = get_max_distance(batch_nr);

        memset(counters, 0, args->nr_processors * sizeof(long));

        for (int i = 0; i < args->nr_processors; ++i) {
            layer[i][0] = -1;
            next_layer[i][0] = -1;
            memset(next_deltas[i], 0, args->vertices_per_proc * sizeof(long double));
        }

        // The processor identifies its vertices with the longest distance.
        for (int i = 0; i < args->vertices_per_proc; ++i) {
            if (batch[batch_nr]->distances[current_process_id][i] == max_distance) {
                layer[current_process_id][counters[current_process_id]] = i * args->nr_processors + current_process_id;
                counters[current_process_id]++;
            }
        }

        layer[current_process_id][counters[current_process_id]] = -1;
        counters[current_process_id]++;

        long double **deltas = allocate_and_register_matrix_double(0, true);
        memset(own_deltas, 0, args->vertices_per_proc * sizeof(long double));
        memset(own_checked, 0, args->vertices_per_proc * sizeof(short));

        bsp_sync();

        // iterate over the levels, beginning at the back
        for (long d = max_distance; d > 0; d--) {
            memset(counters, 0, args->nr_processors * sizeof(long));

            for (long proc = 0; proc < args->nr_processors; ++proc) {
                for (long index = 0; index < args->vertices_per_proc; index++) {
                    long vertex = layer[proc][index];

                    // Go to the next processor vertices when we reach the end of the list, i.e. when vertex = -1.
                    if (vertex < 0) {
                        break;
                    }

                    // Sum the deltas received per vertex.
                    own_deltas[get_index(vertex)] += next_deltas[proc][index];
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

                    collect_deltas(current_process_id, counters, deltas, next_layer, own_deltas, d, vertex, batch_nr);
                }
            }

            // Sometimes vertices of lower d have no successors, but we do have to push delta parts to predecessors.
            for (long index = 0; index < args->vertices_per_proc; index++) {
                long distance = batch[batch_nr]->distances[current_process_id][index];
                long vertex = index * args->nr_processors + current_process_id;

                // Don't fill the predecessors more than once.
                if (own_checked[index] == 1 || distance != d) {
                    continue;
                }

                own_checked[index] = 1;

                collect_deltas(current_process_id, counters, deltas, next_layer, own_deltas, d, vertex, batch_nr);
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

        deltas[batch[batch_nr]->source % args->nr_processors][get_index(batch[batch_nr]->source)] = 0.0;
        batch[batch_nr]->deltas = deltas;

        for (int i = 0; i < args->nr_processors; ++i) {
            bsp_pop_reg(deltas[i]);
        }
    }

    for (int i = 0; i < args->nr_processors; ++i) {
        bsp_pop_reg(next_deltas[i]);
        bsp_pop_reg(layer[i]);
    }

    // Free the variables.
    free_matrix_long(&layer, args->nr_processors);
    free_matrix_long(&next_layer, args->nr_processors);
    free_matrix_double(&next_deltas, args->nr_processors);
    free(own_deltas);
    free(own_checked);
}


void parallel_sigma_and_dependency() {
    bsp_begin(args->nr_processors);

    parallel_sigmas();
    parallel_dependency();

    bsp_end();

    return;
}


void parallel_dependency_wrap(int argc, char **argv) {
    bsp_init(parallel_sigma_and_dependency, argc, argv);

    parallel_sigma_and_dependency();
}


void parallel_betweenness() {
    bsp_begin(args->nr_processors);

    long current_process_id = bsp_pid();

    if (args->nr_vertices % args->batch_size != 0) {
        printf("Number of vertices is not divisible by batch size!");
        return;
    }

    long double *totals = calloc(args->vertices_per_proc, sizeof(long double));

    for (int i = 0; i < args->nr_vertices / args->batch_size; ++i) {
        parallel_sigmas();
        parallel_dependency();

        for (int batch_nr = 0; batch_nr < args->batch_size; ++batch_nr) {
            batch[batch_nr]->source = batch[batch_nr]->source + args->batch_size;

            for (int j = 0; j < args->vertices_per_proc; ++j) {
                printf("Adding %LF \n", batch[batch_nr]->deltas[current_process_id][j]);
                totals[j] += batch[batch_nr]->deltas[current_process_id][j];
            }
        }

        bsp_sync();
    }

    clean_batch_data();

    if (current_process_id == 0) {
        graph = (Graph*) malloc(sizeof(Graph));
        initialize_properties(graph);
        graph->betweennesses = allocate_and_register_matrix_double(0, true);
    }
    bsp_sync();

    bsp_put(0, totals, graph->betweennesses[current_process_id], 0, args->vertices_per_proc * sizeof(long double));

    bsp_sync();

    free(totals);

    bsp_end();

    return;
}


void parallel_betweenness_wrap(int argc, char **argv) {
    bsp_init(parallel_betweenness, argc, argv);

    parallel_betweenness();
}
