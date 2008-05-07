#ifndef ALLOC_H_INCLUDED
#define ALLOC_H_INCLUDED

#include "FileStorage.h"

typedef union Alloc Alloc;
typedef struct MemStorage MemStorage;

/* Describes an allocator function.

   Pass NULL as old to create a new allocation with initial size ``size'',
   pass a valid allocation to resize, or pass (size_t)-1 as size to deallocate.

   The return value is either a non-NULL pointer (if allocation succeeds) or
   NULL if allocation fails (in that case, the old pointer should still be
   valid). On deallocation, NULL is returned.
*/
typedef void *(Allocator)(Alloc *a, void *old, size_t size);

void *Allocator_malloc(Alloc *a, void *old, size_t size);
void *Allocator_mmap(Alloc *a, void *old, size_t size);

struct MemStorage
{
    size_t capacity;        /* Size of memory allocated */
};

union Alloc
{
    FileStorage fs;
    MemStorage  ms;
};

#endif /* ndef ALLOC_H_INCLUDED */
