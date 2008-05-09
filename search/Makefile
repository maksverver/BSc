CFLAGS=-I.. -Wall -Wextra -g -O2
LDLIBS=../nips_vm/libnips_vm.a ../datastructures/datastructures.a -ldb
OBJECTS=main.o search.o

all: search

search: $(OBJECTS) ../nips_vm/libnips_vm.a ../datastructures/datastructures.a
	$(CC) -o search $(OBJECTS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(OBJECTS)

distclean: clean
	rm -f search

.PHONY: all clean distclean

