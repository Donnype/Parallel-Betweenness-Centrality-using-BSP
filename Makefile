CC= bspcc
CFLAGS= -std=c99 -Wall -O3 -D_POSIX_C_SOURCE=199309L
LFLAGS= -lm

BFS = bfs.c

all: bfs

parallel: $(BFS)
	$(CC) $(CFLAGS) -o bfs $(PARALLEL) $(LFLAGS)

.PHONY: clean

clean:
	rm -f bfs
