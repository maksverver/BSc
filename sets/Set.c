#include "Set.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SET_BTREE        1
#define SET_HASH         2
#define SET_BDB          3
#define SET_BDB_HASH     4
#define SET_BDB_BTREE    5

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
    int type, pagesize, capacity, keep;
    char *path;
    Set *result;

    if (argc < 1)
        return NULL;

    path = NULL;
    keep = 0;

    if (strcmp(*argv, "btree") == 0)
    {
        pagesize = 4096;
        type = SET_BTREE;
    }
    else
    if (strcmp(*argv, "hash") == 0)
    {
        capacity = 1000000;
        type = SET_HASH;
    }
    else
    if (strcmp(*argv, "BerkeleyDB") == 0)
    {
        type = SET_BDB;
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
            if (type != SET_BDB)
                return NULL;
            type = SET_BDB_BTREE;
        }
        else
        if (strcmp(*argv, "hash") == 0)
        {
            if (type != SET_BDB)
                return NULL;
            type = SET_BDB_HASH;
        }
        else
        if (sscanf(*argv, "pagesize=%d", &pagesize) == 1)
        {
            if (type != SET_BTREE)
                return NULL;
        }
        else
        if (sscanf(*argv, "capacity=%d", &capacity) == 1)
        {
            if (type != SET_HASH)
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
    case SET_BTREE:
        result = Btree_Set_create(path, pagesize);
        break;
    case SET_HASH:
        result = Hash_Set_create(path, capacity);
        break;
    case SET_BDB_BTREE:
        result = BDB_Btree_Set_create(path);
        break;
    case SET_BDB_HASH:
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
