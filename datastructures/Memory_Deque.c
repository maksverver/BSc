#include "Deque.h"
#include <assert.h>
#include <string.h>

/* In-memory deque implemented as a circular double-linked list.

   Nodes and their data are allocated in a contiguous block of memory, to
   reduce the number of memory allocations somewhat.
*/

typedef struct MemDeque MemDeque;

typedef struct Node Node;

struct Node
{
    Node *prev, *next;
    void   *data;
    size_t size;
};

struct MemDeque
{
    Deque   base;

    size_t  count;          /* Number of elements */
    Node    *last;          /* Last node (or NULL if empty) */
};


/* N.B. This assumes that sizeof(Node) is such that data appended after the
        Node structure is properly aligned. */
static Node *alloc_node(const void *data, size_t size)
{
    Node *node;

    if (size + sizeof(Node) < size)
        return NULL;    /* overflow */

    node = malloc(size + sizeof(Node));
    if (node != NULL)
    {
        node->size = size;
        node->data = (char*)node + sizeof(Node); 
        memcpy(node->data, data, size);
    }

    return node;
}

static void free_node(Node *node)
{
    free(node);
}

static size_t size(MemDeque *deque)
{
    return deque->count;
}

static bool empty(MemDeque *deque)
{
    return deque->count == 0;
}

static bool push_back(MemDeque *deque, const void *data, size_t size)
{
    Node *node = alloc_node(data, size);

    if (node == NULL)
        return false;

    if (deque->count == 0)
    {
        deque->last = node->prev = node->next = node;
    }
    else
    {
        node->prev = deque->last;
        node->next = deque->last->next;
        node->prev->next = node->next->prev = node;
    }

    ++deque->count;

    return true;
}

static bool push_front(MemDeque *deque, const void *data, size_t size)
{
    if (!push_back(deque, data, size))
        return false;

    deque->last = deque->last->prev;

    return true;
}

static bool get_back(MemDeque *deque, const void **data, size_t *size)
{
    if (deque->count == 0)
        return false;

    *data = deque->last->data;
    *size = deque->last->size;

    return true;
}

static bool get_front(MemDeque *deque, const void **data, size_t *size)
{
    if (deque->count == 0)
        return false;

    *data = deque->last->next->data;
    *size = deque->last->next->size;

    return true;
}

static bool pop_back(MemDeque *deque)
{
    Node *last;

    if (deque->count == 0)
        return false;

    last = deque->last;
    deque->last = last->prev;
    last->prev->next = last->next;
    last->next->prev = last->prev;
    free_node(last);

    --deque->count;

    return true;
}

static bool pop_front(MemDeque *deque)
{
    if (deque->count == 0)
        return false;

    deque->last = deque->last->next;
    pop_back(deque); /* can only succeed at this point */

    return true;
}

static void destroy(MemDeque *deque)
{
    while (!empty(deque))
        pop_back(deque);
    free(deque);
}

Deque *Memory_Deque_create()
{
    MemDeque *deque;

    deque = malloc(sizeof(MemDeque));
    if (deque == NULL)
        return NULL;

    deque->base.destroy    = (void*)destroy;
    deque->base.empty      = (void*)empty;
    deque->base.size       = (void*)size;
    deque->base.push_back  = (void*)push_back;
    deque->base.push_front = (void*)push_front;
    deque->base.get_back   = (void*)get_back;
    deque->base.get_front  = (void*)get_front;
    deque->base.pop_back   = (void*)pop_back;
    deque->base.pop_front  = (void*)pop_front;

    deque->count = 0;
    deque->last  = NULL;

    return &deque->base;
}
