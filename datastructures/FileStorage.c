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
    long chunk_size;

    /* Get page size (used as the allocation chunk size) */
    chunk_size = sysconf(_SC_PAGESIZE);
    if (chunk_size <= 0)
        return false;

    /* Open the file */
    fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
        return false;

    /* Initialize FileStorage structure */
    fs->data       = NULL;
    fs->size       = 0;
    fs->capacity   = 0;
    fs->fd         = fd;
    fs->chunk_size = (size_t)chunk_size;

    return true;
}

void FS_destroy(FileStorage *fs)
{
    if (fs->data != NULL)
    {
        munmap(fs->data, fs->size);
        fs->data = NULL;
    }
    close(fs->fd);
}

bool FS_resize(FileStorage *fs, size_t size)
{
    int res;
    size_t old_size, new_size;

    /* First, check to see if we have free space left */
    if (size <= fs->capacity)
    {
        fs->size = size;
        return true;
    }

    /* Round desired size up to the next page boundary. */
    old_size = fs->capacity;
    new_size = size;
    if (new_size%fs->chunk_size != 0)
    {
        new_size += fs->chunk_size - new_size%fs->chunk_size;
        if (new_size < size)
            return false;   /* overflow */
    }

    /* Change file size */
    res = ftruncate(fs->fd, new_size);
    if (res != 0)
        return false;

    /* Remap memory */
    if (HAVE_MREMAP && fs->data != NULL)
    {
        fs->data = mremap(fs->data, old_size, new_size, MREMAP_MAYMOVE);
        /* FIXME: If this fails, does the old mapping still exist? */
    }
    else
    {
        if (fs->data != NULL)
            munmap(fs->data, old_size);
        fs->data = mmap( NULL, new_size, PROT_READ|PROT_WRITE, MAP_SHARED,
                         fs->fd, 0 );
        /* FIXME: if this fails, the old mapping is destroyed! */
    }

    /* Check wether remapping was succesful.*/
    if (fs->data == NULL || fs->data == MAP_FAILED)
    {
        perror("FileStorage.c: mmap/mremap failed");
        abort();
        return false;
    }

    /* Update size/capacity */
    fs->size     = size;
    fs->capacity = new_size;

    return true;
}
