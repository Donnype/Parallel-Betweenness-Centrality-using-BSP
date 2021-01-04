#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include "../include/bfs.h"
#include "../include/parallel_bfs.h"
#include "../include/dependency.h"
#include "../include/Args.h"
#include "../include/Graph.h"


extern Args* args;
extern Graph* graph;


double diff(struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nano_seconds = end.tv_nsec - start.tv_nsec;

    return (double) 1000.0 * seconds + (double) nano_seconds / 1000000.0;
}


double time_bfs_linked() {
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);

    long *d = bfs_linked(0);

    clock_gettime(CLOCK_REALTIME, &end);

    free(d);

    return diff(start, end);
}


double time_bfs_vec() {
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);

    bfs_vec(0);

    clock_gettime(CLOCK_REALTIME, &end);

    if (args->output) {
        for (int i = 0; i < args->nr_vertices; ++i) {
            printf("%ld ", graph->distances[0][i]);
        }

        printf("\n");
    }

    return diff(start, end);
}


double time_bfs_parallel(int argc, char **argv) {
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);

    parallel_wrap(argc, argv);

    clock_gettime(CLOCK_REALTIME, &end);

    return diff(start, end);
}


double time_betweenness_parallel(int argc, char **argv) {
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);

    parallel_betweenness_wrap(argc, argv);

    clock_gettime(CLOCK_REALTIME, &end);

    return diff(start, end);
}


int seq_bfs() {
    double ms = 0.0;
    args->nr_processors = 1;
    args->vertices_per_proc = args->nr_vertices;

    printf("BFS sequential: \n\n");
    printf("ms\n");

    for (int i = 0; i < args->runs; ++i) {
        ms += time_bfs_vec();
    }

    ms = ms / args->runs;
    printf("%f\n", ms);

    return 0;
}


int bfs(int argc, char **argv) {
    double ms = 0.0;

    printf("BFS parallel: \n\n");
    printf("p\tms\n");

    for (args->nr_processors = 1; args->nr_processors < 9; ++args->nr_processors) {
        args->vertices_per_proc = args->nr_vertices / args->nr_processors;

        for (int i = 0; i < args->runs; ++i) {
            ms += time_bfs_parallel(argc, argv);
        }

        ms = ms / args->runs;
        printf("%ld\t%f\n", args->nr_processors, ms);
        ms = 0.0;
    }

    return 0;
}


int betweenness(int argc, char **argv) {
    double ms = 0.0;
    printf("Betweenness parallel: \n\n");
    printf("p\tms\n");

    for (args->nr_processors = 1; args->nr_processors < 9; args->nr_processors++) {
        args->vertices_per_proc = args->nr_vertices / args->nr_processors;

        for (int i = 0; i < args->runs; ++i) {
            ms += time_betweenness_parallel(argc, argv);
        }

        ms = ms / args->runs;
        printf("%ld\t%f\n", args->nr_processors, ms);
        ms = 0.0;
    }

    return 0;
}


int main(int argc, char **argv) {
    read_args(argc, argv);

    if (args->test == 1) {
        args->nr_vertices = 10;

        short adjacency[10][10] = {
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

        construct_graph(adjacency);
    } else {
        generate_graph();
    }

    if (args->print_matrix == 1) {
        print_matrix(graph->adjacency_matrix);
    }

//    return bfs(argc, argv);
    int val = seq_bfs();
//    int val = betweenness(argc, argv);

    free_graph();
    free(args);

    return val;
}
