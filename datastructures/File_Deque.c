#include "Deque.h"
#include "FileStorage.h"
#include <assert.h>
#include <string.h>

/* Deque implementation

   Current implementation stores queue contents as a sequence of elements,
   with the size stored at both ends (in order to be able to seek both ways).

   ~------+--------+--------+--------+--~ ~-+--------+--------+------~
   unused | size_1 |  data  | size_1 |      | data_N | size_N | unused
   ~------+--------+--------+--------+--~ ~-+--------+--------+------~
          |-- 8 ---|-size_1-|-- 8 ---|      |-- 8 ---|      |
        begin                                                end

   (Assuming sizeof(size_t) == 8)
   Data stored is padded to multiples of sizeof(size_t).


    Some notes:
    - pushing data in front of the deque is not supported in this
      implementation!)
    - in order to save space, the allocated data is moved to the front of the
      file periodically (e.g. whenever end - begin > begin) to save disk
      space while maintaining amortized (but not per-operation) complexity
      bounds.
    - 16 bytes per entry is a lot of overhead; using 32-bit sizes would
      reduce that to 8 bytes per entry and is probably sufficient as well (but
      in that case data should probably still be stored on 8-byte boundaries)
*/

typedef struct FileDeque FileDeque;

struct FileDeque
{
    Deque       base;
    size_t      count;                  /* Number of elements */
    size_t      begin;                  /* Offset to start of data */
    size_t      end;                    /* Offset to end of data */
    char        *data;                  /* Allocated data */
    FileStorage fs;                     /* FileStorage for allocated data */
};

/* Rounds argument up to a multiple of sizeof(size_t) */
static size_t align(size_t size)
{
    if (size%sizeof(size_t) != 0)
        size = size - size%sizeof(size_t) + sizeof(size_t);
    return size;
}

static void destroy(FileDeque *deque)
{
    FS_destroy(&deque->fs, deque->data);
    free(deque);
}

static size_t size(FileDeque *deque)
{
    return deque->count;
}

static bool empty(FileDeque *deque)
{
    return deque->count == 0;
}

static bool push_back(FileDeque *deque, const void *data, size_t size)
{
    size_t aligned_size = align(size);
    void *new_data;

    /* Reallocate deque */
    new_data = FS_resize( &deque->fs, deque->data,
                          deque->end + 2*sizeof(size_t) + aligned_size );
    if (new_data == NULL)
        return false;
    deque->data = new_data;

    /* Append item */
    *(size_t*)(deque->data + deque->end) = size;
    memcpy(deque->data + deque->end + sizeof(size_t), data, size);
    *(size_t*)(deque->data + deque->end +
               sizeof(size_t) + aligned_size) = size;
    deque->end += 2*sizeof(size_t) + aligned_size;
    ++deque->count;

    return true;
}

static bool push_front(FileDeque *deque, const void *data, size_t size)
{
    /* NOT IMPLEMENTED */
    return false;
}

static bool get_back(FileDeque *deque, const void **data, size_t *size)
{
    if (deque->count == 0)
        return false;

    *size = *(size_t*)(deque->data + deque->end - sizeof(size_t));
    *data = deque->data + deque->end - sizeof(size_t) - align(*size);
    return true;
}

static bool get_front(FileDeque *deque, const void **data, size_t *size)
{
    if (deque->count == 0)
        return false;

    *size = *(size_t*)(deque->data + deque->begin);
    *data = deque->data + deque->begin + sizeof(size_t);
    return true;
}

static bool pop_back(FileDeque *deque)
{
    size_t size;

    if (deque->count == 0)
        return false;

    size = *((size_t*)(deque->data + deque->end) - 1);
    deque->end -= 2*sizeof(size_t) + align(size);
    --deque->count;

    return true;
}

static bool pop_front(FileDeque *deque)
{
    size_t size;

    if (deque->count == 0)
        return false;

    size = *(size_t*)(deque->data + deque->begin);
    deque->begin += 2*sizeof(size_t) + align(size);
    --deque->count;

    if (deque->begin > deque->end - deque->begin)
    {
        /* Compact space */
        memmove( deque->data,
                 deque->data + deque->begin, deque->end - deque->begin );
        deque->end   -= deque->begin;
        deque->begin -= deque->begin;
    }

    return true;
}

static bool reserve(FileDeque *deque, size_t count, size_t size)
{
    void *new_data;

    new_data = FS_reserve( &deque->fs, deque->data,
                           deque->end + count*(2*sizeof(size_t) + size) );
    if (new_data == NULL)
        return false;
    deque->data = new_data;
    return true;
}

Deque *File_Deque_create(const char *filepath)
{
    FileDeque *deque;

    /* Allocate memory */
    deque = malloc(sizeof(FileDeque));
    assert(deque != NULL);

    deque->base.destroy    = (void*)destroy;
    deque->base.empty      = (void*)empty;
    deque->base.size       = (void*)size;
    deque->base.push_back  = (void*)push_back;
    deque->base.push_front = (void*)push_front;
    deque->base.get_back   = (void*)get_back;
    deque->base.get_front  = (void*)get_front;
    deque->base.pop_back   = (void*)pop_back;
    deque->base.pop_front  = (void*)pop_front;
    deque->base.reserve    = (void*)reserve;

    deque->count = 0;
    deque->begin = 0;
    deque->end   = 0;
    deque->data  = NULL;

    if (!FS_create(&deque->fs, filepath))
    {
        free(deque);
        return NULL;
    }

    return &deque->base;
}
