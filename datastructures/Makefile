CFLAGS=-I/usr/include/db1 -Wall -Wextra -g -O2

LDLIBS=
# removed: -ldb-4.5

OBJECTS=Alloc.o Bender_Set.o Bender_Impl.o Btree_Set.o Dummy_Set.o \
        File_Deque.o FileStorage.o Hash_Set.o Memory_Deque.o Mock_Set.o Set.o \
	comparison.o hashing.o
# removed: BDB_Set.o


all: test-set test-deque datastructures.a

datastructures.a: $(OBJECTS)
	$(AR) rcs "$@" $(OBJECTS)

test-set: datastructures.a test-set.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o "$@" test-set.c datastructures.a $(LDLIBS)

test-deque: datastructures.a test-deque.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o "$@" test-deque.c datastructures.a $(LDLIBS)

clean:
	rm -f $(OBJECTS)

distclean: clean
	rm -f test-set test-deque datastructures.a

.PHONY: all clean distclean

