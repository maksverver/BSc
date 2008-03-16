#include "config.h"
#include "Set.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* Defined in hashing.c */
unsigned default_hash(const void *ignored, const void *data, size_t size);

/* Defined in comparison.c */
int default_compare(const void *d1, size_t s1, const void *d2, size_t s2);


/* Table lay-out (assuming sizeof(size_t) == 8):

            |-------- Entry --------|
    +-------+------+------+---------+
    | Index | Next | Size | Data... |
    +-------+------+------+---------+
    |-- I --|-- 8 -|-- 8 -|-- Size -|

    I = 8*capacity
    The index is simply an array of "capacity" pointers to entries stored in
    the rest of the file, or 0 if there is no entry for a position.
    Each entry is part of a linked list and has a pointer to the next element,
    or 0 if there is none.
*/

typedef struct Hash_Set
{
    Set     base;
    int     fd;             /* Descriptor of file backing the set */
    size_t  capacity;       /* Index capacity (number of buckets) */

    char    *data;          /* Contents of backing file */
    size_t  data_size;      /* Size of backing file */
} Hash_Set;


/* Prints the contents of the data file in a human-readable format.
   Useful for debugging. */
static void debug_print_data(Hash_Set *set, FILE *fp)
{
    size_t n;

    fprintf(fp, "Capacity: %d buckets\n", (int)set->capacity);
    for (n = 0; n < set->capacity; ++n)
        fprintf( fp, "Bucket %d: offset %d\n",
                 (int)n, (int)((size_t*)set->data)[n] );

    for (n = 0; n < set->data_size; ++n)
    {
        if (n > 0)
        {
            if (n%16 == 0)
            {
                putc('\n', fp);
            }
            else
            {
                putc(' ', fp);
                if (n%4 == 0)
                    putc(' ', fp);
            }
        }
        putc("0123456789ABCDEF"[(set->data[n]>>4)&15], fp);
        putc("0123456789ABCDEF"[(set->data[n]>>0)&15], fp);
    }
    putc('\n', fp);
    fprintf(fp, "----\n");
}

/* Resize the data file to the given size.
   NB. This invalidates pointers to set->data! */
static void resize(Hash_Set *set, size_t new_size)
{
    int res;

    res = ftruncate(set->fd, new_size);
    assert(res == 0);

    if (HAVE_MREMAP && set->data != NULL)
    {
        set->data = mremap(set->data, set->data_size, new_size, MREMAP_MAYMOVE);
    }
    else
    {
        if (set->data != NULL)
            munmap(set->data, set->data_size);

        set->data = mmap( NULL, new_size, PROT_READ|PROT_WRITE, MAP_SHARED,
                      set->fd, 0 );
    }
    assert(set->data != NULL && set->data != MAP_FAILED);
    set->data_size = new_size;
}

static bool find_or_insert( Hash_Set *set, const void *key_data, size_t key_size,
                            bool insert_if_not_found )
{
    unsigned hash;
    size_t *next;

    /* Find initial entry */
    hash = default_hash(NULL, key_data, key_size);
    next = (size_t*)set->data + hash%set->capacity;
    while (*next != 0)
    {
        size_t size;
        void *data;

        assert((*next & (sizeof(size_t)-1)) == 0);  /* checks alignment */
        assert(*next >= set->capacity*sizeof(size_t) && *next <= set->data_size - 2*sizeof(size_t));

        size = *(size_t*)(set->data + *next + sizeof(size_t));
        data = set->data + *next + 2*sizeof(size_t);
        if (default_compare(key_data, key_size, data, size) == 0)
            break;

        next = (size_t*)(set->data + *next);
    }

    if (*next != 0)
    {
        /* Element was found. */
        return true;
    }

    if (insert_if_not_found)
    {
        /* Insert new element */
        size_t begin, end;

        /* Create room for the new entry at the end of file, and make sure
           sure it's aligned to sizeof(size_t) (which is assumed to be a power
           of two). */
        begin = set->data_size;
        begin = (begin + (sizeof(size_t) - 1))&~(sizeof(size_t) - 1);
        end   = begin + 2*sizeof(size_t) + key_size;

        /* Add to linked list */
        *next = begin;
        next = NULL;

        /* NB. after resizing, "next" may be no longer valid, which is why
               update it above, and may not refer to it later! */
        resize(set, end);

        /* Copy new value */
        *(size_t*)(set->data + begin) = 0;
        *(size_t*)(set->data + begin + sizeof(size_t)) = key_size;
        memcpy(set->data + begin + 2*sizeof(size_t), key_data, key_size);

        /* debug_print_data(set, stderr); */
    }

    return false;
}

static bool set_insert(Hash_Set *set, const void *key_data, size_t key_size)
{
    return find_or_insert(set, key_data, key_size, true);
}

static bool set_contains(Hash_Set *set, const void *key_data, size_t key_size)
{
    return find_or_insert(set, key_data, key_size, false);
}

/* Destroys a set data structure, by closing the backing file
   and freeing all associated resources. */
static void set_destroy(Hash_Set *set)
{
    if (set->data != NULL)
        munmap(set->data, set->data_size);
    close(set->fd);
    free(set);
}

/* Creates a set data structure. */
Set *Hash_Set_create(const char *filepath, size_t capacity)
{
    Hash_Set *set;
    int fd;

    assert(capacity > 0);

    /* Open file */
    fd = open(filepath, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
        return NULL;

    /* Allocate memory */
    set = malloc(sizeof(Hash_Set));
    if (set == NULL)
        return NULL;

    set->base.destroy  = (void*)set_destroy;
    set->base.insert   = (void*)set_insert;
    set->base.contains = (void*)set_contains;
    set->fd        = fd;
    set->capacity  = capacity;
    set->data      = NULL;
    set->data_size = 0;

    /* Create index */
    resize(set, capacity*sizeof(size_t));
    memset(set->data, 0, set->data_size);

    return &set->base;
}
