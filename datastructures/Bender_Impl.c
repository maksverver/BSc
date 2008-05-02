#include "config.h"
#include "Bender_Impl.h"
#include "FileStorage.h"
#include <assert.h>

/* Implementation of Bender's cache-oblivious set data structure. */

void Bender_Impl_create( Bender_Impl *bi,
                         const char *filepath, size_t value_size )
{
    bool ok;

    /* Check for valid size (positive integer multiple of sizeof(int)) */
    assert(value_size > 0 && value_size%sizeof(int) == 0);
    assert((int)value_size == value_size);  /* check for overflow */

    ok = FS_create(&bi->fs, filepath);
    assert(ok);

    bi->value_size = value_size;
}

void Bender_Impl_destroy(Bender_Impl *bi)
{
    FS_destroy(&bi->fs);
}

bool Bender_Impl_insert( Bender_Impl *bi,
                         const void *key_data, size_t key_size )
{
    /* TODO */
    assert(0);
}

bool Bender_Impl_contains( Bender_Impl *set,
                           const void *key_data, size_t key_size )
{
    /* TODO */
    assert(0);
}
