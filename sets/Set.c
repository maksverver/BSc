#include "Set.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum SetType {
    Btree, Hash, BDB_Unspecified, BDB_Hash, BDB_Btree
} SetType;

static const char *make_tempfile()
{
    int res;
    static char path[] = "db-XXXXXX";

    res = mkstemp(path);
    assert(res >= 0);
    close(res);

    return path;
}

/*  Creates a set data structure from a string description.
    Arguments between square brackets are optional.

    If invalid arguments have been passed or a set with the requested
    parameters cannot be constructed, NULL is returned. (This is not very
    user-friendly.)

    "btree [pagesize=P] .."
    Creates a B-tree based set with a pagesize of P bytes (default: 4096).

    "hash [capacity=C] .."
    Creates a hash table based with capacity C items (default: 1,000,000).

    "BerkeleyDB btree .."
    Creates a BerkeleyDB B-tree based set.

    "BerkeleyDB hash .."
    Creates a BerkeleyDB hash table based set.

    Common arguments:
    ".. [file=path]"
    Specifies a path to use to create the backing file. Note that if this
    file exists, it will be overwritten! If no file is specified, a file is
    created in the working directory.

    ".. [keep]"
    Keep the temporary file (i.e. do not unlink it after creation).
*/
Set *Set_create_from_args(int argc, const char * const *argv)
{
    SetType type;
    int pagesize, capacity, keep;
    char *path;
    Set *result;

    if (argc < 1)
        return NULL;

    path = NULL;
    keep = 0;

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
        if (path == NULL && sscanf(*argv, "file=%as", &path))
        {
            if (path == NULL)
                return NULL;
        }
        else
        if (strcmp(*argv, "keep") == 0)
        {
            keep = 1;
        }
        else
        {
            /* no option matched! */
            return NULL;
        }
    }

    if (path == NULL)
    {
        /* Create temporary file (and strdup() so we can free() it later) */
        path = (char*)make_tempfile();
        assert(path != NULL);
        path = strdup(path);
        assert(path != NULL);
    }

    switch (type)
    {
    case Btree:
        result = Btree_Set_create(path, pagesize);
        break;
    case Hash:
        result = Hash_Set_create(path, capacity);
        break;
    case BDB_Btree:
        result = BDB_Btree_Set_create(path);
        break;
    case BDB_Hash:
        result = BDB_Hash_Set_create(path);
        break;
    default:
        /* No valid set selected */
        result = NULL;
    }

    if (!keep || !result)
        unlink(path);

    free(path);
    return result;
}
