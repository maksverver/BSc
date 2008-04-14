CFLAGS=-I/usr/include/db1 -Wall -g -O0
LDLIBS=-ldb-4.5
OBJECTS=Set.o BDB_Set.o Btree_Set.o Hash_Set.o comparison.o hashing.o

all: test

test: $(OBJECTS) test.o
	$(CC) $(LDFLAGS) -o "$@" $(OBJECTS) test.o $(LDLIBS)

clean:
	rm -f $(OBJECTS) test.o

distclean: clean
	rm -f test

.PHONY: clean all

