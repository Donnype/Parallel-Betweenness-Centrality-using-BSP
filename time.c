# include <stdio.h>
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

    bfs_linked(matrix, 0);

    clock_gettime(CLOCK_REALTIME, &end);

    return diff(start, end);
}


double time_bfs_vec(short **matrix) {
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);

    bfs_vec(matrix, 0);

    clock_gettime(CLOCK_REALTIME, &end);

    return diff(start, end);
}


int main(int argc, char **argv) {
    int c, i;
    double ms;

//    Scan the optional CLI arguments using getopt.
    while ((c = getopt (argc, argv, ":s:p:l:f:F")) != -1) {
        switch (c){
            default:
                printf("BFS linked list vs array: \n\n");
                printf("fn\tms\n");

                short **matrix = generate_symmetric_matrix();
                ms = time_bfs_linked(matrix);
                printf("link\t%f", ms);

                ms = time_bfs_vec(matrix);
                printf("link\t%f", ms);

                return 0;
        }
    }
}
