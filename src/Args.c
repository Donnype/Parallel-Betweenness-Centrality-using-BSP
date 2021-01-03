#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "Args.h"


Args* args;


void read_args(int argc, char **argv) {
    int c;
    args = (Args*) malloc(sizeof(Args));

//    Scan the optional CLI arguments using getopt.
    while ((c = getopt(argc, argv, ":i:n:s:p:o:m:t:")) != -1) {
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
            case 'o':
                args->output = true;
                break;
            case 'm':
                args->print_matrix = true;
                break;
            case 't':
                args->test = true;
                break;
        }
    }
};
