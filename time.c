# include <stdio.h>
#include <stdlib.h>
# include <time.h>
# include <math.h>
#include <unistd.h>
# include "bfs.h"


extern long NR_VERTICES;
extern long NBH_INIT_SIZE;
extern long SPARSITY;


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

    free(d);

    return diff(start, end);
}


int main(int argc, char **argv) {
    int c, runs = 5.0;
    double ms = 0.0;

//    Scan the optional CLI arguments using getopt.
    while ((c = getopt (argc, argv, ":i:n:s:")) != -1) {
        switch (c){
            case 'i':
                NBH_INIT_SIZE = strtoul(optarg, NULL, 10);
                break;
            case 'n':
                NR_VERTICES = strtoul(optarg, NULL, 10);
                break;
            case 's':
                SPARSITY = strtoul(optarg, NULL, 10);
                break;
        }
    }

    printf("BFS linked list vs array: \n\n");
    printf("fn\tms\n");

    short **matrix = generate_symmetric_matrix();

    for (int i = 0; i < runs; ++i) {
        ms += time_bfs_linked(matrix);
    }

    ms = ms / runs;
    printf("link\t%f\n", ms);
    ms = 0.0;

    for (int i = 0; i < runs; ++i) {
        ms += time_bfs_vec(matrix);
    }

    ms = ms / runs;
    printf("vec\t%f\n", ms);

    free(matrix);

    return 0;
}
