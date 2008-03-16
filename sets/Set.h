#ifndef SET_H_INCLUDED
#define SET_H_INCLUDED

#include <stdbool.h>
#include <stdlib.h>

/* Defines the basic operations on a set data structure.

void destroy(Set *set)
    Frees the data structure and associated resources.

bool insert(Set *set, const void *key_data, size_t key_size)
    If the key element is not present in the set, it is inserted and
    false is returned. Otherwise, true is returned.

bool contains(Set *set, const void *key_data, size_t key_size)
    Returns true if the key element is present in the set; false otherwise.

*/
typedef struct Set {
    void (*destroy)(struct Set *);
    bool (*insert)(struct Set *, const void *, size_t);
    bool (*contains)(struct Set *, const void *, size_t);
} Set;

/* Creates a set data structure backed by a BerkeleyDB B-tree. */
Set *BDB_Btree_Set_create(const char *filepath);

/* Creates a set data structure backed by a BerkeleyDB hashtable. */
Set *BDB_Hash_Set_create(const char *filepath);

/* Creates a set data structure backed by a custom B-tree implementation. */
Set *Btree_Set_create(const char *filepath, size_t pagesize);

/* Creates a set data structure backed by a custom hash table implementation.
   The capacity parameter is the capacity of the hash table index (i.e. the
   number of buckets) which is fixed for the set (no resizing possible). */
Set *Hash_Set_create(const char *filepath, size_t capacity);

#endif /* ndef SET_H_INCLUDED */
