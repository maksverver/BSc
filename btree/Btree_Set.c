#include "Set.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Page lay-out:

    |---- 'pagesize' bytes ---|
    +------+------+-------+---+
    | Data | Free | Index | N |
    +------+------+-------+---+
    |- X --|- Y --|-- Z --| 4 |

    X+Y+Z+4 = pagesize
    Z = 8*(N + 1)

    Every page has N values and N+1 children. Typically, N should be larger than 1.

    Index is an array of N + 1 elements describing the contents of the page:
        page[pagesize - 1] = N
        page[pagesize - 2] = 0
        ..
        page[pagesize - 2*i - 2] = Offset to begin of i-th value.
        page[pagesize - 2*i - 3] = Index of i-th child page.
        page[pagesize - 2*i - 4] = Offset to end of the i-th value.
    (NB. alle indices zero-based)
*/

/* Note: these macros assume 'set' is in scope */
#define IDX(p)          ((int*)((char*)p + set->pagesize))
#define COUNT(p)        (IDX(p)[-1])          /* number of values in page */
#define BEGIN(p, i)     (IDX(p)[-2*(i)-2])    /* offset to start of i-th value */
#define CHILD(p, i)     (IDX(p)[-2*(i)-3])    /* index of i-th child page */
#define SIZE(p, i)      (END(p,i)-BEGIN(p,i)) /* size of i-th value */
#define END(p, i)       (IDX(p)[-2*(i)-4])    /* offset to end of i-th value */
#define ISIZE(N)        ((1+2*((N)+1))*sizeof(int))

typedef struct Btree_Set
{
    Set     base;
    int     fd;             /* Descriptor of file backing the set */
    size_t  pagesize;       /* Size of data pages */
    int     pages;          /* Number of pages */
    int     root;           /* Index of root page */

    /* Temporary memory pool */
    size_t  mem_size;       /* Total amount of memory allocated */
    size_t  mem_used;       /* Memory used (set to zero before each operation) */
    char    *mem;           /* Allocated memory pool */
} Btree_Set;

typedef struct PageEntry
{
    size_t size;            /* Size of data */
    int    child;           /* Index of page following entry */
    char   data[1];         /* Data */
} PageEntry;

/* Prints the contents of the given page in a human-readable format.
   Useful for debugging. */
static void debug_print_page(Btree_Set *set, char *page, FILE *fp)
{
    int n, N;

    N = COUNT(page);
    fprintf(fp, "Page: count=%d size=%d\n", N, END(page, N-1));
    for (n = 0; n < N; ++n)
        fprintf(fp, "Value %d: %d-%d\n", n, BEGIN(page, n), END(page, n));
    for (n = 0; n <= N; ++n)
        fprintf(fp, "Child %d: %d\n", n, CHILD(page, n));

    for (n = 0; n < set->pagesize; ++n)
    {
        if (n > 0)
        {
            if (n%16 == 0)
            {
                putc('\n', fp);
            }
            else
            {
                putc(' ', fp);
                if (n%4 == 0)
                    putc(' ', fp);
            }
        }
        putc("0123456789ABCDEF"[(((char*)page)[n]>>4)&15], fp);
        putc("0123456789ABCDEF"[(((char*)page)[n]>>0)&15], fp);
    }
    putc('\n', fp);
    fprintf(fp, "----\n");
}

/*  Allocates the requested number of a bytes from the reserved space. */
static void *alloc_mem(Btree_Set *set, size_t size)
{
    void *data;

    /* Align to "long" while assuming sizeof(long) is a power of two! */
    size = (size + (sizeof(long) - 1))&~(sizeof(long) - 1);

    /* Allocate from the pool. */
    assert(set->mem_size - set->mem_used >= size);
    data = set->mem + set->mem_used;
    set->mem_used += size;

    return data;
}

/* Allocates a new page from the reserved space.
   This page must not be freed by the caller. */
static void *alloc_page(Btree_Set *set)
{
    return alloc_mem(set, set->pagesize);
}

/* Reads a page with the given index.
   The returned page must be freed by the caller. */
static void *read_page(Btree_Set *set, int index)
{
    int res;
    void *page;

    /* Seek to page offset */
    assert(index >= 0);
    res = lseek(set->fd, (off_t)index*set->pagesize, SEEK_SET);
    assert(res >= 0);

    /* Read page */
    page = alloc_page(set);
    res = read(set->fd, page, set->pagesize);
    assert(res == set->pagesize);

    /* fprintf(stderr, "Read page %d\n", index);
    debug_print_page(set, page, stderr); */

    return page;
}

/* Writes a page with the given index. */
static void write_page(Btree_Set *set, void *page, int index)
{
    int res;

    /* Seek to page offset */
    assert(index >= 0);
    res = lseek(set->fd, (off_t)index*set->pagesize, SEEK_SET);
    assert(res >= 0);

    /* Write page */
    res = write(set->fd, page, set->pagesize);
    assert(res == set->pagesize);

    /* fprintf(stderr, "Write page %d\n", index);
    debug_print_page(set, page, stderr); */
}

/* Destroys a set data structure, by closing the backing file
   and freeing all associated resources. */
static void set_destroy(Btree_Set *set)
{
    close(set->fd);
    free(set);
}

/* Creates an entry to be inserted in a page, consisting of a value
   (a size/data pair) and a successor page index. */
static PageEntry *make_entry(
    Btree_Set *set, size_t size, int page, const void *data)
{
    PageEntry *entry;

    entry = alloc_mem(set, sizeof(PageEntry) + size);
    entry->size  = size;
    entry->child = page;
    memcpy(entry->data, data, size);

    return entry;
}

/* Lexicographical comparison. */
static int cmp(const void *d1, size_t s1, const void *d2, size_t s2)
{
    int dif;

    dif = memcmp(d1, d2, s1 < s2 ? s1 : s2);
    if (dif == 0)
    {
        if (s1 < s2)
            dif = -1;
        else
        if (s1 > s2)
            dif = +1;
    }
    return dif;
}

/* Inserts an entry into a page.
   The 'entry' should be dynamically allocated, and is freed by this function.
   If the page has to be split, a new entry is returned, that is to be inserted
   in the parent page; this entry must be freed by the caller. */
static PageEntry *insert_entry( Btree_Set *set,
    char *page, int pos, PageEntry *entry )
{
    PageEntry *result;
    int N, size;

    N    = COUNT(page);
    size = END(page, N - 1);

    if (size + entry->size + ISIZE(N+1) <= set->pagesize)
    {
        int k;

        /* No need to split, just insert */
        result = NULL;

        /* NB. This uses some knowledge of the page lay-out */

        /* Insert value at position 'pos' */
        k = BEGIN(page, pos);
        memmove(page + k + entry->size, page + k, size - k);
        memcpy(page + k, entry->data, entry->size);

        /* Update index */
        memmove( page + set->pagesize - ISIZE(N) - 2*sizeof(int),
                 page + set->pagesize - ISIZE(N), (N - pos)*2*sizeof(int) );
        CHILD(page, pos + 1) = entry->child;
        END(page, pos) = k + entry->size;
        while (++pos <= N)
            END(page, pos) += entry->size;
        ++COUNT(page);
    }
    else
    {
        /* Split required */
        int n, k;
        char *new_page;

        /* For now, just split in the middle.
           This works because we require all keys to be less than 1/4th of a
           page (minus the required index size) so no matter how we split, we
           can always insert the new element in either of the two new pages. */
        k = N/2;

        /* Take out middle page */
        result = make_entry(set, SIZE(page, k), set->pages++, page + BEGIN(page, k));
        COUNT(page) = k;

        /* Create new page */
        /* NB. This uses some knowledge of the page lay-out */
        new_page = alloc_page(set);
        memset(new_page, 0, set->pagesize); /* for debugging */
        COUNT(new_page) = N - (k + 1);
        memcpy(new_page, page + BEGIN(page, k + 1),
               END(page, N - 1) - BEGIN(page, k + 1));
        memcpy(new_page + set->pagesize - ISIZE(N - (k + 1)),
               page - ISIZE(N), 2*sizeof(int)*(N - (k + 1)));
        BEGIN(new_page, 0) = 0;
        for (n = k + 1; n < N; ++n)
            END(new_page, n - (k + 1)) = END(page, n) - BEGIN(page, k + 1);
        for (n = k + 1; n <= N; ++n)
            CHILD(new_page, n - (k + 1)) = CHILD(page, n);

        /* To make debugging easier, set freed space to zero. */
        memset(page + BEGIN(page, k), 0, END(page, N-1) - BEGIN(page, k));
        memset(page + set->pagesize - ISIZE(N), 0, 2*sizeof(int)*(N -k));

        /* NB. insert_entry() will return NULL here, since we have ensured
               enough space is available to insert the entry. */
        if (pos <= k)
            insert_entry(set, page, pos, entry);
        else
            insert_entry(set, new_page, pos - k - 1, entry);

        /* Write out newly created page. */
        write_page(set, new_page, result->child);
    }

    return result;
}

static PageEntry *find_or_insert_page( Btree_Set *set, int pageno,
    const void *key_data, size_t key_size, bool *found )
{
    char *page;
    int N, n, m, child;
    PageEntry *entry;

    page = read_page(set, pageno);
    N = COUNT(page);

    /* Binary search for first element larger than key. */
    n = 0;
    m = N;
    while (n < m)
    {
        int d, mid;

        mid = (n + m)/2;
        d = cmp(page + BEGIN(page, mid), SIZE(page, mid), key_data, key_size);

        if (d < 0)
        {
            n = mid + 1;
        }
        else
        if (d > 0)
        {
            m = mid;
        }
        else
        {
            /* Entry found! */
            *found = true;
            return NULL;
        }
    }

    /* Entry was not found in the current page.
       Search child page, if there is one. */
    child = CHILD(page, n);
    if (child == -1)
    {
        if (*found)
        {
            /* Entry not found and we must insert it. */
            entry = make_entry(set, key_size, -1, key_data);
        }
        else
        {
            /* Entry not found, but we won't insert it. */
            entry = NULL;
        }
        *found = false;
    }
    else
    {
        /* Not found yet -- search subtree */
        entry = find_or_insert_page(set, child, key_data, key_size, found);
    }

    if (entry != NULL)
    {
        /* We must insert the given entry in this page at index n */
        entry = insert_entry(set, page, n, entry);
        write_page(set, page, pageno);
    }

    return entry;
}

static bool find_or_insert( Btree_Set *set,
    const void *key_data, size_t key_size, bool insert_if_not_found )
{
    PageEntry *entry;
    bool found;

    found = insert_if_not_found;
    entry = find_or_insert_page(set, set->root, key_data, key_size, &found);

    if (entry != NULL)
    {
        /* New root page must be created */
        char *page;

        page = alloc_page(set);
        memset(page, 0, set->pagesize); /* for debugging */
        COUNT(page) = 1;
        BEGIN(page, 0) = 0;
        END(page, 0) = entry->size;
        memcpy(page, entry->data, entry->size);
        CHILD(page, 0) = set->root;
        CHILD(page, 1) = entry->child;
        write_page(set, page, set->pages);
        set->root = set->pages++;
    }

    return found;
}

static bool set_insert(Btree_Set *set, const void *key_data, size_t key_size)
{
    assert(key_size <= (set->pagesize - ISIZE(4))/4);
    set->mem_used = 0;
    return find_or_insert(set, key_data, key_size, true);
}

static bool set_contains(Btree_Set *set, const void *key_data, size_t key_size)
{
    set->mem_used = 0;
    return find_or_insert(set, key_data, key_size, false);
}

Set *Btree_Set_create(const char *filepath, size_t pagesize)
{
    Btree_Set *set;
    int fd;
    char *page;
    char *mem;
    size_t mem_size;

    /* Ensure page size is valid */
    assert(pagesize > ISIZE(4));
    assert(pagesize%sizeof(int) == 0);
    assert(pagesize%sizeof(int) == 0);

    /* Open file */
    fd = open(filepath, O_CREAT | O_RDWR, 0666);
    if (fd < 0)
        return NULL;

    /* Allocate memory */
    set = malloc(sizeof(Btree_Set));
    if (set == NULL)
        return NULL;

    /* Temporary memory is needed to fetch pages and to copy entries, so an
       upper bound is: 2*H*pagesize, where H is the maximum height of the B-tree. */
    mem_size = 2*20*pagesize;
    mem = malloc(mem_size);
    if (mem == NULL)
    {
        free(set);
        return NULL;
    }

    set->base.destroy  = (void*)set_destroy;
    set->base.insert   = (void*)set_insert;
    set->base.contains = (void*)set_contains;
    set->fd       = fd;
    set->pagesize = pagesize;
    set->pages    = 1;
    set->root     = 0;
    set->mem_size = mem_size;
    set->mem_used = 0;
    set->mem      = mem;

    /* Create root page. */
    page = alloc_page(set);
    memset(page, 0, set->pagesize); /* for debugging */
    COUNT(page)    = 0;
    BEGIN(page, 0) = 0;
    CHILD(page, 0) = -1;
    write_page(set, page, 0);

    return &set->base;
}
