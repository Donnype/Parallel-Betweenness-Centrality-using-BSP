# include <stdio.h>
#include <stdlib.h>
# include <time.h>
# include <math.h>
#include <unistd.h>
# include "bfs.h"
# include "parallel_bfs.h"
# include "dependency.h"


extern long NR_VERTICES;
extern long NBH_INIT_SIZE;
extern long MAX_NR_VERTICES_PER_P;
extern long SPARSITY;
extern long P;
extern short output;

extern short **adjacency_matrix;

double diff(struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nano_seconds = end.tv_nsec - start.tv_nsec;

    return (double) 1000.0 * seconds + (double) nano_seconds / 1000000.0;
}


double time_bfs_linked(short **matrix) {
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);

    long *d = bfs_linked(matrix, 0);

    clock_gettime(CLOCK_REALTIME, &end);

    free(d);

    return diff(start, end);
}


double time_bfs_vec(short **matrix) {
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);

    long *d = bfs_vec(matrix, 0);

    clock_gettime(CLOCK_REALTIME, &end);

    if (output) {
        for (int i = 0; i < NR_VERTICES; ++i) {
            printf("%ld ", d[i]);
        }

        printf("\n");
    }

    free(d);

    return diff(start, end);
}


double time_bfs_vec_parallel(int argc, char **argv) {
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


int bfs(int argc, char **argv) {
    double ms = 0.0, runs = 5.0;
//    printf("BFS linked list vs array: \n\n");
//    printf("BFS seq vs parallel: \n\n");
//    printf("fn\tms\n");
    printf("BFS parallel: \n\n");
    printf("p\tms\n");

    for (P = 1; P < 9; ++P) {
        MAX_NR_VERTICES_PER_P = NR_VERTICES / P;

        for (int i = 0; i < runs; ++i) {
            ms += time_bfs_vec_parallel(argc, argv);
        }

        ms = ms / runs;
        printf("%ld\t%f\n", P, ms);
        ms = 0.0;
    }


    free_matrix(&adjacency_matrix, NR_VERTICES);

    return 0;
}


int betweenness(int argc, char **argv) {
    double ms = 0.0, runs = 5.0;
    printf("Betweenness parallel: \n\n");
    printf("p\tms\n");

    for (P = 1; P < 9; P++) {
        MAX_NR_VERTICES_PER_P = NR_VERTICES / P;

        for (int i = 0; i < runs; ++i) {
            ms += time_betweenness_parallel(argc, argv);
        }

        ms = ms / runs;
        printf("%ld\t%f\n", P, ms);
        ms = 0.0;
    }


    free_matrix(&adjacency_matrix, NR_VERTICES);

    return 0;
}


int main(int argc, char **argv) {
    int c, mat = 0, test = 0;

//    Scan the optional CLI arguments using getopt.
    while ((c = getopt(argc, argv, ":i:n:s:p:o:m:t:")) != -1) {
        switch (c) {
            case 'i':
                NBH_INIT_SIZE = strtoul(optarg, NULL, 10);
                break;
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

        adjacency_matrix = fill_buffer(graph);
    } else {
        adjacency_matrix = generate_symmetric_matrix();
    }

    if (mat == 1) {
        print_matrix(adjacency_matrix);
    }

//    return bfs(argc, argv);
    return betweenness(argc, argv);

}
