#ifndef PARALLEL_BFC_H_
#define PARALLEL_BFC_H_

#include <stdbool.h>
#include "Args.h"

extern Args* args;

void parallel_wrap(int argc, char **argv);
short all(long vec[args->nr_processors], long value);
long get_index(long vertex);
long ** allocate_and_register_matrix(long value, bool push_register);

#endif