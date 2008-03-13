CFLAGS=-I/usr/include/db1 -Wall -g -O0
LDLIBS=-ldb-4.5
TEST_OBJECTS=BDB_Set.o Btree_Set.o comparison.o hashing.o test.o


test: $(TEST_OBJECTS)
	$(CC) $(LDFLAGS) -o "$@" $(TEST_OBJECTS) $(LDLIBS)

clean:
	rm -f *.o

.PHONY: clean all

