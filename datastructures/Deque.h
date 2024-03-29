#ifndef DEQUE_H_INCLUDED
#define DEQUE_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

typedef struct Deque Deque;

/* Creates a new deque backed by the given file. */
Deque *File_Deque_create(const char *filepath);

/* Creates a new deque backed by memory. */
Deque *Memory_Deque_create();


/* The Deque structure implements a deque data structure: a queue that
   supports adding and removing elements both at the front and at the back.

   Contains the following methods:

    void destroy()
        Destroys the queue and frees all allocated resources.

    bool empty()
        Returns wether the queue is empty.

    size_t size()
        Returns the number of elements in the queue.

    bool push_back(const void *data, size_t length)
        Adds an element at the back of the queue or returns false in case
        of failure.

    bool push_front(const void *data, size_t length)
        Adds an element at the front of the queue or returns false in case
        of failure.

    bool get_back(void **data, size_t *length)
        Retrieves the element at the back of the queue or returns false if
        the deque is empty or the element could not be retrieved.

    bool get_front(void **data, size_t *length)
        Retrieves the element at the front of the queue or returns false if
        the deque is empty or the element could not be retrieved.

    bool pop_back()
        Removes an element at the back of the queue or returns false if the
        deque is empty or the element could not be removed.

    bool pop_front()
        Removes an element at the front of the queue or returns false if the
        deque is empty or the element could not be removed.

    bool reserve(size_t count, size_t size)
        Tries to reserve space for ``count'' more elements of size ``size''
        or returns false if the reservation could not be made. This prevents
        reallocations when growing.
*/
struct Deque
{
    void (*destroy)(struct Deque *);
    bool (*empty)(struct Deque *);
    size_t (*size)(struct Deque *);
    bool (*push_back)(struct Deque *, const void *, size_t);
    bool (*push_front)(struct Deque *, const void *, size_t);
    bool (*get_back)(struct Deque *, void **, size_t *);
    bool (*get_front)(struct Deque *, void **, size_t *);
    bool (*pop_back)(struct Deque *);
    bool (*pop_front)(struct Deque *);
    bool (*reserve)(struct Deque *, size_t count, size_t size);
};

#endif /* ndef DEQUE_H_INCLUDED */
