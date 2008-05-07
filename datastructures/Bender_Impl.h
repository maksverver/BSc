#ifndef BENDER_IMPL_H_INCLUDED
#define BENDER_IMPL_H_INCLUDED

#include "Alloc.h"

/* Implementation of Bender's cache-oblivious set data structure.

   Stores values upto a fixed size (which must be a multiple of sizeof(int))
   and tags data with the size as well. As a result, if value_size == X,
   then the data structure can store elements of size upto (X - sizeof(int)).
*/

typedef struct Bender_Impl  Bender_Impl;
typedef struct ArrayNode    ArrayNode;
typedef struct TreeNode     TreeNode;

struct ArrayNode
{
    TreeNode *tree;

    /* NB. Any additional members must be added here, before ``size'', as
           values are copied by copyinging the contents starting from ``size''.
    */

    size_t   size;
    char     data[];
};

struct TreeNode
{
    TreeNode  *left, *right, *parent;
    ArrayNode *array;

    /* NB. Any additional members must be added here, before ``size'', as
           values are copied by copyinging the contents starting from ``size''.
    */

    size_t    size;
    char      data[];
};

struct Bender_Impl
{
    size_t      V;              /* Size of values */
    int         O;              /* Order (capacity == pow(2,order)) */
    int         L;              /* Number of levels */
    double      density;        /* Upper bound on density on level 0 */
    size_t      *upper_bound;   /* Population upper bound per level */
    size_t      *population;    /* Population per window */
    TreeNode    *tree;          /* Pointer to index tree root */
    char        *data;          /* Allocated data */
    Allocator   *allocator;     /* Allocator function */
    Alloc       alloc;          /* Allocator context */

    /* Custom comparison function */
    int (*compare)(const void *, const void *, size_t, const void *, size_t);
    const void *context;
};

/* Create a Bender set implementation. */
void Bender_Impl_create( Bender_Impl *bi, Allocator *allocator,
                         size_t value_size, double density );

/* Destroy a Bender set implementation and free all associated resources. */
void Bender_Impl_destroy(Bender_Impl *bi);

/* Insert a value into a set implementation.
   Callers must ensure that (key_size + sizeof(int) <= bi->value_size) */
bool Bender_Impl_insert( Bender_Impl *bi,
                         const void *key_data, size_t key_size );

/* Insert a value into a set implementation.
   Callers must ensure that (key_size + sizeof(int) <= bi->value_size) */
bool Bender_Impl_contains( Bender_Impl *set,
                           const void *key_data, size_t key_size );

#endif /* ndef BENDER_H_INCLUDED */
