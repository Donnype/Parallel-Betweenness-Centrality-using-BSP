#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "../include/bfs.h"
#include "../include/Node.h"
#include "../include/Args.h"
#include "../include/Graph.h"


extern Args* args;
extern Graph* graph;
extern Graph** batch;


short all(long vec[args->nr_processors], long value) {
    for (int i = 0; i < args->nr_processors; ++i) {
        if (vec[i] != value) {
            return 0;
        }
    }

    return 1;
}


long get_index(long vertex) {
    long offset = vertex % args->nr_processors;

    return (vertex - offset) / args->nr_processors;
}


long ** allocate_and_register_matrix(long value, bool push_register) {
    long ** matrix = (long **) malloc(args->nr_processors * sizeof(long *));

    for (int i = 0; i < args->nr_processors; ++i) {
        matrix[i] = (long *) calloc(args->vertices_per_proc, sizeof(long));

        if (value != 0) {
            memset(matrix[i], value, args->vertices_per_proc * sizeof(long));
        }

        if (push_register) {
            bsp_push_reg(matrix[i], args->vertices_per_proc * sizeof(long));
        }
    }

    return matrix;
}


void sparse_collect_neighbours(long **next_neighbourhoods, long counters[], long **distances, long vertex, long level, long batch_nr) {
    for (long i = 0; i < batch[batch_nr]->degrees[vertex]; ++i) {
        long neighbour = batch[batch_nr]->adjacency_lists[vertex][i];

        if (distances[neighbour % args->nr_processors][get_index(neighbour)] >= 0) {
            continue;
        }

        short dest_proc = neighbour % args->nr_processors;

        distances[dest_proc][get_index(neighbour)] = level;
        next_neighbourhoods[dest_proc][counters[dest_proc]] = neighbour;

        // Keep track of the length of the vector that will be sent to dest_proc.
        counters[dest_proc]++;
    }
}


void parallel_bfs() {
    bsp_begin(args->nr_processors);

    long current_process_id = bsp_pid();

    // Variable that keeps track of if processors have anything left to send.
    long done[args->nr_processors];
    // Variable that keeps track of the length of the message to send.
    long counters[args->nr_processors];

    // Allocate and register all the relevant variables.
    long **neighbourhood = allocate_and_register_matrix(-1, true);
    long **next_neighbourhoods = allocate_and_register_matrix(-1, true);

    for (int i = 0; i < args->nr_processors; ++i) {
        done[i] = 0;
        bsp_push_reg(&done[i], sizeof(long));
    }

    // It is convenient to keep track of the distances received by each processor separately.
    long *own_distances = (long *) malloc(args->vertices_per_proc * sizeof(long));
    memset(own_distances, -1, args->vertices_per_proc * sizeof(long));

    for (int j = 0; j < args->batch_size; ++j) {
        long **distances = allocate_and_register_matrix(-1, true);
        bsp_sync();

        if (current_process_id == batch[j]->source % args->nr_processors) {
            // We assume that the src vertex was received from processor 0.
            neighbourhood[current_process_id][0] = batch[j]->source;
        }

        for (long level = 1; level < args->nr_vertices; ++level) {
            memset(counters, 0, args->nr_processors * sizeof(long));

            // We loop over the nodes received from each processor.
            for (int proc = 0; proc < args->nr_processors; ++proc) {
                for (long index = 0; index < args->vertices_per_proc; index++) {
                    long vertex = neighbourhood[proc][index];

                    // Go to the next processor vertices when we reach the end of the list, i.e. when vertex = -1.
                    if (vertex < 0) {
                        break;
                    }

                    // Skip the vertex if we have received it before.
                    if (own_distances[get_index(vertex)] >= 0) {
                        continue;
                    }

                    // Keep track of the distances.
                    own_distances[get_index(vertex)] = level - 1;

                    // Collect all neighbours of the vector and to which processor they should be sent.
                    if (batch[j]->is_sparse) {
                        sparse_collect_neighbours(next_neighbourhoods, counters, distances, vertex, level, j);
                        continue;
                    }

                    for (long neighbour = 0; neighbour < args->nr_vertices; ++neighbour) {
                        if (batch[j]->adjacency_matrix[neighbour][vertex] <= 0 || distances[neighbour % args->nr_processors][get_index(neighbour)] >= 0) {
                            continue;
                        }

                        short dest_proc = neighbour % args->nr_processors;

                        distances[dest_proc][get_index(neighbour)] = level;
                        next_neighbourhoods[dest_proc][counters[dest_proc]] = neighbour;

                        // Keep track of the length of the vector that will be sent to dest_proc.
                        counters[dest_proc]++;
                    }
                }
            }

            if (level == args->nr_vertices - 1) {
                break;
            }

            // Does the current processor have anything to send, or is it done at this level?
            done[current_process_id] = all(counters, 0);

            for (int i = 0; i < args->nr_processors; ++i) {
                // Append the vectors with a -1 to denote the end if they are smaller than the maximum size.
                if (counters[i] + 1 < args->vertices_per_proc) {
                    next_neighbourhoods[i][counters[i]] = -1;
                    counters[i]++;
                }

                // Send the new neighbourhood to the relevant processor, and if this processor is done in this level.
                bsp_put(i, next_neighbourhoods[i], neighbourhood[current_process_id], 0, counters[i] * sizeof(long));
                bsp_put(i, &done[current_process_id], &done[current_process_id], 0, sizeof(long));
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
        }
        bsp_sync();

        batch[j]->distances = distances;

        // Depending on a CLI argument, print the distances found in processor 0.
        if (args->output && current_process_id == 0) {
            print_graph_values(batch[j]->distances);
        }
    }

    // Free the variables.
    free_matrix_long(&neighbourhood, args->nr_processors);
    free_matrix_long(&next_neighbourhoods, args->nr_processors);
    free(own_distances);

    bsp_end();
}


void parallel_wrap(int argc, char **argv) {
    bsp_init(parallel_bfs, argc, argv);

    parallel_bfs();
}
