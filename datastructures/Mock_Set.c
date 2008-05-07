#include "Set.h"
#include "comparison.h"
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct Mock_Set Mock_Set;

struct Mock_Set
{
    Set     base;

    /* When recording: */
    Set     *impl;
    FILE    *fp;

    /* When replaying */
    char    *begin, *end, *pos;
    int     fd;
};

static bool set_insert(Mock_Set *set, const void *key_data, size_t key_size)
{
    bool res;

    if (set->impl != NULL)
    {
        char ch;

        /* Use real set implementation to obtain result */
        set->impl->hash    = set->base.hash;
        set->impl->compare = set->base.compare;
        res = set->impl->insert(set->impl, key_data, key_size);

        /* Write result to file */
        ch = fputc(res, set->fp);
        assert(ch == 0 || ch == 1);
    }
    else
    {
        /* Read result from file */
        assert(set->pos < set->end);
        res = *set->pos++;
    }

    return res;
}

static bool set_contains(Mock_Set *set, const void *key_data, size_t key_size)
{
    bool res;

    if (set->impl != NULL)
    {
        char ch;

        /* Use real set implementation to obtain result */
        res = set->impl->contains(set->impl, key_data, key_size);

        /* Write result to file */
        ch = fputc(res, set->fp);
        assert(ch == 0 || ch == 1);
    }
    else
    {
        /* Read result from file */
        assert(set->pos < set->end);
        res = *set->pos++;
    }

    return res;
}

static void set_destroy(Mock_Set *set)
{
    if (set->impl != NULL)
        set->impl->destroy(set->impl);
    if (set->fp != NULL)
        fclose(set->fp);
    if (set->begin != NULL)
        munmap(set->begin, set->end - set->begin);
    if (set->fd != -1)
        close(set->fd);
    free(set);
}

Set *Mock_Set_create(const char *path, bool recording)
{
    Mock_Set *set;

    /* Allocate memory */
    set = malloc(sizeof(Mock_Set));
    if (set == NULL)
        return NULL;

    set->impl  = NULL;
    set->fp    = NULL;
    set->begin = NULL;
    set->end   = NULL;
    set->pos   = NULL;
    set->fd    = -1;

    if (recording)
    {
        /* Create real set instance */
        set->impl = Btree_Set_create(Allocator_mmap, 4096);
        if (set->impl == NULL)
        {
            set_destroy(set);
            return NULL;
        }

        /* Open file */
        set->fp = fopen(path, recording ? "wb" : "rb");
        if (set->fp == NULL)
        {
            set_destroy(set);
            return NULL;
        }
    }
    else
    {
        struct stat st;

        /* Open file */
        set->fd = open(path, O_RDONLY);
        if (set->fd < 0 || fstat(set->fd, &st) < 0)
        {
            set_destroy(set);
            return NULL;
        }

        /* Memory map file */
        set->begin = mmap( NULL, (size_t)st.st_size, PROT_READ, MAP_SHARED,
                           set->fd, (off_t)0 );
        if (set->begin == NULL)
        {
            set_destroy(set);
            return NULL;
        }
        set->end = set->begin + st.st_size;
        set->pos = set->begin;
    }

    set->base.context  = NULL;
    set->base.destroy  = (void*)set_destroy;
    set->base.insert   = (void*)set_insert;
    set->base.contains = (void*)set_contains;
    set->base.hash     = default_hash;
    set->base.compare  = default_compare;

    return &set->base;
}
