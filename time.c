# include <stdio.h>
#include <stdlib.h>
# include <time.h>
# include <math.h>
#include <unistd.h>
# include "bfs.h"


extern long unsigned n;
extern long P;


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
    while ((c = getopt (argc, argv, ":b")) != -1) {
        switch (c){
            default:
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
    }
}
