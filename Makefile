CC= bspcc
CFLAGS= -std=c99 -Wall -O3 -D_POSIX_C_SOURCE=199309L
LFLAGS= -lm

SCRIPTS = time.c test.c
all: $(SCRIPTS:%.c=%)
EXECUTABLES := $(SCRIPTS:%.c=%)

SRC_DIR := src
INCLUDE_DIR := include

dependency: $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c 
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(LFLAGS)

test: $(SRC_DIR)/test.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c
	$(CC) $(CFLAGS) -o test $(SRC_DIR)/test.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c $(LFLAGS)

time: $(SRC_DIR)/time.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c
	$(CC) $(CFLAGS) -o time $(SRC_DIR)/time.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c $(LFLAGS)

PARALLEL = $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c

debug: $(PARALLEL)
	$(CC) $(CFLAGS) --debug -o debug $(PARALLEL) $(LFLAGS)

memory: $(PARALLEL)
	$(CC) -std=c99 -Wall -o0 -g -o memory $(PARALLEL) $(LFLAGS)

memory_test: $(SRC_DIR)/test.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c
	$(CC) -std=c99 -Wall -o0 -g -o memory_test $(SRC_DIR)/test.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c $(LFLAGS)

memory_time: $(SRC_DIR)/time.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c
	$(CC) -std=c99 -Wall -o0 -g -o memory_time $(SRC_DIR)/time.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c $(LFLAGS)

memory_dep: dependency.c bfs.c Node.c
	$(CC) -std=c99 -Wall -o0 -g -o memory_dep $(SRC_DIR)/dependency.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(LFLAGS)

profile: $(SRC_DIR)/time.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c
	$(CC) $(CFLAGS) --profile -o profile $(SRC_DIR)/time.c $(SRC_DIR)/dependency.c $(SRC_DIR)/parallel_bfs.c $(SRC_DIR)/bfs.c $(SRC_DIR)/Node.c $(SRC_DIR)/Args.c $(SRC_DIR)/Graph.c $(LFLAGS)


.PHONY: clean

clean:
	rm -f $(EXECUTABLES) debug memory test time profile memory_time memory_dep memory_test dependency
