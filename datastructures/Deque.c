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

typedef struct DequeImpl DequeImpl;

struct DequeImpl
{
    Deque   base;
    size_t  count;          /* Number of elements */
    char    *data;          /* Contents of backing file */
    size_t  data_size;      /* Size of data used in the backing file */
    size_t  data_left;      /* Unallocated space left in backing file */
    size_t  data_begin;     /* Begin of valid data */
    size_t  data_end;       /* End of valid data */
};

static void deque_destroy(DequeImpl *deque)
{
    free(deque);
}

static size_t size(DequeImpl *deque)
{
    return deque->count;
}

static bool empty(DequeImpl *deque)
{
    return deque->empty();
}

static void push_back(DequeImpl *deque, const void *data, size_t size)
{
    /* TODO */
}

static void push_front(DequeImpl *deque, const void *data, size_t size)
{
    /* NOT IMPLEMENTED */
    assert(0);
}

static void get_back(DequeImpl *deque, const void **data, size_t *size)
{
    /* TODO */
}

static void get_front(DequeImpl *deque, const void **data, size_t *size)
{
    /* TODO */
}

static void pop_back(DequeImpl *deque)
{
    /* TODO */
}

static void pop_front(DequeImpl *deque)
{
    /* TODO */
}

Deque *Deque_create(const char *filepath)
{
    DequeImpl *deque;
    int fd;

    /* Open file */
    fd = open(filepath, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
        return NULL;

    /* Allocate memory */
    deque = malloc(sizeof(DequeImpl));
    assert(deque != NULL);

    deque->base.destroy    = (void*)destroy;
    deque->base.empty      = (void*)empty;
    deque->base.size       = (void*)size;
    deque->base.push_back  = (void*)push_back;
    deque->base.push_front = (void*)push_Front;
    deque->base.get_back   = (void*)get_back;
    deque->base.get_front  = (void*)get_front;


    deque->count = 0;

    return deque;
}

struct Deque
{
    void (*destroy)(struct Deque *);
    size_t (*size)(struct Deque *, const void *, size_t);
    void (*push_back)(struct Deque *, const void *, size_t);
    void (*push_front)(struct Deque *, const void *, size_t);
    bool (*get_back)(struct Deque *, const void *, size_t);
    bool (*get_front)(struct Deque *, const void *, size_t);
    FILE    *fp;
    size_t  count;
};
