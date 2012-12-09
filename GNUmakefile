CC=gcc

all: tests

dsm:
	mkdir -p out
	$(CC) -c src/dsm/dsm.c src/dsm/mem.c -lsigsegv
	ar rsc out/libdsm.a dsm.o mem.o
	rm dsm.o mem.o

tests: dsm
	$(CC) src/tests/dsm_tests.c -o out/dsmrun -L out/ -ldsm -lsigsegv

clean:
	rm -rf out
