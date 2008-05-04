#include "config.h"
#include "comparison.h"
#include "Bender_Impl.h"
#include "FileStorage.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Implementation of Bender's cache-oblivious set data structure.

   The data structure has an order O and a corresponding capacity
   C = 2^O. (Here ^ denotes exponentation.)

   It is then implemented using a sparse array that stores all values in order
   and an overlaying static search tree in van Emde Boas that isused for fast
   lookups. A sparse array is an array in which some elements may be empty (or
   blank values).

   The array is subdivided into windows on a number of different levels;
   at each level the array is subdivided in windows with sizes that are powers
   of 2, ranging from B=log2(C) rounded up to the next power of 2 to C.

   Therefore, there are L=O-ceil(log2(O)) levels (numbered from 0 to L-1);
   level x consists of 2^x windows of size C/(2^x).

   For each window (at each level) a population count is kept (the number of
   non-blank values stored in the window).

   Each level has an associated lower and upper bound on the number of values
   stored in it. If a window stores more (/less) values than its upper
   (/lower) bound permits, it is said to be overflowing (/underflowing).

   The index is a binary search tree; the leaf nodes store the same elements
   as the array (i.e. the i-th leaf stores the i-th element in the array,
   which may be blank if there is no value at index i in the sparse array)
   while each interior node stores the maximum non-blank value of its children
   (or a blank value if there are no non-blank children).

   New values are inserted in the lowest level (with the highest number)
   that is not overflowing and not underflowing, and then the entire window
   is rebalanced (meaning that blank and non-blank values are evenly
   redistributed) and the index is updated accordingly.

   Since we will only insert new elements in the array, we will ignore
   underflow entirely, only rebalancing when overflow occurs. When the single
   window at level 0 overflows, the size of the array (and its index) is
   doubled (and the window then rebalanced).
*/


/* Returns a pointer to the i-th element in the data array. */
#define ARRAY_AT(i) ((ArrayNode*)(bi->fs.data+(i)*(sizeof(ArrayNode)+(bi->V))))

/* Returns the number of elements in each window at the i-th level. */
#define WINDOW_SIZE(i) ((size_t)1 << (bi->O - (i)))

/* Returns the number of windows at the i-th level. */
#define NUM_WINDOWS(i) ((size_t)1 << (i))

/* Capacity of the set */
#define C ((size_t)1 << (bi->O))

static void debug_print_array(Bender_Impl *bi, FILE *fp)
{
    int n;
    char *p;
    size_t s;

    /* Print population */
    for (n = 0; n < C; ++n)
        fputc((ARRAY_AT(n)->size == (size_t)-1) ? '.' : 'x', fp);
    fputc('\n', fp);

    /* Print data */
    for (n = 0; n < C; ++n)
    {
        s = ARRAY_AT(n)->size;
        if (s != (size_t)-1)
        {
            printf("%7d ", n);
            p = ARRAY_AT(n)->data;
            while (s--)
                fputc((*p >= 32 && *p < 127) ? *p : '.', fp);
            fputc('\n', fp);
        }
    }
}

/* Calculates the binary logarithm of "x", rounded down.
   If x is 0 the logarithm is undefined and -1 is returned. */
static int log2i(unsigned long long x)
{
    if (x == 0)
        return -1;

    return (8*sizeof(long long) - __builtin_clzll(x));
}

/* Returns wether x is an integer power of 2. */
static bool ispow2(unsigned long long x)
{
    return x && (x&(x - 1)) == 0;
}

static void resize(Bender_Impl *bi, int new_order)
{
    bool ok;
    int n;

    /* Resize array */
    ok = FS_resize(&bi->fs, (sizeof(ArrayNode)+bi->V)<<new_order);
    assert(ok);
    for ( n = (bi->O > 0) ? ((size_t)1 << (bi->O)) : 0;
          n < ((size_t)1 << new_order); ++n )
    {
        ARRAY_AT(n)->size = (size_t)-1;
    }
    bi->O = new_order;
    debug_print_array(bi, stdout);

    /* Create new levels */
    free(bi->level);
    for (n = 0; n < bi->L; ++n)
        free(bi->level[n].population);
    bi->L = new_order - log2i(new_order) + 1;
    bi->level = malloc(sizeof(Level)*bi->L);
    for (n = 0; n < bi->L; ++n)
    {
        /* FIXME: Bender uses a range from 1 to x (x<1) for this! */
        bi->level[n].upper_bound = WINDOW_SIZE(n);
        bi->level[n].population  = malloc(sizeof(size_t)*NUM_WINDOWS(n));
        memset(bi->level[n].population, 0, sizeof(size_t)*NUM_WINDOWS(n));
    }

    /* TODO: recreate index tree */

    /* TODO: update population counts */
}

/* Returns the index into the data array of the first element not smaller
   than the argument, or C if no smaller element exists. */
static size_t find_successor(Bender_Impl *bi, const void *data, size_t size)
{
    size_t n;

    for (n = 0; n < C; ++n)
    {
        if (ARRAY_AT(n)->size != (size_t)-1)
        {
            /* FIXME: make comparison configurable */
            if (default_compare(NULL, ARRAY_AT(n)->data, ARRAY_AT(n)->size,
                                      data, size) >= 0)
            {
                break;
            }
        }
    }
    return n;
}

void Bender_Impl_create( Bender_Impl *bi,
                         const char *filepath, size_t value_size )
{
    bool ok;

    /* Check for valid size (positive integer multiple of sizeof(size_t)) */
    assert(value_size > 0 && value_size%sizeof(size_t) == 0);

    /* Open file */
    ok = FS_create(&bi->fs, filepath);
    assert(ok);

    /* Initialize structure */
    bi->V     = value_size;
    bi->O     = 0;
    bi->L     = 0;
    bi->level = NULL;
    resize(bi, 4);
}

void Bender_Impl_destroy(Bender_Impl *bi)
{
    FS_destroy(&bi->fs);
}

bool Bender_Impl_insert( Bender_Impl *bi,
                         const void *key_data, size_t key_size )
{
    size_t i, j, win;
    int lev;

    i = find_successor(bi, key_data, key_size);
    j = (i == C) ? i - 1 : i;

    for (lev = bi->L - 1; lev >= 0; --lev)
    {
        win = j/WINDOW_SIZE(lev);
        if (bi->level[lev].population[win] < bi->level[lev].upper_bound)
            break;
    }
    assert(lev >= 0); /* if this fails, we need to extend the array */

    printf("%d levels; inserting at level %d window %d\n", bi->L, lev, (int)win);
    assert(win < NUM_WINDOWS(lev));

    debug_print_array(bi, stdout);
    /* TODO */
    assert(0);
}

bool Bender_Impl_contains( Bender_Impl *set,
                           const void *key_data, size_t key_size )
{
    /* TODO */
    assert(0);
}
