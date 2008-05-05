#include "config.h"
#include "comparison.h"
#include "Bender_Impl.h"
#include "FileStorage.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

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
#define ARRAY_AT(i) ((ArrayNode*)(bi->fs.data+(i)*(sizeof(ArrayNode)+bi->V)))

/* Copy value of ArrayNode *q to ArrayNode *p while keeping the pointer data
   unmodified. */
#define ARRAY_COPY(p, q)                                                    \
    do { assert(q->size != (size_t)-1);                                     \
         if (p == q) break; /* Necessary because memset doesn't allow       \
                               overlapping memory regions. */               \
         memcpy( (char*)(p) + offsetof(ArrayNode, size),                    \
                 (char*)(q) + offsetof(ArrayNode, size),                    \
                 sizeof(ArrayNode) + bi->V - offsetof(ArrayNode, size) );   \
    } while(0);

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
            fprintf(fp, "%7d ", n);
            p = ARRAY_AT(n)->data;
            while (s--)
            {
                fputc((*p >= 32 && *p < 127) ? *p : '.', fp);
                p += 1;
            }
            fputc('\n', fp);
        }
    }
}

/* Verifies the population count of all windows */
static void debug_check_counts(Bender_Impl *bi)
{
    int lev, win;
    size_t pop, n;

    for (lev = 0; lev < bi->L; ++lev)
    {
        for (win = 0; win < NUM_WINDOWS(lev); ++win)
        {
            if (lev < bi->L - 1)
            {
                /* Check non-lowest level */
                pop = bi->level[lev + 1].population[2*win + 0] +
                      bi->level[lev + 1].population[2*win + 1];
            }
            else
            {
                /* Recompute for lowest-level window */
                pop = 0;
                for (n = 0; n < WINDOW_SIZE(lev); ++n)
                    pop += (ARRAY_AT(win*WINDOW_SIZE(lev) + n)->size
                            != (size_t)-1);
            }

            if (bi->level[lev].population[win] != pop)
            {
                fprintf( stderr,
                         "Window %d at level %d as incorrect population!"
                         " (Was: %llu; expected: %llu)\n", win, lev,
                        (unsigned long long)bi->level[lev].population[win],
                        (unsigned long long)pop );
                abort();
            }
            assert(bi->level[lev].population[win] == pop);
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

/* Creates a single node of the index tree. */
static TreeNode *create_tree_node(
    Bender_Impl *bi, size_t *tree_pos, size_t *array_pos, int level )
{
    TreeNode *node;

    /* Allocate a new node */
    node = (TreeNode*)((char*)bi->tree + *tree_pos*(sizeof(TreeNode) + bi->V));
    *tree_pos += 1;
    node->size = (size_t)-1;

    if (level == bi->O)
    {
        /* Create leaf node */
        node->left = node->right = NULL;
        node->array = ARRAY_AT(*array_pos);
        *array_pos += 1;
    }
    else
    {
        /* Create interior node */
        node->left  = create_tree_node(bi, tree_pos, array_pos, level + 1);
        node->right = create_tree_node(bi, tree_pos, array_pos, level + 1);
        node->array = NULL;
    }

    return node;
}

/* Creates the tree index structure.

   TODO: change this so the tree is created in van Emde Boas layout.
*/
static void create_tree(Bender_Impl *bi)
{
    size_t tree_pos, array_pos;

    tree_pos = array_pos = 0;
    create_tree_node(bi, &tree_pos, &array_pos, 0);
    assert(array_pos == C);
    assert(tree_pos = 2*array_pos - 1);
}

/* Updates the contents of the tree corresponding to the array nodes in the
   range [begin:end). The tree must be traversed in postorder for optimal
   performance.

   FIXME: some of these arguments can be eliminated.
*/
static void update_tree( TreeNode *node, size_t offset, size_t size,
                         size_t begin, size_t end )
{
    if (node->array)
    {
        /* Leaf node */
        node->size = node->array->size;
        if (node->size != (size_t)-1)
            memcpy(node->data, node->array->data, node->size);
    }
    else
    {
        /* Internal node */
        TreeNode *greatest;
        size_t k = offset + size/2;
        if (begin < k)
        {
            /* Traverse left subtree */
            update_tree(node->left, offset, size/2, begin, end);
        }
        if (end > k)
        {
            /* Traverse right subtree */
            update_tree(node->left, k, size/2, begin, end);
        }

        /* Copy maximum value of child nodes to current node. */
        greatest = default_compare( NULL, node->left->data, node->left->size,
                                    node->right->data, node->right->size ) >= 0
                   ? node->left : node->right;
        node->size = greatest->size;
        if (node->size != (size_t)-1)
            memcpy(node->data, greatest->data, node->size);
    }
}

/* Returns the index into the data array of the first element not smaller
   than the argument, or C if no smaller element exists. */
static size_t find_successor(Bender_Impl *bi,
                             const void *data, size_t size, int *diff)
{
    size_t n;

    for (n = 0; n < C; ++n)
    {
        if (ARRAY_AT(n)->size != (size_t)-1)
        {
            /* FIXME: make comparison configurable */
            *diff = default_compare(NULL, ARRAY_AT(n)->data, ARRAY_AT(n)->size,
                                    data, size);
            if (*diff >=0)
            {
                break;
            }
        }
    }
    return n;
}

/* Recomputes population counts of all levels below (excluding!) ``lev''
   in the windows overlapping ``win'' on level ``lev''. */
static void recompute_populations(Bender_Impl *bi, int lev, size_t win)
{
    size_t p, w, n;
    int l;

    /* If we are the lowest level, are no lower levels. */
    if (lev == bi->L - 1)
        return;

    /* The windows on levels below the current level have to be recomputed
    bottom-up. First, compute the lowest-level. */
    p = win*WINDOW_SIZE(lev);
    for (w = (win << (bi->L - 1 - lev)); w < ((win+1) << (bi->L - 1 - lev)); ++w)
    {
        bi->level[bi->L - 1].population[w] = 0;
        for (n = 0; n < WINDOW_SIZE(bi->L-1); ++n)
        {
            bi->level[bi->L - 1].population[w] += ARRAY_AT(p)->size != (size_t)-1;
            p += 1;
        }
    }

    /* Recompute for higher levels */
    for (l = bi->L-2; l > lev; --l)
    {
        for (w = (win << (l - lev)); w < ((win+1) << (l - lev)); ++w)
        {
            bi->level[l].population[w] = bi->level[l + 1].population[2*w + 0] +
                                        bi->level[l + 1].population[2*w + 1];
        }
    }
}

static void resize(Bender_Impl *bi, int new_order)
{
    bool ok;
    int n;

    /* Resize file; we need C array elements and 2*C-1 tree nodes. */
    ok = FS_resize(&bi->fs,
        ((sizeof(ArrayNode) + bi->V)<<new_order) +
        2*((sizeof(TreeNode) + bi->V)<<(new_order)) - 1);
    assert(ok);

    /* Write blank values to the new part of the array */
    for ( n = (bi->O > 0) ? ((size_t)1 << (bi->O)) : 0;
          n < ((size_t)1 << new_order); ++n )
    {
        ARRAY_AT(n)->size = (size_t)-1;
    }
    bi->O = new_order;

    /* Create new levels */
    free(bi->level);
    for (n = 0; n < bi->L; ++n)
        free(bi->level[n].population);
    bi->L = new_order - log2i(new_order) + 1;
    assert(bi->L >= 2);
    bi->level = malloc(sizeof(Level)*bi->L);
    for (n = 0; n < bi->L; ++n)
    {
        /* FIXME: Bender proposes using a range from 1 to x (x<1) for this!
                  (I don't see why this is necessary?) */
        bi->level[n].upper_bound = WINDOW_SIZE(n);
        bi->level[n].population  = malloc(sizeof(size_t)*NUM_WINDOWS(n));
        memset(bi->level[n].population, 0, sizeof(size_t)*NUM_WINDOWS(n));
    }

    /* Recompute population counts */
    recompute_populations(bi, 0, 0);
    bi->level[0].population[0] = bi->level[1].population[0] +
                                 bi->level[1].population[1];

    /* Recreate tree index */
    bi->tree = (TreeNode*)( bi->fs.data +
                            ((sizeof(ArrayNode) + bi->V)<<new_order) );
    create_tree(bi);

    /* Update entire tree */
    update_tree(bi->tree, 0, C, 0, C);
}

/* Redistributes window ``win'' at level ``lev'' while inserting a new data
   element (described by ``data'' and ``size'' at index ``idx''. The caller
   must ensure that the window has a a free slot. */
static void insert_and_redistribute (Bender_Impl *bi,
    int lev, size_t win, size_t idx, const void *data, size_t size )
{
    size_t begin, end, n, p, q, w, W, N, gap_space, gap_extra;
    int l;

    W = WINDOW_SIZE(lev);
    begin = win*W;
    end   = begin + W;

    /* We start by packing all the elements to the front of the array.
       If there is a gap at or before ``idx'' this is easy, otherwise
       we need to do something smart.

       In the following, p will iterate from begin to end over the original
       data and q will indicate where the next element is written, so
       q <= p. The goal is to pack all elements (including the new one) in
       range [begin:q).
    */

    /* Find first gap. */
    for (p = begin; p < idx; ++p)
    {
        if (ARRAY_AT(p)->size == (size_t)-1)
            break;
    }
    q = p; /* elements in range [begin:p) are already packed */

    if (p < idx)
    {
        /* Copy remaining elements before ``idx''. */
        for ( ; p < idx; ++p)
        {
            if (ARRAY_AT(p)->size != (size_t)-1)
            {
                ARRAY_COPY(ARRAY_AT(q), ARRAY_AT(p));
                q += 1;
            }
        }

        /* Insert new element. */
        ARRAY_AT(q)->size = size;
        memcpy(ARRAY_AT(q)->data, data, size);
        q += 1;
    }
    else
    {
        /* No gap before ``idx''; find first gap after ``idx''.*/
        for ( ; p < end; ++p)
        {
            if (ARRAY_AT(p)->size == (size_t)-1)
                break;
        }
        assert(p < end);    /* otherwise, the entire window is full */

        /* Now move consecutive elements one place to the right to create
           a gap to insert the new element in */
        for (n = p; n > q; --n)
            ARRAY_COPY(ARRAY_AT(p), ARRAY_AT(p - 1));

        /* Insert new element in newly created gap */
        ARRAY_AT(q)->size = size;
        memcpy(ARRAY_AT(q)->data, data, size);

        q = p; /* [begin:p) is packed */
    }

    /* Copy remaining elements after ``idx'' */
    for ( ; p < end; ++p)
    {
        if (ARRAY_AT(p)->size != (size_t)-1)
        {
            ARRAY_COPY(ARRAY_AT(q), ARRAY_AT(p));
            q += 1;
        }
    }

    /* At this point, elements are packed into the range [begin:q) and the
       contents of [q:end) are undefined. (Not necessarily all blank!) */


    /* Redistribute elements evenly.

       "Evenly" means that if the window has size "W" and contains "N"
       elements, then there are (W-N) spaces to be divided over N+1 gaps,
       which means that each gap is at least (W-N)/(N+1) (rounded down)
       in size, while the last (W-N)%(N+1) gaps have an extra space.

       (This definition if evenly is not specifically required, as long
        as elements are divided roughly evenly among windows of the lower
        levels.)
    */
    N = bi->level[lev].population[win] + 1;
    gap_space = (W - N)/(N + 1);
    gap_extra = (W - N)%(N + 1);

    p = end;
    while (q > begin)
    {
        /* Insert gap */
        for (n = 0; n < gap_space; ++n)
        {
            p -= 1;
            ARRAY_AT(p)->size = (size_t)-1;
        }
        if (gap_extra > 0)
        {
            p -= 1;
            ARRAY_AT(p)->size = (size_t)-1;
            gap_extra -= 1;
        }

        /* Copy element */
        p -= 1, q -= 1;
        ARRAY_COPY(ARRAY_AT(p), ARRAY_AT(q));
    }
    while (p > begin)
    {
        p -= 1;
        ARRAY_AT(p)->size = (size_t)-1;
    }

    /* Update population counts. On the current level and all higher levels
       (with larger windows) only one window changes. */
    w = win;
    for (l = lev; l >= 0; --l)
    {
        bi->level[l].population[w] += 1;
        w /= 2;
    }
    recompute_populations(bi, lev, win);
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
    int lev, diff;

    assert(key_size < bi->V);

    if (bi->level[0].population[0] == bi->level[0].upper_bound)
    {
        /* Array is full -- resize to next order of size. */
        debug_print_array(bi, stdout);
        resize(bi, bi->O + 1);
        debug_print_array(bi, stdout);
    }

    i = find_successor(bi, key_data, key_size, &diff);
    if (i < C && diff == 0)
        return true;
    j = (i == C) ? i - 1 : i;

    for (lev = bi->L - 1; lev >= 0; --lev)
    {
        win = j/WINDOW_SIZE(lev);
        if (bi->level[lev].population[win] < bi->level[lev].upper_bound)
            break;
    }
    assert(lev >= 0);
    assert(win < NUM_WINDOWS(lev));

    insert_and_redistribute(bi, lev, win, i, key_data, key_size);
    debug_check_counts(bi);

    /* Now update tree index to reflect the changes */
    update_tree(bi->tree, 0, C, 0, C);

    return false;
}

bool Bender_Impl_contains( Bender_Impl *set,
                           const void *key_data, size_t key_size )
{
    /* TODO */
    assert(0);
}
