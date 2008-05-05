#include "config.h"
#include "comparison.h"
#include "Set.h"
#include "Bender_Impl.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Set interface to Bender's cache-oblivious set data structure.

   Since Bender's data structure works with fixed-size elements, this wrapper
   implementation is required that keeps different sets storing elements of
   different maximum sizes, ranging from 2**4 to 2**15. (Here, ** denotes
   exponentiation.)
*/

typedef struct Bender_Set Bender_Set;

struct Bender_Set
{
    Set         base;
    Bender_Impl impl[12];
};

/* Computes the index of the set to use for storage.

   Note that the implementation at index i stores values of size upto and
   including 2**(4+i).  This function therefore computes and returns the
   smallest non-negative integer i such that 2**(4+i) >= size + sizeof(size_t)
*/
static unsigned get_index(size_t size)
{
    int index;

    assert((unsigned)size == size); /* check for overflow */
    if (size == 0)
        return 0;
    index = (int)(8*sizeof(unsigned) - __builtin_clz(size - 1)) - 4;

    return index < 0 ? 0 : (unsigned)index;
}

static bool set_insert(Bender_Set *set, const void *key_data, size_t key_size)
{
    unsigned index = get_index(key_size);
    assert(index < 12);
    return Bender_Impl_insert(&set->impl[index], key_data, key_size);
}

static bool set_contains(Bender_Set *set, const void *key_data, size_t key_size)
{
    unsigned index = get_index(key_size);
    assert(index < 12);
    return Bender_Impl_contains(&set->impl[index], key_data, key_size);
}

/* Destroys a set data structure, by closing the backing file
   and freeing all associated resources. */
static void set_destroy(Bender_Set *set)
{
    int n;

    for (n = 0; n < 12; ++n)
        Bender_Impl_destroy(&set->impl[n]);
    free(set);
}

/* Creates a set data structure. */
Set *Bender_Set_create(const char *filepath)
{
    Bender_Set *set;
    int index;

    /* Allocate memory */
    set = malloc(sizeof(Bender_Set));
    if (set == NULL)
        return NULL;

    set->base.context  = NULL;
    set->base.destroy  = (void*)set_destroy;
    set->base.insert   = (void*)set_insert;
    set->base.contains = (void*)set_contains;
    set->base.compare  = default_compare;
    set->base.hash     = default_hash;

    /* Create statically sized sets */
    for (index = 0; index < 12; ++index)
    {
        char buf[1024];
        int len;

        len = snprintf(buf, sizeof(buf), "%s-%02d", filepath, 4 + index);
        assert(len < sizeof(buf));

        Bender_Impl_create(&set->impl[index], buf, 16 << index);

        /* TODO: unlink temp files? */
    }

    return &set->base;
}
