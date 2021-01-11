#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "../include/bfs.h"
#include "../include/parallel_bfs.h"
#include "../include/dependency.h"
#include "../include/Args.h"
#include "../include/Graph.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define RESET   "\x1b[0m"


extern Args *args;
extern Graph *graph;
extern Graph **batch;

long p_count;
long double epsilon = 0.00001;
char out_text[80];


void print_success() {
    printf(GREEN "%s" RESET "\n", out_text);
}


void print_failure() {
    printf(RED "%s" RESET "\n", out_text);
}


int check_long(long **result, long expected[args->nr_vertices]) {
    int failed = 0;

    for (int i = 0; i < args->vertices_per_proc; ++i) {
        for (int j = 0; j < args->nr_processors; ++j) {
            long idx = i * args->nr_processors + j;

            if (result[j][i] != expected[idx]) {
                sprintf(out_text, "Index %ld was wrong! Expected %ld vs result %ld", idx, expected[idx], result[j][i]);
                print_failure();
                failed = 1;
            }
        }
    }

    return failed;
}


int check_double(long double **result, long double expected[args->nr_vertices]) {
    int failed = 0;

    for (int i = 0; i < args->vertices_per_proc; ++i) {
        for (int j = 0; j < args->nr_processors; ++j) {
            long idx = i * args->nr_processors + j;

            if (fabsl(result[j][i] - expected[idx]) > epsilon) {
                sprintf(out_text, "Index %ld was wrong! Expected %Lf vs result %Lf", idx, expected[idx], result[j][i]);
                print_failure();
                failed = 1;
            }
        }
    }

    return failed;
}

void test_bfs(int argc, char **argv, long ps[], long expected[args->batch_size][args->nr_vertices]) {
    for (int i = 0; i < p_count; ++i) {
        args->nr_processors = ps[i];
        args->vertices_per_proc = args->nr_vertices / args->nr_processors;

        parallel_wrap(argc, argv);

        for (int j = 0; j < args->batch_size; ++j) {
            int failed = check_long(batch[j]->distances, expected[j]);

            if (failed == 1) {
                sprintf(out_text, "BFS test failed for batch %i and P = %ld", j, args->nr_processors);
                print_failure();
            } else {
                sprintf(out_text, "BFS test succeeded for batch %i and P = %ld", j, args->nr_processors);
                print_success();
            }
        }

        clean_batch_data();
    }
}


void test_dependency(int argc, char **argv, long ps[], long expected_sigmas[args->batch_size][args->nr_vertices],
                     long double expected_deltas[args->batch_size][args->nr_vertices]) {
    for (int i = 0; i < p_count; ++i) {
        args->nr_processors = ps[i];
        args->vertices_per_proc = args->nr_vertices / args->nr_processors;

        parallel_dependency_wrap(argc, argv);

        for (int j = 0; j < args->batch_size; ++j) {
            int failed = check_long(batch[j]->sigmas, expected_sigmas[j]);

            if (failed == 1) {
                sprintf(out_text, "Dependency test sigmas failed for batch %i and P = %ld", j, args->nr_processors);
                print_failure();
            } else {
                sprintf(out_text, "Dependency test sigmas succeeded for batch %i and P = %ld", j, args->nr_processors);
                print_success();
            }

            failed = check_double(batch[j]->deltas, expected_deltas[j]);

            if (failed == 1) {
                sprintf(out_text, "Dependency test deltas failed for batch %i and P = %ld", j, args->nr_processors);
                print_failure();
            } else {
                sprintf(out_text, "Dependency test deltas succeeded for batch %i and P = %ld", j, args->nr_processors);
                print_success();
            }
        }

        clean_batch_data();
        printf("\n");
    }
}


void test_betweenness(int argc, char **argv, long ps[], long double expected[args->nr_vertices]) {
    for (int i = 0; i < p_count; ++i) {
        args->nr_processors = ps[i];
        args->vertices_per_proc = args->nr_vertices / args->nr_processors;

        parallel_betweenness_wrap(argc, argv);

        int failed = check_double(graph->betweennesses, expected);

        if (failed == 1) {
            sprintf(out_text, "Betweenness test failed for P = %ld", args->nr_processors);
            print_failure();
        } else {
            sprintf(out_text, "Betweenness test succeeded for P = %ld", args->nr_processors);
            print_success();
        }

        free_matrix_double(&(graph->betweennesses), args->nr_processors);

        printf("\n");
    }
}


void test_to_sparse() {
    printf("Performing to sparse test on 7x7 adjacency matrix. \n\n");

    args->nr_vertices = 7;

    short adjacency[7][7] = {
            {0, 1, 1, 1, 0, 0, 0},
            {1, 0, 0, 0, 1, 0, 0},
            {1, 0, 0, 0, 0, 1, 0},
            {1, 0, 0, 0, 0, 1, 0},
            {0, 1, 0, 0, 0, 0, 1},
            {0, 0, 1, 1, 0, 0, 1},
            {0, 0, 0, 0, 1, 1, 0},
    };

    long expected_sparse[7][3] = {
            {1, 2, 3},
            {0, 4},
            {0, 5},
            {0, 5},
            {1, 6},
            {2, 3, 6},
            {4, 5},
    };

    long expected_degrees[7] = {3, 2, 2, 2, 2, 3, 2};

    construct_graph(adjacency);
    to_sparse();

    if (!graph->is_sparse) {
        sprintf(out_text, "Matrix is_sparse flag is not set to true!");
        print_failure();
    }

    for (int j = 0; j < args->nr_vertices; ++j) {
        if (graph->degrees[j] != expected_degrees[j]) {
            sprintf(out_text, "Degrees index %i was wrong: %li != %li", j, graph->degrees[j], expected_degrees[j]);
            print_failure();
        }

        for (int i = 0; i < graph->degrees[j]; ++i) {
            if (graph->adjacency_lists[j][i] != expected_sparse[j][i]) {
                sprintf(out_text, "Column %i, row %i not as expected: %li != %li", j, i, graph->adjacency_lists[j][i],
                        expected_sparse[j][i]);
                print_failure();
                continue;
            }

            sprintf(out_text, "Column %i row %i is indeed %li", j, i, graph->adjacency_lists[j][i]);
            print_success();
        }
    }

    free_graph(graph);
}


void first_test(int argc, char **argv) {
    printf("Performing first test with 7x7 graph. \n\n");

    args->nr_vertices = 7;

    short adjacency[7][7] = {
            {0, 1, 1, 1, 0, 0, 0},
            {1, 0, 0, 0, 1, 0, 0},
            {1, 0, 0, 0, 0, 1, 0},
            {1, 0, 0, 0, 0, 1, 0},
            {0, 1, 0, 0, 0, 0, 1},
            {0, 0, 1, 1, 0, 0, 1},
            {0, 0, 0, 0, 1, 1, 0},
    };

    construct_graph(adjacency);
    to_sparse();
    create_batch();

    batch[0]->is_sparse = false;

    long expected[1][7] = {{0, 1, 1, 1, 2, 2, 3}};
    long ps[] = {1, 7};
    p_count = 2;

    test_bfs(argc, argv, ps, expected);

    printf("\nPerforming the test sparse.\n");

    batch[0]->is_sparse = true;
    test_bfs(argc, argv, ps, expected);

    long expected_sigmas[1][7] = {{1, 1, 1, 1, 1, 2, 3}};
    long double expected_deltas[1][7] = {{0.0, 4.0 / 3.0, 5.0 / 6.0, 5.0 / 6.0, 1.0 / 3.0, 2.0 / 3.0, 0.0}};

    batch[0]->is_sparse = false;
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    printf("\nPerforming the test sparse.\n");
    batch[0]->is_sparse = true;
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    free_batch();
}


void second_test(int argc, char **argv) {
    printf("Performing second test with 10x10 graph. \n\n");

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
    to_sparse();
    create_batch();

    long expected[1][10] = {{0, 2, 1, 1, 1, 2, 1, 2, 1, 2}};
    long ps[] = {1, 2, 5};
    p_count = 3;

    batch[0]->is_sparse = false;
    test_bfs(argc, argv, ps, expected);

    printf("\nPerforming the test sparse.\n");

    batch[0]->is_sparse = true;
    test_bfs(argc, argv, ps, expected);
    printf("\n");

    long expected_sigmas[1][10] = {{1, 2, 1, 1, 1, 2, 1, 2, 1, 3}};
    long double expected_deltas[1][10] = {{0.0, 0.0, 1.0, 1.0 / 2.0, 4.0 / 3.0, 0.0, 5.0 / 6.0, 0.0, 1.0 / 3.0, 0.0}};

    batch[0]->is_sparse = false;
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    printf("\nPerforming the test sparse.\n");
    batch[0]->is_sparse = true;
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    free_batch();
}


void third_test(int argc, char **argv) {
    printf("Performing third test with 9x9 graph. \n\n");

    args->nr_vertices = 9;

    short adjacency[9][9] = {
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

    construct_graph(adjacency);
    to_sparse();
    create_batch();

    long expected[1][9] = {{0, 1, 2, 3, 1, 3, 2, 3, 3}};
    long ps[] = {1, 3};
    p_count = 2;

    batch[0]->is_sparse = false;
    test_bfs(argc, argv, ps, expected);
    printf("\nPerforming the test sparse.\n");

    batch[0]->is_sparse = true;
    test_bfs(argc, argv, ps, expected);
    printf("\n");

    long expected_sigmas[1][9] = {{1, 1, 2, 2, 1, 1, 1, 1, 3}};
    long double expected_deltas[1][9] = {{0.0, 4.0 / 3.0, 5.0 / 3.0, 0.0, 14.0 / 3.0, 0.0, 7.0 / 3.0, 0.0, 0.0}};

    batch[0]->is_sparse = false;
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    printf("\nPerforming the test sparse.\n");
    batch[0]->is_sparse = true;
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    free_batch();
}


void test_batched(int argc, char **argv) {
    printf("Performing batched test with 9x9 graph. \n\n");

    args->nr_vertices = 9;
    args->batch_size = 3;

    short adjacency[9][9] = {
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

    construct_graph(adjacency);
    to_sparse();
    create_batch();

    long expected[9][9] = {
            {0, 1, 2, 3, 1, 3, 2, 3, 3},
            {1, 0, 1, 2, 2, 4, 3, 4, 2},
            {2, 1, 0, 1, 1, 3, 2, 3, 1},
            {3, 2, 1, 0, 2, 3, 2, 3, 1},
            {1, 2, 1, 2, 0, 2, 1, 2, 2},
            {3, 4, 3, 3, 2, 0, 1, 2, 2},
            {2, 3, 2, 2, 1, 1, 0, 1, 1},
            {3, 4, 3, 3, 2, 2, 1, 0, 2},
            {3, 2, 1, 1, 2, 2, 1, 2, 0},
    };

    long ps[] = {1, 3};
    p_count = 2;

    for (int j = 0; j < args->batch_size; ++j) {
        batch[j]->is_sparse = false;
    }

    test_bfs(argc, argv, ps, expected);
    printf("\nPerforming the test sparse.\n");

    for (int j = 0; j < args->batch_size; ++j) {
        batch[j]->is_sparse = true;
    }
    test_bfs(argc, argv, ps, expected);
    printf("\n");

    long expected_sigmas[9][9] = {
            {1, 1, 2, 2, 1, 1, 1, 1, 3},
            {1, 1, 1, 1, 2, 3, 3, 3, 1},
            {2, 1, 1, 1, 1, 2, 2, 2, 1},
            {2, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 2, 1, 1, 1, 1, 1, 1, 2},
            {1, 3, 2, 1, 1, 1, 1, 1, 1},
            {1, 3, 2, 1, 1, 1, 1, 1, 1},
            {1, 3, 2, 1, 1, 1, 1, 1, 1},
            {3, 1, 1, 1, 2, 1, 1, 1, 1},
    };
    long double expected_deltas[9][9] = {
            {0.0,       4.0 / 3.0, 5.0 / 3.0,  0.0, 14.0 / 3.0, 0.0, 7.0 / 3.0,  0.0, 0.0},
            {3.0 / 2.0, 0.0,       9.0 / 2.0,  0.0, 2.0,        0.0, 2.0,        0.0, 1.0},
            {0.0,       1.0 / 2.0, 0.0,        0.0, 2.0,        0.0, 2.0,        0.0, 3.0 / 2.0},
            {0.0,       1.0 / 2.0, 3.0,        0.0, 1.0 / 2.0,  0.0, 2.0,        0.0, 3.0},
            {1.0 / 2.0, 0.0,       2.0,        0.0, 0.0,        0.0, 5.0 / 2.0,  0.0, 0.0},
            {1.0 / 3.0, 0.0,       2.0 / 3.0,  0.0, 13.0 / 6.0, 0.0, 7.0,        0.0, 11.0 / 6.0},
            {1.0 / 3.0, 0.0,       2.0 / 3.0,  0.0, 13.0 / 6.0, 0.0, 0.0,        0.0, 11.0 / 6.0},
            {1.0 / 3.0, 0.0,       2.0 / 3.0,  0.0, 13.0 / 6.0, 0.0, 7.0,        0.0, 11.0 / 6.0},
            {0.0,       1.0 / 3.0, 13.0 / 6.0, 0.0, 2.0 / 3.0,  0.0, 17.0 / 6.0, 0.0, 0.0},
    };

    for (int j = 0; j < args->batch_size; ++j) {
        batch[j]->is_sparse = false;
    }
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    printf("\nPerforming the test sparse.\n");

    for (int j = 0; j < args->batch_size; ++j) {
        batch[j]->is_sparse = true;
    }
    test_dependency(argc, argv, ps, expected_sigmas, expected_deltas);

    printf("\nPerforming betweenness test.\n");

    long double expected_betweennesses[9] = {3.0, 8.0 / 3.0, 46.0 / 3.0, 0.0, 98.0 / 6.0, 0.0, 83.0 / 3.0, 0.0, 11.0};
    test_betweenness(argc, argv, ps, expected_betweennesses);

    free_batch();
}


int main(int argc, char **argv) {
    read_args(argc, argv);
    args->batch_size = 1;
    args->set_sparse = false;

    test_to_sparse();
    printf("\n");
    first_test(argc, argv);
    printf("\n");
    second_test(argc, argv);
    printf("\n");
    third_test(argc, argv);
    printf("\n");
    test_batched(argc, argv);

    free(args);
    return 0;
}