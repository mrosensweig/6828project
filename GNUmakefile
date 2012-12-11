CC=gcc
CFLAGS=-pthread
SOURCES=src/dsm/dsm.c src/dsm/mem.c src/dsm/ownership.c src/dsm/net.c
OBJECTS=dsm.o mem.o ownership.o net.o

all: tests

dsm:
	mkdir -p out
	$(CC) $(CFLAGS) -c $(SOURCES) -lsigsegv
	ar rsc out/libdsm.a $(OBJECTS)
	rm $(OBJECTS)

tests: dsm
	$(CC) $(CFLAGS) src/tests/dsm_run.c -o out/dsmrun -L out/ -ldsm -lsigsegv
	$(CC) $(CFLAGS) src/tests/dsm_more.c -o out/morecores -L out/ -ldsm -lsigsegv

clean:
	rm -rf out

