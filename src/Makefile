all: test

HEADERS = ../include/side/trace.h ../include/side/macros.h rcu.h smp.h list.h \
		../include/side/endian.h

CFLAGS = -g -O2 -Wall
CPPFLAGS = -I../include/ -D_GNU_SOURCE

side.o: side.c $(HEADERS)
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

rcu.o: rcu.c $(HEADERS)
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

smp.o: smp.c $(HEADERS)
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tracer.o: tracer.c $(HEADERS)
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

test.o: test.c $(HEADERS)
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

test: tracer.o test.o side.o rcu.o smp.o
	gcc $(CFLAGS) -o $@ $^ -lrseq

.PHONY: clean

clean:
	rm -f test *.o
