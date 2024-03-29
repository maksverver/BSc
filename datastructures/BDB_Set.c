#include "config.h"
#include "comparison.h"
#include "Set.h"
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <db_185.h>

typedef struct BDB_Set
{
    Set base;
    DB *db;
} BDB_Set;

static void set_destroy(BDB_Set *set)
{
    int res;

    res = set->db->close(set->db);
    assert(res == 0);

    free(set);
}

static bool set_insert(BDB_Set *set, const void *key_data, size_t key_size)
{
    int res;
    DBT key = { (void*)key_data, key_size }, data = { NULL, 0 };

    res = set->db->put(set->db, &key, &data, R_NOOVERWRITE);
    assert(res >= 0);

    return res;
}

static bool set_contains(BDB_Set *set, const void *key_data, size_t key_size)
{
    int res;
    DBT key = { (void*)key_data, key_size }, data = { NULL, 0 };

    res = set->db->get(set->db, &key, &data, 0);
    assert(res >= 0);

    return !res;
}

static Set *BDB_Set_create(const char *filepath, int type)
{
    BDB_Set *set;
    DB *db;

    db = dbopen(filepath, O_CREAT | O_RDWR, 0666, type, NULL);
    if (db == NULL)
        return NULL;

    set = malloc(sizeof(BDB_Set));
    if (set == NULL)
        return NULL;

    set->base.destroy  = (void*)set_destroy;
    set->base.insert   = (void*)set_insert;
    set->base.contains = (void*)set_contains;
    set->base.compare  = type == DB_BTREE ? default_compare : NULL;
    set->base.hash     = type == DB_HASH  ? default_hash    : NULL;
    set->db = db;

    return &set->base;
}

Set *BDB_Btree_Set_create(const char *filepath)
{
    return BDB_Set_create(filepath, DB_BTREE);
}

Set *BDB_Hash_Set_create(const char *filepath)
{
    return BDB_Set_create(filepath, DB_HASH);
}

