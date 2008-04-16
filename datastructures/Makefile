CFLAGS=-I/usr/include/db1 -Wall -g -O0
LDLIBS=-ldb-4.5
OBJECTS=FileStorage.o Set.o BDB_Set.o Btree_Set.o Hash_Set.o comparison.o hashing.o

all: test datastructures.a

datastructures.a: $(OBJECTS)
	$(AR) rcs "$@" $(OBJECTS)

test: datastructures.a
	$(CC) $(LDFLAGS) -o "$@" test.c datastructures.a $(LDLIBS)

clean:
	rm -f $(OBJECTS)

distclean: clean
	rm -f test datastructures.a

.PHONY: all clean distclean

