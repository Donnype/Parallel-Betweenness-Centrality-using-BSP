CC= bspcc
CFLAGS= -std=c99 -Wall -O3 -D_POSIX_C_SOURCE=199309L
LFLAGS= -lm

SCRIPTS = time.c test.c parallel_bfc.c
all: $(SCRIPTS:%.c=%)
EXECUTABLES := $(SCRIPTS:%.c=%)

%: %.c bfs.c
	$(CC) $(CFLAGS) -o $@ $@.c bfs.c $(LFLAGS)

.PHONY: clean

clean:
	rm -f $(EXECUTABLES)
