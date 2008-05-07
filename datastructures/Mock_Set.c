#include "Set.h"
#include "comparison.h"
#include <assert.h>
#include <stdio.h>

typedef struct Mock_Set Mock_Set;

struct Mock_Set
{
    Set     base;
    Set     *impl;      /* Actual implementation (when recording) */
    FILE    *fp;        /* File being written to/read from */
};

static bool set_insert(Mock_Set *set, const void *key_data, size_t key_size)
{
    int ch;
    bool res;

    if (set->impl != NULL)
    {
        /* Use real set implementation to obtain result */
        set->impl->hash    = set->base.hash;
        set->impl->compare = set->base.compare;
        res = set->impl->insert(set->impl, key_data, key_size);

        /* Write result to file */
        ch = fputc(res, set->fp);
        assert(ch == 0 || ch == 1);
        return ch;
    }
    else
    {
        /* Read result from file */
        ch = fgetc(set->fp);
        assert(ch == 0 || ch == 1);
        res = ch;
    }

    return res;
}

static bool set_contains(Mock_Set *set, const void *key_data, size_t key_size)
{
    int ch;
    bool res;

    if (set->impl != NULL)
    {
        /* Use real set implementation to obtain result */
        res = set->impl->insert(set->impl, key_data, key_size);

        /* Write result to file */
        ch = fputc(res, set->fp);
        assert(ch == 0 || ch == 1);
        return ch;
    }
    else
    {
        /* Read result from file */
        ch = fgetc(set->fp);
        assert(ch == 0 || ch == 1);
        res = ch;
    }

    return res;
}

static void set_destroy(Mock_Set *set)
{
    if (set->impl != NULL)
    {
        set->impl->destroy(set->impl);
    }
    fclose(set->fp);
    free(set);
}

Set *Mock_Set_create(const char *path, bool recording)
{
    Mock_Set    *set;
    Set         *impl;
    FILE        *fp;

    /* Allocate memory */
    set = malloc(sizeof(Mock_Set));
    if (set == NULL)
        return NULL;

    /* Create real set instance, if recording */
    if (recording)
    {
        impl = Btree_Set_create(Allocator_mmap, 4096);
        if (impl == NULL)
        {
            free(set);
            return NULL;
        }
    }
    else
    {
        impl = NULL;
    }

    /* Open file */
    fp = fopen(path, recording ? "wb" : "rb");
    if (fp == NULL)
    {
        free(set);
        impl->destroy(impl);
        return NULL;
    }

    set->base.context  = NULL;
    set->base.destroy  = (void*)set_destroy;
    set->base.insert   = (void*)set_insert;
    set->base.contains = (void*)set_contains;
    set->base.hash     = default_hash;
    set->base.compare  = default_compare;
    set->impl = impl;
    set->fp   = fp;

    return &set->base;
}
