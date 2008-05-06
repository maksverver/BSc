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


/* Maximum density on level 0.
   Since the capacity is doubled when this is exceeded, the minimum density at
   any time is actually max_density/2.
*/
const double max_density = 0.8;


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

/* Convert ArrayNode pointer to index */
#define ARRAY_IDX(an) (((char*)(an) - bi->fs.data)/(sizeof(ArrayNode) + bi->V))

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
    int lev;
    size_t win, pop, n;

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
                         "Window %llu at level %d as incorrect population!"
                         " (Was: %llu; expected: %llu)\n",
                         (unsigned long long)win, lev,
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

        /* NOTE: if this is slow, we can easily pre-compute the required
                 splits since height will be small (less than 40 or so). */
        int top_height, bottom_height;
        TreeNode *root, *leaf;

        bottom_height = 1;
        while (2*bottom_height < height)
            bottom_height *= 2;
        top_height = height - bottom_height;

        root = create_subtree(bi, tree_pos, array_pos, top_height, level);

        /* Find first leaf node in subtree */
        leaf = root;
        while (leaf->left != NULL)
            leaf = leaf->left;

        /* Create all subtrees */
        do {
            leaf->left = create_subtree( bi, tree_pos, array_pos,
                                         bottom_height, level + top_height );
            leaf->left->parent = leaf;
            leaf->right = create_subtree( bi, tree_pos, array_pos,
                                          bottom_height, level + top_height );
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
    int l;
    size_t n;

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
    for (l = 0; l < bi->L; ++l)
        free(bi->level[l].population);
    free(bi->level);
    bi->L = new_order - log2i(new_order) + 1;
    assert(bi->L >= 2);
    bi->level = malloc(sizeof(Level)*bi->L);
    for (l = 0; l < bi->L; ++l)
    {
        bi->level[l].upper_bound = (size_t)WINDOW_SIZE(l)*
            (max_density + (1 - max_density)*l/(bi->L - 1));
        bi->level[l].population  = malloc(sizeof(size_t)*NUM_WINDOWS(l));
        memset(bi->level[l].population, 0, sizeof(size_t)*NUM_WINDOWS(l));
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
    update_tree_window(bi, bi->tree, 0, 0, C);

    /* debug_dump_tree(bi, "tree.dot"); */
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
        resize(bi, bi->O + 1);
    }

    /* First, find successor node */
    i = find_successor(bi, key_data, key_size, &diff);
    if (i < C && diff == 0)
        return true;    /* value already exists */

    /* Find size of gap before successor */
    j = i;
    while (j > 0 &&ARRAY_AT(j - 1)->size == (size_t)-1)
        --j;

    if (j < i)
    {
        /* There is a gap before the successor! Insert in the middle. */
        TreeNode *node;

        j = (i + j)/2;
        assert(ARRAY_AT(j)->size == (size_t)-1);

        /* Insert before successor */
        ARRAY_AT(j)->size = key_size;
        memcpy(ARRAY_AT(j)->data, key_data, key_size);

        /* Update population counts */
        for (lev = bi->L - 1; lev >= 0; --lev)
        {
            win = j/WINDOW_SIZE(lev);
            bi->level[lev].population[win] += 1;
        }

        /* Update tree index */
        node = ARRAY_AT(j)->tree;
        do {
            node->size = key_size;
            memcpy(node->data, key_data, key_size);
            node = node->parent;
        } while (node != NULL && (node->size == (size_t)-1 ||
                    bi->compare( bi->context, node->data, node->size,
                                              key_data, key_size ) < 0));

        /* Update complete! */
        /* debug_dump_tree(bi, "tree.dot"); */
        return false;
    }

    /* There was no free space; we need to rebalance a window to fit the
       element in. */
    j = (i == C) ? i - 1 : i;

    /* Find a suitable window */
    for (lev = bi->L - 1; lev >= 0; --lev)
    {
        win = j/WINDOW_SIZE(lev);
        if (bi->level[lev].population[win] < bi->level[lev].upper_bound)
            break;
    }
    assert(lev >= 0); /* No suitable window found -- this is impossible */
    assert(win < NUM_WINDOWS(lev));

    insert_and_redistribute(bi, lev, win, i, key_data, key_size);
    /* debug_check_counts(bi); */

    /* Now update tree index to reflect the changes */
    update_tree_window(bi, bi->tree, 0, 0, C);
    /* debug_dump_tree(bi, "tree.dot"); */

    return false;
}

bool Bender_Impl_contains( Bender_Impl *bi,
                           const void *key_data, size_t key_size )
{
    int diff;

    return find_successor(bi, key_data, key_size, &diff) < C && diff == 0;
}
