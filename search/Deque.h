#ifndef DEQUE_H_INCLUDED
#define DEQUE_H_INCLUDED

#include <stdlib.h>

typedef struct Deque Deque;

/* Creates a new deque backed by the given file. */
Deque *Deque_create(const char *filepath);

/* The Deque structure implements a file-backed deque data structure (which is
   a queue that supports adding and removing elements both at the front and at
   the back).

   Contains the following methods:

    void destroy()
        Destroys the queue and frees all allocated resources.

    bool empty()
        Returns wether the queue is empty.

    size_t size()
        Returns the number of elements in the queue.

    void push_back(const void *data, size_t length)
        Adds an element at the back of the queue.

    void push_front(const void *data, size_t length)
        Adds an element at the front of the queue.

    void get_back(void **data, size_t *length)
        Retrieves the element at the back of the queue.

    void get_front(void **data, size_t *length)
        Retrieves the element at the front of the queue.

    void pop_back(const void *data, size_t length)
        Removes an element at the back of the queue.

    void pop_front(const void *data, size_t length)
        Removes an element at the front of the queue.

*/
struct Deque
{
    void (*destroy)(struct Deque *);
    bool (*empty)(struct Deque *);
    size_t (*size)(struct Deque *, const void *, size_t);
    void (*push_back)(struct Deque *, const void *, size_t);
    void (*push_front)(struct Deque *, const void *, size_t);
    bool (*get_back)(struct Deque *, void **, size_t *);
    bool (*get_front)(struct Deque *, void **, size_t *);
    void (*pop_back)(struct Deque *, const void *, size_t);
    void (*pop_front)(struct Deque *, const void *, size_t);
};

#endif /* ndef DEQUE_H_INCLUDED */
