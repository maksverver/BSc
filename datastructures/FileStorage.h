#ifndef FILE_STORAGE_H_INCLUDED
#define FILE_STORAGE_H_INCLUDED

#include <stdbool.h>
#include <stdlib.h>

typedef struct FileStorage FileStorage;

/* Models a memory-mapped storage area backed by a file. */
struct FileStorage
{
    size_t  size;           /* Size of memory used */
    size_t  capacity;       /* Size of memory allocated */
    int     fd;             /* Open file descriptor */
};

/* Creates an empty storage area backed by a file with the specified path.
   Returns true and initializes the referenced by ``fs'' if the file can be
   opened, or returns false and sets errno. */
bool FS_create(FileStorage *fs, const char *path);

/* Releases all associated resources */
void FS_destroy(FileStorage *fs, void *data);

/* Resizes the used memory size.
   This may cause the underlying file to be extended, in which case ``data''
   may have tobe moved!

   Returns true if the request was completed succesfully, or returns false and
   sets errno. */
void *FS_resize(FileStorage *fs, void *data, size_t new_size);

#endif /* ndef FILE_STORAGE_H_INCLUDED */
