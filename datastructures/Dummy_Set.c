#include "Set.h"

static bool insert(Set *set, const void *key_data, size_t key_size)
{
    return false;
}

static bool contains(Set *set, const void *key_data, size_t key_size)
{
    return false;
}

static void destroy(Set *set)
{
    free(set);
}

Set *Dummy_Set_create()
{
    Set *set;

    /* Allocate memory */
    set = malloc(sizeof(Set));
    if (set == NULL)
        return NULL;

    set->context  = NULL;
    set->destroy  = destroy;
    set->insert   = insert;
    set->contains = contains;
    set->hash     = NULL;
    set->compare  = NULL;

    return set;
}
