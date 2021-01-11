# Parallel Betweenness Centrality

To build the time and test executables, run `make`. See the Makefile for options for debugging with valgrind (like `make memory_test`).

To test the algorithms, run

  `./test`.
  
To time them, run e.g.

  `./time -n 8400 -s 20 -S 1`

Here `n` is the number of vertices of the generated graph, `s` the sparsity (`s = 20` meaning 1 in 20 edges are present), and the `-S`
flag meaning the adjacency matrix will be used in a sparse format. For the other flags, see `include/Args.h` and `src/Args.c`.
