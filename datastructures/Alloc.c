#include "Alloc.h"
#include "config.h"
#include <assert.h>
#include <unistd.h>

/* Uses malloc to allocate memory. */
void *Allocator_malloc(Alloc *a, void *old_data, size_t size)
{
    void *new_data;
    size_t new_size;

    if (size == 0)
    {
        /* Free allocation */
        if (old_data != NULL)
            free(old_data);
        return NULL;
    }

    if (old_data != NULL && a->ms.capacity >= size)
    {
        return old_data;
    }

    new_size = size;
    if (new_size%ALLOC_CHUNK_SIZE != 0)
    {
        new_size += ALLOC_CHUNK_SIZE - new_size%ALLOC_CHUNK_SIZE;
        if (new_size < size)
            return NULL;   /* overflow */
    }
    new_data = realloc(old_data, new_size);
    if (new_data == NULL)
        return NULL;
    a->ms.capacity = new_size;
    return new_data;
}

/* Uee the FileStorage to mmap() data backed by a temporary file. */
void *Allocator_mmap(Alloc *a, void *old_data, size_t size)
{
    if (size == 0)
    {
        /* Free allocation */
        if (old_data != NULL)
            FS_destroy(&a->fs, old_data);
        return NULL;
    }
    else
    if (old_data == NULL)
    {
        assert(size > 0);
        if (!FS_create(&a->fs, "/dev/zero"))
            return NULL;

        return FS_resize(&a->fs, NULL, size);
    }
    else
    {
        /* Resize allocation */
        return FS_resize(&a->fs, old_data, size);
    }
}
