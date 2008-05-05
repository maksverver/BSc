#ifndef COMPARISON_H_INCLUDED
#define COMPARISON_H_INCLUDED

#include "comparison.h"
#include <stdlib.h>

/* Default comparison function */
int default_compare( const void *ignored,
                     const void *d1, size_t s1,
                     const void *d2, size_t s2 );

/* Default hash function */
unsigned default_hash( const void *ignored,
                       const void *data, size_t size );

#endif /* ndef COMPARISON_H_INCLUDED */
