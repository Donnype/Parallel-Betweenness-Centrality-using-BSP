CC= bspcc
CFLAGS= -std=c99 -Wall -O3 -D_POSIX_C_SOURCE=199309L
LFLAGS= -lm

SCRIPTS = time.c test.c parallel_bfc.c
all: $(SCRIPTS:%.c=%)
EXECUTABLES := $(SCRIPTS:%.c=%)

%: %.c bfs.c Node.c
	$(CC) $(CFLAGS) -o $@ $@.c bfs.c Node.c $(LFLAGS)

PARALLEL = parallel_bfc.c bfs.c Node.c

debug: $(PARALLEL)
	$(CC) $(CFLAGS) --debug -o debug $(PARALLEL) $(LFLAGS)

memory: $(PARALLEL)
	$(CC) -std=c99 -Wall -o0 -g -o memory $(PARALLEL) $(LFLAGS)

profile: $(PARALLEL)
	$(CC) $(CFLAGS) --profile -o profile $(PARALLEL) $(LFLAGS)

.PHONY: clean

clean:
	rm -f $(EXECUTABLES)
