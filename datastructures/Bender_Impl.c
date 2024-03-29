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
   and an overlaying static search tree in van Emde Boas that is used for fast
   lookups. A sparse array is an array in which some elements may be empty
   (i.e. blank values).

   The array is subdivided into windows on a number of different levels;
   at each level the array is subdivided in windows with sizes that are powers
   of 2, ranging from C to log2(C) (rounded up to the next power of 2).

   Therefore, there are L=O-ceil(log2(O)) levels (numbered from 0 to L-1);
   level x consists of 2^x windows of size C/(2^x).

   For the windows on the lowest level (level L-1) a population count is kept
   (the number of non-blank values stored in the window). These counts are
   used to compute the population counts of higher-level windows on the fly.

   Each level has an associated lower and upper bound on the number of values
   stored in it. If a window stores more (/less) values than its upper
   (/lower) bound permits, it is said to be overflowing (/underflowing).
   Note that this is not an error condition; it is just a state that will be
   rectified when a higher-level window is rebalanced.

   Since we will only insert new elements in the array, undeflow cannot occur
   and we will ignore it entirely.

   The upper bounds for different levels are chosen in such a way that windows
   on the lowest level have a density of 1 (the upper bound is then equal to
   the window size) while higher levels have a lower bound.

   New values are inserted in the lowest level (with the highest number)
   that is not overflowing and then the entire window is rebalanced (meaning
   that blank and non-blank values are evenly redistributed) and the index is
   updated accordingly.

   For fast finding of elements (both when inserting and looking up element)
   a binary search tree index is kept. Its leaf nodes store the same elements
   as the array (i.e. the i-th leaf node stores the i-th element in the array,
   which may be blank if there is no value at index i in the sparse array)
   while each interior node stores the maximum non-blank value of its children
   (or a blank value if there are no non-blank children).

   The tree index then is stored in van Emde Boas lay-out to make it
   cache-friendly without requiring parametrization.
*/


/* Enable an optimization that makes updates somewhat (~33%) faster.

   Normally, updates are performed by finding the smallest non-overflowing
   window where a new element can be inserted before its successor, and then
   rebalancing this window. This rebalancing operation moves all the elements
   in the window around.

   Sometimes, we can insert a new element without moving any other element,
   which also means updating the tree index is easier. This variable enables
   this optimization, which is not part of Bender's specification.
*/
const bool opt_fast_update = true;


/* Returns a pointer to the i-th element in the data array. */
#define ARRAY_AT(i) ((ArrayNode*)(bi->data+(i)*(sizeof(ArrayNode)+bi->V)))

/* Copy value of ArrayNode *q to ArrayNode *p while keeping the pointer data
   unmodified. */
#define ARRAY_COPY(p, q)                                                    \
    do { if (p == q) break; /* Necessary because memset doesn't allow       \
                               overlapping memory regions. */               \
         memcpy( (char*)(p) + offsetof(ArrayNode, size),                    \
                 (char*)(q) + offsetof(ArrayNode, size),                    \
                 sizeof(ArrayNode) + bi->V - offsetof(ArrayNode, size) );   \
    } while(0);

/* Convert ArrayNode pointer to index */
#define ARRAY_IDX(an) (((char*)(an) - bi->data)/(sizeof(ArrayNode) + bi->V))

/* Convert TreeNode pointer to index */
#define TREE_IDX(tn) (((char*)(tn) - (char*)bi->tree)/(sizeof(TreeNode) + bi->V))

/* Returns the number of elements in each window at the i-th level. */
#define WINDOW_SIZE(i) ((size_t)1 << (bi->O - (i)))

/* Returns the number of windows at the i-th level. */
#define NUM_WINDOWS(i) ((size_t)1 << (i))

/* Capacity of the set */
#define C ((size_t)1 << (bi->O))

static void debug_print_array(Bender_Impl *bi, FILE *fp)
{
    size_t n, s, total;
    char *p;

    /* Print population */
    total = 0;
    for (n = 0; n < C; ++n)
    {
        if (ARRAY_AT(n)->size == (size_t)-1)
        {
            fputc('.', fp);
        }
        else
        {
            fputc('x', fp);
            total += 1;
        }
    }
    fputc('\n', fp);
    fprintf(fp, "Total population: %7d\n", (int)total);

    /* Print data */
    for (n = 0; n < C; ++n)
    {
        s = ARRAY_AT(n)->size;
        if (s != (size_t)-1)
        {
            fprintf(fp, "%7d ", (int)n);
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

/* Generates a graph representation of the tree with values associated with
   nodes, in the GraphViz DOT language. */
static void debug_dump_tree(Bender_Impl *bi, const char *filepath)
{
    FILE *fp;
    size_t n, m;
    TreeNode *node;

    fp = fopen(filepath, "wt");
    assert(fp != NULL);
    fprintf(fp, "digraph {\n");

    /* Output node labels */
    for (n = 0; n < 2*C - 1; ++n)
    {
        char buf[100];
        size_t len;

        node = (TreeNode*)((char*)bi->tree + n*(sizeof(TreeNode) + bi->V));
        if (node->size == (size_t)-1)
        {
            strcpy(buf, "[blank]");
        }
        else
        {
            len = node->size >= sizeof(buf) ? sizeof(buf) - 1 : node->size;
            memcpy(buf, node->data, len);
            buf[len] = '\0';
        }

        fprintf(fp, "\tn%d [shape=plaintext,label=\"%s (%d)\"]\n", (int)n, buf, (int)n);
    }

    /* Output graph configuration */
    for (n = 0; n < 2*C - 1; ++n)
    {
        node = (TreeNode*)((char*)bi->tree + n*(sizeof(TreeNode) + bi->V));
        if (node->left != NULL)
        {
            m = ((char*)node->left - (char*)bi->tree)/(sizeof(TreeNode) + bi->V);
            fprintf(fp, "\tn%d -> n%d\n", (int)n, (int)m);
        }
        if (node->right != NULL)
        {
            m = ((char*)node->right - (char*)bi->tree)/(sizeof(TreeNode) + bi->V);
            fprintf(fp, "\tn%d -> n%d\n", (int)n, (int)m);
        }
    }

    fprintf(fp, "}\n");
    fclose(fp);
}

/* Verifies the population count of all windows */
static void debug_check_counts(Bender_Impl *bi)
{
    size_t win, pop, n;

    for (win = 0; win < NUM_WINDOWS(bi->L - 1); ++win)
    {
        /* Recompute for lowest-level window */
        pop = 0;
        for (n = 0; n < WINDOW_SIZE(bi->L - 1); ++n)
            pop += (ARRAY_AT(win*WINDOW_SIZE(bi->L - 1) + n)->size != (size_t)-1);

        if (bi->population[win] != pop)
        {
            fprintf( stderr,
                        "Window %llu has incorrect population!"
                        " (Was: %llu; expected: %llu)\n",
                        (unsigned long long)win,
                        (unsigned long long)bi->population[win],
                        (unsigned long long)pop );
            abort();
        }
    }
}

/* Calculates the binary logarithm of "x", rounded down.
   If x is 0 the logarithm is undefined and -1 is returned. */
static int log2i(unsigned long long x)
{
    if (x == 0)
        return -1;

    return (8*sizeof(unsigned long long) - __builtin_clzll(x));
}

/* Given a node (which is assumed to be a leaf node; i.e. its children
   are not considered) returns the next leaf node in order or NULL if none. */
static TreeNode *next_leaf(TreeNode *node)
{
    /* Go up to the next turning point */
    while (node->parent != NULL && node == node->parent->right)
        node = node->parent;
    if (node->parent == NULL)
        return NULL;

    /* Now go to the next node in-order */
    node = node->parent->right;
    while (node->left != NULL)
        node = node->left;

    return node;
}

/* Creates a tree in van Emde Boas lay-out of the required ``height'',
   on level ``level'' and returns the root. */
static TreeNode *create_subtree(
    Bender_Impl *bi, size_t *tree_pos, size_t *array_pos,
    int height, int level )
{
    if (height == 1)
    {
        /* Create a single node */
        TreeNode *node;

        /* Create a new empty node */
        node = (TreeNode*)((char*)bi->tree +
                           *tree_pos*(sizeof(TreeNode) + bi->V));
        *tree_pos += 1;
        node->left   = NULL;
        node->right  = NULL;
        node->parent = NULL;
        node->size   = (size_t)-1;

        if (level == bi->O)
        {
            /* This node is a leaf node in the complete tree;
               link it with the corresponding array node. */
            node->array = ARRAY_AT(*array_pos);
            *array_pos += 1;
            node->array->tree = node;
        }
        else
        {
            node->array = NULL;
        }

        return node;
    }
    else
    {
        /* Create a subtree by dividing the height into two halves, first
           creating the top subtree, and then creating the bottom subtrees,
           which are connected to the leaf nodes of the top subtree. */
        TreeNode *root, *leaf;

        root = create_subtree(bi, tree_pos, array_pos, height/2, level);

        /* Find first leaf node in subtree */
        leaf = root;
        while (leaf->left != NULL)
            leaf = leaf->left;

        /* Create all subtrees */
        do {
            leaf->left = create_subtree( bi, tree_pos, array_pos,
                                         height - height/2, level + height/2 );
            leaf->left->parent = leaf;
            leaf->right = create_subtree( bi, tree_pos, array_pos,
                                          height - height/2, level + height/2 );
            leaf->right->parent = leaf;
        } while ((leaf = next_leaf(leaf)) != NULL);

        return root;
    }
}

/* Creates the tree index structure. */
static void create_tree(Bender_Impl *bi)
{
    size_t tree_pos, array_pos;

    tree_pos = array_pos = 0;
    create_subtree(bi, &tree_pos, &array_pos, bi->O + 1, 0);
    assert(array_pos == C);
    assert(tree_pos = 2*array_pos - 1);
}

/* Updates the contents of the tree corresponding to a range of array nodes
   The tree must be traversed in postorder for optimal performance. */
static void update_tree_window(
    Bender_Impl *bi, TreeNode *node, int level, ssize_t begin, ssize_t end )
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
        ssize_t k = (ssize_t)WINDOW_SIZE(level + 1);
        if (begin < k)
        {
            /* Traverse left subtree */
            update_tree_window(bi, node->left, level + 1, begin, end);
        }
        if (end > k)
        {
            /* Traverse right subtree */
            update_tree_window(bi, node->right, level + 1, begin - k, end - k);
        }

        /* Copy maximum value of child nodes to current node. */
        if (node->left->size != (size_t)-1 && node->right->size != (size_t)-1)
        {
            greatest = bi->compare( bi->context,
                                    node->left->data,  node->left->size,
                                    node->right->data, node->right->size ) >= 0
                    ? node->left : node->right;
        }
        else
        if (node->left->size != (size_t)-1)
        {
            greatest = node->left;
        }
        else
        if (node->right->size != (size_t)-1)
        {
            greatest = node->right;
        }
        else
        {
            greatest = NULL;
        }

        if (greatest != NULL)
        {
            node->size = greatest->size;
            if (node->size != (size_t)-1)
                memcpy(node->data, greatest->data, node->size);
        }
        else
        {
            node->size = (size_t)-1;
        }
    }
}

/*  Overwrites a blank value in the array with a new value and updates the
    tree index to reflect the change.

    This is used as an alternative method of inserting new values in the
    data structure. (See opt_fast_update for a description)
*/
static void overwrite_blank( Bender_Impl *bi, size_t i,
                             const void *data, size_t size )
{
    TreeNode *node;
    size_t win;

    /* Set value */
    ARRAY_AT(i)->size = size;
    memcpy(ARRAY_AT(i)->data, data, size);

    /* Update population count */
    win = i/WINDOW_SIZE(bi->L - 1);
    bi->population[win] += 1;

    /* Update tree index */
    node = ARRAY_AT(i)->tree;
    do {
        node->size = size;
        memcpy(node->data, data, size);
        node = node->parent;
    } while ( node != NULL &&
              ( node->size == (size_t)-1 ||
                bi->compare( bi->context, node->data, node->size,
                             data, size ) < 0 ) );

    /* debug_dump_tree(bi, "tree.dot"); */
}

/* Returns the population count for window ``win'' of level ``lev''. */
static size_t count(Bender_Impl *bi, int lev, size_t win)
{
    size_t i, j, N;

    i =  win      << (bi->L - 1 - lev);
    j = (win + 1) << (bi->L - 1 - lev);
    N = 0;
    do N += bi->population[i]; while (++i < j);

    return N;
}

/* Returns the index into the data array of the first element not smaller
   than the argument, or C if no smaller element exists. */
static size_t find_successor(Bender_Impl *bi,
                             const void *data, size_t size, int *diff)
{
    TreeNode *node;

    /* First, see if any successor exists, by comparing against the root
       which stores the maximum value in the array. */
    node = bi->tree;
    if (node->size == (size_t)-1 ||
        bi->compare(bi->context, node->data, node->size, data, size) < 0)
    {
        return C;
    }

    /* Now find the real successor by moving down the tree.*/
    while (node->array == NULL)
    {
        if ( node->left->size == (size_t)-1 ||
             bi->compare( bi->context, node->left->data, node->left->size,
                                       data, size ) < 0 )
        {
            /* Values in left subtree too small -- go to right subtree */
            node = node->right;
        }
        else
        {
            /* Left subtree has a successor. */
            node = node->left;
        }
    }

    *diff = bi->compare(bi->context, node->data, node->size, data, size);

    /* Convert pointer to index (a bit ugly) */
    return ARRAY_IDX(node->array);
}

/* Recomputes the population count for windows in range [i:j) */
static void recompute_populations(Bender_Impl *bi, size_t i, size_t j)
{
    size_t p, n;

    p = i*WINDOW_SIZE(bi->L - 1);
    while (i < j)
    {
        bi->population[i] = 0;
        for (n = 0; n < WINDOW_SIZE(bi->L-1); ++n)
        {
            bi->population[i] += ARRAY_AT(p)->size != (size_t)-1;
            p += 1;
        }
        ++i;
    }
}

static void resize(Bender_Impl *bi, int new_order)
{
    int l;
    size_t n;

    /* Resize file; we need C array elements and 2*C-1 tree nodes. */
    bi->data = (*bi->allocator)(&bi->alloc, bi->data, 
        ((sizeof(ArrayNode) + bi->V)<<new_order) +
        2*((sizeof(TreeNode) + bi->V)<<(new_order)) - 1);
    assert(bi->data != NULL);

    /* Write blank values to the new part of the array */
    for ( n = (bi->O > 0) ? ((size_t)1 << (bi->O)) : 0;
          n < ((size_t)1 << new_order); ++n )
    {
        ARRAY_AT(n)->size = (size_t)-1;
    }
    bi->O = new_order;

    /* Create new levels */
    free(bi->upper_bound);
    free(bi->population);
    bi->L = new_order - log2i(new_order) + 1;
    assert(bi->L >= 2);
    bi->upper_bound = malloc(sizeof(size_t)*bi->L);
    bi->population  = malloc(sizeof(size_t)*NUM_WINDOWS(bi->L - 1));
    for (l = 0; l < bi->L; ++l)
    {
        bi->upper_bound[l] = (size_t)WINDOW_SIZE(l)*
            (bi->density + (1 - bi->density)*l/(bi->L - 1));
    }

    /* Recompute population counts */
    recompute_populations(bi, 0, NUM_WINDOWS(bi->L - 1));

    /* Recreate tree index */
    bi->tree = (TreeNode*)(bi->data +
                          ((sizeof(ArrayNode) + bi->V)<<new_order));
    create_tree(bi);

    /* Update entire tree */
    update_tree_window(bi, bi->tree, 0, 0, C);

    /* debug_dump_tree(bi, "tree.dot"); */
}

/* Redistributes window ``win'' at level ``lev'' containing ``N'' elements
   while inserting a new data element (described by ``data'' and ``size'')
   at index ``idx''. The caller must ensure that the window has a free slot. */
static void insert_and_redistribute (Bender_Impl *bi,
    int lev, size_t win, size_t idx, size_t N, const void *data, size_t size )
{
    size_t begin, end, n, p, q, W;

    W = WINDOW_SIZE(lev);
    begin = win*W;
    end   = begin + W;
    /* printf("Redistributing window %d on level %d/%d (%d/%d elements); inserting at %d\n",
              (int)win, (int)lev, bi->L, (int)N + 1, (int)W, (int)idx); */

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

        /* Now move consecutive elements one place to the right to create
           a gap to insert the new element in */
        for (n = p; n > q; --n)
            ARRAY_COPY(ARRAY_AT(n), ARRAY_AT(n - 1));

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
       in size; there are (W-N)%(N+1) gaps with an extra space, which must
       be divided uniformly over all windows.
    */
    N += 1;

    p = end;
    while (p > begin)
    {
        /* FIXME: potential overflow here (though unlikely when using 64-bit
                  aritmetic); try to implement in a safer way (and if possible,
                  more efficiently as well). */
        if ((q - begin)*W >= (p - begin)*N)
        {
            p -= 1, q -= 1;
            ARRAY_COPY(ARRAY_AT(p), ARRAY_AT(q));
        }
        else
        {
            p -= 1;
            ARRAY_AT(p)->size = (size_t)-1;
        }
    }

    /* Update population counts. */
    recompute_populations( bi, win << (bi->L - 1 - lev),
                               (win + 1) << (bi->L - 1 - lev) );
}


void Bender_Impl_create( Bender_Impl *bi, Allocator *allocator,
                         size_t value_size, double density )
{
    /* Check for valid size (positive integer multiple of sizeof(size_t)) */
    assert(value_size > 0 && value_size%sizeof(size_t) == 0);

    /* Initialize structure */
    bi->V           = value_size;
    bi->O           = 0;
    bi->L           = 0;
    bi->density     = density;
    bi->upper_bound = NULL;
    bi->population  = NULL;
    bi->data        = NULL;
    bi->allocator   = allocator;
    resize(bi, 4);
}

void Bender_Impl_destroy(Bender_Impl *bi)
{
    (*bi->allocator)(&bi->alloc, bi->data, 0);
}

bool Bender_Impl_insert( Bender_Impl *bi,
                         const void *key_data, size_t key_size )
{
    size_t i, j, win, N;
    int lev, diff;

    assert(key_size <= bi->V);

    /* First, find successor node */
    i = find_successor(bi, key_data, key_size, &diff);
    if (i < C && diff == 0)
        return true;    /* value already exists */

    if (opt_fast_update)
    {
        /* Find size of gap before successor */
        j = i;
        while (j > 0 && ARRAY_AT(j - 1)->size == (size_t)-1)
            --j;

        if (j < i)
        {
            /* Insert in the middle of the gap. */
            overwrite_blank(bi, (i + j)/2, key_data, key_size);
            return false;
        }
    }

    /* There was no free space; we need to rebalance a window to fit the
       element in. */
    j = (i == C) ? i - 1 : i;

    /* Find a suitable window */
    lev = bi->L - 1;
    do {
        win = j/WINDOW_SIZE(lev);
        N = count(bi, lev, win);
    } while (N >= bi->upper_bound[lev] && --lev >= 0);
    if (lev < 0)
    {
        /* Array is full -- resize to next order of size. */
        /* printf("resizing to %d\n", bi->O + 1); */
        resize(bi, bi->O + 1);
        lev = 0;
    }

    insert_and_redistribute(bi, lev, win, i, N, key_data, key_size);
    /* debug_check_counts(bi); */

    /* Now update tree index to reflect the changes */
    update_tree_window( bi, bi->tree, 0,
                        win*WINDOW_SIZE(lev), (win + 1)*WINDOW_SIZE(lev) );
    /* debug_dump_tree(bi, "tree.dot"); */

    return false;
}

bool Bender_Impl_contains( Bender_Impl *bi,
                           const void *key_data, size_t key_size )
{
    int diff;

    return find_successor(bi, key_data, key_size, &diff) < C && diff == 0;
}
