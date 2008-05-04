CFLAGS=-I/usr/include/db1 -Wall -g -O0
LDLIBS=-ldb-4.5
OBJECTS=FileStorage.o Set.o BDB_Set.o Btree_Set.o Hash_Set.o Bender_Set.o \
        Bender_Impl.o Memory_Deque.o File_Deque.o comparison.o hashing.o

all: test-set test-deque datastructures.a test

datastructures.a: $(OBJECTS)
	$(AR) rcs "$@" $(OBJECTS)

test-set: datastructures.a test-set.c
	$(CC) $(LDFLAGS) -o "$@" test-set.c datastructures.a $(LDLIBS)

test-deque: datastructures.a test-deque.c
	$(CC) $(LDFLAGS) -o "$@" test-deque.c datastructures.a $(LDLIBS)

test: datastructures.a test.c
	$(CC) $(LDFLAGS) -o "$@" test.c datastructures.a $(LDLIBS)

clean:
	rm -f $(OBJECTS)

distclean: clean
	rm -f test-set datastructures.a

.PHONY: all clean distclean

