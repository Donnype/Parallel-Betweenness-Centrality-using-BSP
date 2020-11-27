#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "bfs.h"


// Number of processors requested.
long P;

extern long NR_VERTICES;

// A matrix representation of the graph, vertex partitioned.
short **matrix;

void parallel_bfs() {
    bsp_begin(P);

    bsp_end();
}


void vertex_partition(short ***M) {
    srand(time(NULL));
    short ** matrix = *M;

    for (long i = 0; i < NR_VERTICES; ++i) {
        for (long j = i; j < NR_VERTICES; ++j) {
            if (matrix[i][j] == 1) {
                short val = rand() % (P + 1);

                matrix[i][j] = val;
                matrix[j][i] = val;
            }
        }
    }
}


int main(int argc, char **argv) {
    int c;
    long n, sparsity = SPARSITY;

    // Scan the optional CLI arguments using getopt.
    while ((c = getopt (argc, argv, ":p:n:s:")) != -1) {
        switch (c){
            case 'p':
                P = strtol(optarg, NULL, 10);
                break;
            case 'n':
                n = strtoul(optarg, NULL, 10);
                break;
            case 's':
                sparsity = strtoul(optarg, NULL, 10);
                break;
        }
    }

    // If not given, ask for the required parameters.
    if (!P) {
        printf("How many processors do you want to use?\n");
        fflush(stdout);
        scanf("%ld",&P);

        if (P > bsp_nprocs()){
            printf("Sorry, only %u processors available.\n",
                   bsp_nprocs());
            fflush(stdout);
            return 1;
        }
    }

    if (!n) {
        printf("Please provide a number of vertices: \n");
        scanf("%lu", &n);
        fflush(stdin);
    }

    NR_VERTICES = n;
    SPARSITY = sparsity;
    matrix = generate_symmetric_matrix();

    vertex_partition(&matrix);
    print_matrix(matrix);

    if (matrix != NULL) {
        free(matrix);
    }
}