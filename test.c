#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "bfs.h"


extern long NR_VERTICES;


int main(int argc, char **argv) {
    /*

    Some examples to fill a buffer with:

    short graph[5][5] = {
            {0, 1, 1, 1, 0},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 1},
            {0, 0, 0, 1, 0},
    };

    */

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

    short **matrix = fill_buffer(graph);
    long s = 0;
    long *distances = bfs_vec(matrix, s);


//    srand(time(NULL));
//
//    short **matrix = generate_symmetric_matrix();
//    long s = rand() % NR_VERTICES;

    print_matrix(matrix);


//    long *distances = bfs_linked(matrix, s);
//    long *distances_2 = bfs_vec(matrix, s);
//
    for (long i = 0; i < 10; ++i) {
        printf(" %ld", distances[i]);
    }

    printf("\n");

    if (distances != NULL) {
        free(distances);
    }


//    if (distances_2 != NULL) {
//        free(distances_2);
//    }

    if (matrix != NULL) {
        free(matrix);
    }

    return 0;
}