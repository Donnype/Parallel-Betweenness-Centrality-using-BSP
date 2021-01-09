#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "../include/Args.h"


Args* args;


// Scan the optional CLI arguments using getopt.
void read_args(int argc, char **argv) {
    int c;
    args = (Args*) malloc(sizeof(Args));
    args->neighbourhood_size = 10;
    args->nr_vertices = 840;
    args->sparsity = 2;
    args->nr_processors = 1;
    args->runs = 5.0;
    args->set_sparse = false;
    args->output = false;
    args->print_matrix = false;
    args->test = false;

    while ((c = getopt(argc, argv, ":i:n:s:p:r:o:S:m:t:")) != -1) {
        switch (c) {
            case 'i':
                args->neighbourhood_size = strtoul(optarg, NULL, 10);
                break;
            case 'n':
                args->nr_vertices = strtoul(optarg, NULL, 10);
                break;
            case 's':
                args->sparsity = strtoul(optarg, NULL, 10);
                break;
            case 'p':
                args->nr_processors = strtol(optarg, NULL, 10);
                break;
            case 'r':
                args->runs = strtod(optarg, NULL);
                break;
            case 'o':
                args->output = true;
                break;
            case 'S':
                args->set_sparse = true;
                break;
            case 'm':
                args->print_matrix = true;
                break;
            case 't':
                args->test = true;
                break;
        }
    }

    args->vertices_per_proc = args->nr_vertices / args->nr_processors;
};
