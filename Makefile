SUBDIRS=proposal paper datastructures search

all:
	set -e;for dir in $(SUBDIRS); do $(MAKE) -C $$dir all; done

clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

distclean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir distclean; done

.PHONY: all clean distclean
	
