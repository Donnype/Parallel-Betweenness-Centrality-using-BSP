#ifndef PARALLEL_BFC_H_
#define PARALLEL_BFC_H_

#include "Args.h"

extern Args* args;

void parallel_wrap(int argc, char **argv);
short all_null(long vec[args->nr_processors]);
long get_index(long vertex);
long ** allocate_and_register_matrix(long value);

#endif