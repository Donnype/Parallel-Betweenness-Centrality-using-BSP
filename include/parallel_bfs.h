#ifndef PARALLEL_BFC_H_
#define PARALLEL_BFC_H_

extern long P;
void parallel_wrap(int argc, char **argv);
short all_null(long vec[P]);
long get_index(long vertex);

#endif