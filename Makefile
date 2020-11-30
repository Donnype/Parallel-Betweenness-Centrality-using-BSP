CC= bspcc
CFLAGS= -std=c99 -Wall -O3 -D_POSIX_C_SOURCE=199309L
LFLAGS= -lm

SCRIPTS = time.c test.c
all: $(SCRIPTS:%.c=%)
EXECUTABLES := $(SCRIPTS:%.c=%)


dependency: dependency.c bfs.c Node.c
	$(CC) $(CFLAGS) -o $@ dependency.c bfs.c Node.c $(LFLAGS)

%: %.c parallel_bfs.c bfs.c Node.c
	$(CC) $(CFLAGS) -o $@ $@.c parallel_bfs.c bfs.c Node.c $(LFLAGS)

PARALLEL = parallel_bfs.c bfs.c Node.c

debug: $(PARALLEL)
	$(CC) $(CFLAGS) --debug -o debug $(PARALLEL) $(LFLAGS)

memory: $(PARALLEL)
	$(CC) -std=c99 -Wall -o0 -g -o memory $(PARALLEL) $(LFLAGS)

memory_time: time.c bfs.c Node.c
	$(CC) -std=c99 -Wall -o0 -g -o memory_time time.c bfs.c Node.c $(LFLAGS)

memory_dep: dependency.c bfs.c Node.c
	$(CC) -std=c99 -Wall -o0 -g -o memory_dep dependency.c bfs.c Node.c $(LFLAGS)

profile: $(PARALLEL)
	$(CC) $(CFLAGS) --profile -o profile $(PARALLEL) $(LFLAGS)

.PHONY: clean

clean:
	rm -f $(EXECUTABLES) debug memory profile memory_time
