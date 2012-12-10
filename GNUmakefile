CC=gcc

all: tests

dsm:
	mkdir -p out
	$(CC) -c src/dsm/dsm.c src/dsm/mem.c src/dsm/ownership.c -lsigsegv
	ar rsc out/libdsm.a dsm.o mem.o ownership.o
	rm dsm.o mem.o ownership.o

tests: dsm
	$(CC) src/tests/dsm_run.c -o out/dsmrun -L out/ -ldsm -lsigsegv

clean:
	rm -rf out

