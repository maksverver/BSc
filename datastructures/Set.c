#include "Set.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum SetType {
    Btree, Hash, BDB_Unspecified, BDB_Hash, BDB_Btree, Bender
} SetType;


/*  Creates a set data structure from a string description.
    Arguments between square brackets are optional.

    If invalid arguments have been passed or a set with the requested
    parameters cannot be constructed, NULL is returned. (This is not very
    user-friendly.)

    "btree [pagesize=P] .."
    Creates a B-tree based set with a pagesize of P bytes (default: 4096).

    "hash [capacity=C] .."
    Creates a hash table based with capacity C items (default: 1,000,000).

    "BerkeleyDB path=FP btree .."
    Creates a BerkeleyDB B-tree based set.

    "BerkeleyDB path=FP hash .."
    Creates a BerkeleyDB hash table based set.

    "Bender"
    Creates a cache-oblivious set (as proposed by Bender et al.)

    Optional arguments:

    [malloc]
    Use the malloc allocator (default)

    [mmap]
    Use the mmap (file backed) allocator
*/
Set *Set_create_from_args(int argc, const char * const *argv)
{
    SetType type;
    int pagesize, capacity;
    char *path;
    Set *result;
    Allocator *allocator = NULL;

    if (argc < 1)
        return NULL;

    path = NULL;

    if (strcmp(*argv, "btree") == 0)
    {
        pagesize = 4096;
        type = Btree;
    }
    else
    if (strcmp(*argv, "hash") == 0)
    {
        capacity = 1000000;
        type = Hash;
    }
    else
    if (strcmp(*argv, "BerkeleyDB") == 0)
    {
        type = BDB_Unspecified;
    }
    else
    if (strcmp(*argv, "Bender") == 0)
    {
        type = Bender;
    }
    else
    {
        /* Invalid type! */
        return NULL;
    }

    --argc, ++argv;

    /* Iterate over remaining arguments */
    for (; argc > 0; --argc, ++argv)
    {
        if (strcmp(*argv, "btree") == 0)
        {
            if (type != BDB_Unspecified)
                return NULL;
            type = BDB_Btree;
        }
        else
        if (strcmp(*argv, "hash") == 0)
        {
            if (type != BDB_Unspecified)
                return NULL;
            type = BDB_Hash;
        }
        else
        if (sscanf(*argv, "pagesize=%d", &pagesize) == 1)
        {
            if (type != Btree)
                return NULL;
        }
        else
        if (sscanf(*argv, "capacity=%d", &capacity) == 1)
        {
            if (type != Hash)
                return NULL;
        }
        else
        if (path == NULL && sscanf(*argv, "file=%as", &path) == 1)
        {
            if (path == NULL)
                return NULL;
        }
        else
        if (strcmp(*argv, "malloc") == 0)
        {
            if (allocator != NULL)
                return NULL;
            allocator = Allocator_malloc;
        }
        else
        if (strcmp(*argv, "mmap") == 0)
        {
            if (allocator != NULL)
                return NULL;
            allocator = Allocator_mmap;
        }
        else
        {
            /* no option matched! */
            return NULL;
        }
    }

    /* Set default allocator */
    if (allocator == NULL)
        allocator = Allocator_malloc;

    switch (type)
    {
    case Btree:
        result = Btree_Set_create(allocator, pagesize);
        break;

    case Hash:
        result = Hash_Set_create(allocator, (size_t)capacity);
        break;

    case BDB_Btree:
        if (path == NULL)
            return NULL;
        result = BDB_Btree_Set_create(path);
        break;

    case BDB_Hash:
        if (path == NULL)
            return NULL;
        result = BDB_Hash_Set_create(path);
        break;

    case Bender:
        result = Bender_Set_create(allocator);
        break;

    default:
        /* No valid set selected */
        result = NULL;
        break;
    }

    return result;
}
