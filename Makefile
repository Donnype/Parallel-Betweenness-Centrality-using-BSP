CC= bspcc
CFLAGS= -std=c99 -Wall -O3 -D_POSIX_C_SOURCE=199309L
LFLAGS= -lm

all: time test

SCRIPT = time
$(SCRIPT): $(SCRIPT).c bfs.c
	$(CC) $(CFLAGS) -o $(SCRIPT) $(SCRIPT).c bfs.c $(LFLAGS)

SCRIPT = test
$(SCRIPT): $(SCRIPT).c bfs.c
	$(CC) $(CFLAGS) -o $(SCRIPT) $(SCRIPT).c bfs.c $(LFLAGS)

.PHONY: clean

clean:
	rm -f time test
