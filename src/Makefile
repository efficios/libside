all: test

tracer.o: tracer.c
	gcc -O2 -g -Wall -I../include/ -c -o tracer.o tracer.c

test.o: test.c
	gcc -O2 -g -Wall -I../include/ -c -o test.o test.c

test: tracer.o test.o
	gcc -O2 -g -Wall -o test tracer.o test.o

.PHONY: clean

clean:
	rm -f test *.o
