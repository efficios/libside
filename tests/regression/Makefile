all: side-rcu-test

HEADERS = ../../src/rcu.h

CFLAGS = -g -O2 -Wall
CPPFLAGS = -I../../include/ -D_GNU_SOURCE

side-rcu-test.o: side-rcu-test.c $(HEADERS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

side-rcu-test: side-rcu-test.o ../../src/rcu.o ../../src/smp.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lrseq

.PHONY: clean

clean:
	rm -f side-rcu-test *.o
