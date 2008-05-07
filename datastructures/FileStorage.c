#include "config.h"
#include "FileStorage.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

bool FS_create(FileStorage *fs, const char *path)
{
    int fd;

    assert(ALLOC_CHUNK_SIZE > 0);

    /* Open the file */
    fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
        return false;

    /* Initialize FileStorage structure */
    fs->size       = 0;
    fs->capacity   = 0;
    fs->fd         = fd;

    return true;
}

void FS_destroy(FileStorage *fs, void *data)
{
    if (data != NULL)
        munmap(data, fs->size);
    close(fs->fd);
}

void *FS_resize(FileStorage *fs, void *data, size_t size)
{
    int res;
    size_t old_size, new_size;

    /* First, check to see if we have free space left */
    if (size <= fs->capacity)
    {
        fs->size = size;
        return data;
    }

    /* Round desired size up to the next page boundary. */
    old_size = fs->capacity;
    new_size = size;
    if (new_size%ALLOC_CHUNK_SIZE != 0)
    {
        new_size += ALLOC_CHUNK_SIZE - new_size%ALLOC_CHUNK_SIZE;
        if (new_size < size)
            return NULL;   /* overflow */
    }

    /* Change file size */
    assert(sizeof(off_t) == sizeof(size_t));
    res = ftruncate(fs->fd, (off_t)new_size);
    if (res != 0)
        return NULL;

    /* Remap memory */
    if (HAVE_MREMAP && data != NULL)
    {
        data = mremap(data, old_size, new_size, MREMAP_MAYMOVE);
        /* FIXME: If this fails, does the old mapping still exist? */
    }
    else
    {
        if (data != NULL)
            munmap(data, old_size);
        data = mmap( NULL, new_size, PROT_READ|PROT_WRITE, MAP_PRIVATE,
                     fs->fd, (off_t)0 );
        /* FIXME: if this fails, the old mapping is destroyed! */
    }

    /* Check wether remapping was succesful.*/
    if (data == NULL || data == MAP_FAILED)
        return NULL;

    /* Update size/capacity */
    fs->size     = size;
    fs->capacity = new_size;

    return data;
}
