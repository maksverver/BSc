CFLAGS=-I/usr/include/db1 -Wall -g -O0
LDLIBS=-ldb-4.5
OBJECTS=Set.o BDB_Set.o Btree_Set.o Hash_Set.o comparison.o hashing.o

all: test sets.a

sets.a: $(OBJECTS)
	$(AR) rcs "$@" $(OBJECTS)

test: sets.a
	$(CC) $(LDFLAGS) -o "$@" test.c sets.a $(LDLIBS)

clean:
	rm -f $(OBJECTS)

distclean: clean
	rm -f test sets.a

.PHONY: clean all

