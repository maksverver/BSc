#ifndef SET_H_INCLUDED
#define SET_H_INCLUDED

#include <stdbool.h>
#include <stdlib.h>
#include "Alloc.h"

typedef struct Set Set;

/* Creates a set data structure from a string description */
/* Set *Set_create_from_string(const char *descr); */
Set *Set_create_from_args(int argc, const char * const *argv);


/* Defines the basic operations on a set data structure.

The "context" field may be set to a specific value by the application.
It is not directly used by the set implementations.

void destroy(Set *set)
    Frees the data structure and associated resources.

bool insert(Set *set, const void *key_data, size_t key_size)
    If the key element is not present in the set, it is inserted and
    false is returned. Otherwise, true is returned.

bool contains(Set *set, const void *key_data, size_t key_size)
    Returns true if the key element is present in the set; false otherwise.

int compare(const void *context, const void *data1, size_t size1,
                                 const void *data2, size_t size2)
    Performs a three-way comparison between two keys, returning an integer
    less than, equal to, or greater than zero if the first key is respectively
    less than, equal to, or greater than the second.

unsigned hash(const void *context, const void *key_data, size_t key_size)
    Computes a hash value for the given key.

*/
struct Set {
    void *context;

    /* These functions are set by the constructor */
    void (*destroy)(Set *);
    bool (*insert)(Set *, const void *, size_t);
    bool (*contains)(Set *, const void *, size_t);

    /* These functions may be overridden by the caller */
    int (*compare)(const void *, const void *, size_t, const void *, size_t);
    unsigned (*hash)(const void *, const void *, size_t);
};

/* Creates a set data structure backed by a BerkeleyDB B-tree. */
Set *BDB_Btree_Set_create(const char *filepath);

/* Creates a set data structure backed by a BerkeleyDB hashtable. */
Set *BDB_Hash_Set_create(const char *filepath);

/* Creates a set data structure backed by a custom B-tree implementation. */
Set *Btree_Set_create(Allocator *alloc, int pagesize);

/* Creates a set data structure backed by a custom hash table implementation. */
Set *Hash_Set_create(Allocator *alloc, size_t capacity);

/* Creates a cache-oblivious set data structure backed as proposed by
   Bender at al. in "A locality-preserving cache-oblivious data structure".

   Density is the density of the top-level window before it is overflowing
   (between 0 and 1). Low values make updates cheaper, but result in larger
   files. */
Set *Bender_Set_create(Allocator *alloc, double density);

/* Creates a mock set data structure that records/replays answers to/from the
   given file path. This is useful for benchmarking purposes. */
Set *Mock_Set_create(const char *filepath, bool record);

/* Creates a dummy set data structure that always returns false.
   This is useful for benchmarking purposes. */
Set *Dummy_Set_create();

#endif /* ndef SET_H_INCLUDED */
