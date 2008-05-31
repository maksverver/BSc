COMMON="
datastructures/Alloc.h
datastructures/Alloc.c
datastructures/config.h
datastructures/comparison.c
datastructures/comparison.h
datastructures/hashing.c
datastructures/Set.c
datastructures/Set.h
datastructures/FileStorage.c
datastructures/FileStorage.h"

HASHTABLE="
datastructures/Hash_Set.c"

BTREE="
datastructures/Btree_Set.c"

BENDERSET="
datastructures/Bender_Set.c
datastructures/Bender_Impl.h
datastructures/Bender_Impl.c"

DEQUE="
datastructures/Deque.h
datastructures/File_Deque.c"

SEARCH="
search/search.h
search/main.c
search/search.c
datastructures/Mock_Set.c
datastructures/Dummy_Set.c"

report() {
	LINES=`cat $@ | ./strip-comments.pl | wc -l`
	echo $LINES
	TOTAL=$(($TOTAL + $LINES))
}

echo -n "Common:           "; report $COMMON
echo -n "Hash table:       "; report $HASHTABLE
echo -n "B-tree:           "; report $BTREE
echo -n "Bender set:       "; report $BENDERSET
echo -n "Deque:            "; report $DEQUE
echo -n "Search framework: "; report $SEARCH
echo -n "TOTAL:           "; echo $TOTAL
