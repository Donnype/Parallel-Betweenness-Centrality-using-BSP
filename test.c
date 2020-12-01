#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "bfs.h"
#include "parallel_bfs.h"
#include "dependency.h"

extern long NR_VERTICES;
extern long NBH_INIT_SIZE;
extern long MAX_NR_VERTICES_PER_P;
extern long SPARSITY;
extern long P;
extern short output;
extern short** adjacency_matrix;

long p_count;
long double epsilon = 0.00001;

extern long** all_distances;
extern long** all_sigmas;
extern long double** all_deltas;


int check_long(long** result, long expected[NR_VERTICES]) {
    int failed = 0;

    for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
        for (int j = 0; j < P; ++j) {
            long idx = i * P + j;

            if (result[j][i] != expected[idx]) {
                printf("Index %ld was wrong! Expected %ld vs result %ld \n", idx, expected[idx], result[j][i]);
                failed = 1;
            }
        }
    }

    return failed;
}


int check_double(long double** result, long double expected[NR_VERTICES]) {
    int failed = 0;

    for (int i = 0; i < MAX_NR_VERTICES_PER_P; ++i) {
        for (int j = 0; j < P; ++j) {
            long idx = i * P + j;

            if (fabsl(result[j][i] - expected[idx]) > epsilon) {
                printf("Index %ld was wrong! Expected %Lf vs result %Lf \n", idx, expected[idx], result[j][i]);
                failed = 1;
            }
        }
    }

    return failed;
}

void test_bfs(int argc, char**argv, long ps[], long expected[]) {
    for (int i = 0; i < p_count; ++i) {
        P = ps[i];

        MAX_NR_VERTICES_PER_P = NR_VERTICES / P;
        parallel_wrap(argc, argv);
        int failed = check_long(all_distances, expected);
        free_matrix_long(&all_distances, P);

        if (failed == 1) {
            printf("BFS test failed for P = %ld \n", P);
        } else {
            printf("BFS test succeeded for P = %ld \n", P);
        }
    }
}

void test_betweenness(int argc, char**argv, long ps[], long expected_sigmas[], long double expected_deltas[]) {
    for (int i = 0; i < p_count; ++i) {
        P = ps[i];

        MAX_NR_VERTICES_PER_P = NR_VERTICES / P;
        parallel_betweenness_wrap(argc, argv);

        int failed = check_long(all_sigmas, expected_sigmas);
        free_matrix_long(&all_sigmas, P);

        if (failed == 1) {
            printf("Betweenness test sigmas failed for P = %ld \n", P);
        } else {
            printf("Betweenness test sigmas succeeded for P = %ld \n", P);
        }

        failed = check_double(all_deltas, expected_deltas);
        free_matrix_double(&all_deltas, P);

        if (failed == 1) {
            printf("Betweenness test deltas failed for P = %ld \n", P);
        } else {
            printf("Betweenness test deltas succeeded for P = %ld \n", P);
        }

        printf("\n");
    }
}


void first_test(int argc, char**argv) {
    printf("Performing first test with 7x7 graph. \n\n");

    NR_VERTICES = 7;
    
    short graph[7][7] = {
        {0, 1, 1, 1, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0, 0, 1},
        {0, 0, 1, 1, 0, 0, 1},
        {0, 0, 0, 0, 1, 1, 0},
    };

    adjacency_matrix = fill_buffer(graph);
    long expected[] = {0, 1, 1, 1, 2, 2, 3};
    long ps[] = {1, 7};
    p_count = 2;

    test_bfs(argc, argv, ps, expected);

    printf("\n");

    long expected_sigmas[] = {1, 1, 1, 1, 1, 2, 3};
    long double expected_deltas[] = {0.0, 4.0/3.0, 5.0/6.0, 5.0/6.0, 1.0/3.0, 2.0/3.0, 0.0};

    test_betweenness(argc, argv, ps, expected_sigmas, expected_deltas);

    free_matrix(&adjacency_matrix, NR_VERTICES);
    printf("\n\n");
}


void second_test(int argc, char**argv) {
    printf("Performing second test with 10x10 graph. \n\n");

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
    long expected[] = {0, 2, 1, 1, 1, 2, 1, 2, 1, 2};
    long ps[] = {1, 2, 5};
    p_count = 3;

    test_bfs(argc, argv, ps, expected);

    printf("\n");

    long expected_sigmas[] = {1, 2, 1, 1, 1, 2, 1, 2, 1, 3};
    long double expected_deltas[] = {0.0, 0.0, 1.0, 1.0/2.0, 4.0/3.0, 0.0, 5.0/6.0, 0.0, 1.0/3.0, 0.0};

    test_betweenness(argc, argv, ps, expected_sigmas, expected_deltas);

    free_matrix(&adjacency_matrix, NR_VERTICES);

    printf("\n\n");
}


void third_test(int argc, char**argv) {
    printf("Performing third test with 9x9 graph. \n\n");

    NR_VERTICES = 9;

    short graph[9][9] = {
        {0, 1, 0, 0, 1, 0, 0, 0, 0},
        {1, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 1, 0, 1, 1, 0, 0, 0, 1},
        {0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 1, 1, 0, 1, 1},
        {0, 0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 1, 1, 0, 0, 1, 0, 0}
    };

    adjacency_matrix = fill_buffer(graph);
    long expected[] = {0, 1, 2, 3, 1, 3, 2, 3, 3};
    long ps[] = {1, 3};
    p_count = 2;

    test_bfs(argc, argv, ps, expected);

    printf("\n");

    long expected_sigmas[] = {1, 1, 2, 2, 1, 1, 1, 1, 3};
    long double expected_deltas[] = {0.0, 4.0/3.0, 5.0/3.0, 0.0, 14.0/3.0, 0.0, 7.0/3.0, 0.0, 0.0};

    test_betweenness(argc, argv, ps, expected_sigmas, expected_deltas);

    free_matrix(&adjacency_matrix, NR_VERTICES);

    printf("\n\n");
}

int main(int argc, char **argv) {
    first_test(argc, argv);
    second_test(argc, argv);
    third_test(argc, argv);

    return 0;
}