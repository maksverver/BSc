#include "Deque.h"
#include <assert.h>

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
*/

typedef struct FileDeque FileDeque;

struct FileDeque
{
    Deque   base;
    size_t  count;          /* Number of elements */
    /* TODO */
};

static void destroy(FileDeque *deque)
{
    /* TODO */
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
    /* TODO */
    return false;
}

static bool push_front(FileDeque *deque, const void *data, size_t size)
{
    /* NOT IMPLEMENTED */
    return false;
}

static bool get_back(FileDeque *deque, const void **data, size_t *size)
{
    /* TODO */
    return false;
}

static bool get_front(FileDeque *deque, const void **data, size_t *size)
{
    /* TODO */
    return false;
}

static bool pop_back(FileDeque *deque)
{
    /* TODO */
    return false;
}

static bool pop_front(FileDeque *deque)
{
    /* TODO */
    return false;
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
    deque->base.pop_back   = (void*)get_back;
    deque->base.pop_front  = (void*)get_front;

    deque->count = 0;

    /* TODO: open file */

    return &deque->base;
}
