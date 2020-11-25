#include <stdlib.h>
#include <stdio.h>
#include <time.h>
# include "bfs.h"


extern long NR_VERTICES;
extern long NBH_INIT_SIZE;

int main(int argc, char **argv) {
    /*

    Some examples:

    long graph[5][5] = {
            {0, 1, 1, 1, 0},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 1},
            {0, 0, 0, 1, 0},
    };

        0, 0, 0, 1, 0, 1, 0, 1, 0, 1
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1
        0, 0, 0, 1, 0, 0, 1, 1, 1, 1
        1, 0, 1, 0, 1, 0, 0, 1, 1, 1
        0, 0, 0, 1, 0, 0, 0, 0, 0, 0
        1, 0, 0, 0, 0, 0, 1, 0, 1, 1
        0, 1, 1, 0, 0, 1, 0, 0, 0, 0
        1, 1, 1, 1, 0, 0, 0, 0, 0, 0
        0, 1, 1, 1, 0, 1, 0, 0, 0, 0
        1, 1, 1, 1, 0, 1, 0, 0, 0, 0


    long graph[10][10] = {
            {0, 0, 0, 1, 0, 1, 0, 1, 0, 1},
            {0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
            {0, 0, 0, 1, 0, 0, 1, 1, 1, 1},
            {1, 0, 1, 0, 1, 0, 0, 1, 1, 1},
            {0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
            {1, 0, 0, 0, 0, 0, 1, 0, 1, 1},
            {0, 1, 1, 0, 0, 1, 0, 0, 0, 0},
            {1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0, 1, 0, 0, 0, 0},
            {1, 1, 1, 1, 0, 1, 0, 0, 0, 0}
    };

    */

    short **matrix = generate_symmetric_matrix();
    print_matrix(matrix);

    srand(time(NULL));
    long s = rand() % NR_VERTICES;

    long *distances = bfs_linked(matrix, s);
    long * distances_2 = bfs_vec(matrix, s);

    for (long i = 0; i < NR_VERTICES; ++i) {
        printf(" %ld\t%ld\n", distances[i], distances_2[i]);
    }

    if (distances != NULL) {
        free(distances);
    }

    if (matrix != NULL) {
        free(matrix);
    }

    return 0;
}