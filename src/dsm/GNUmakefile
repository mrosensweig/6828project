all:
	gcc -c dsm.c mem.c -lsigsegv
	ar rs ../../out/libdsm.a dsm.o mem.o
	rm dsm.o mem.o

