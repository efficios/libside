all: test

test: test.c
	gcc -O2 -g -Wall -o test test.c

.PHONY: clean

clean:
	rm -f test
