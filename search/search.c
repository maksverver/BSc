#include "search.h"
#include <assert.h>
#include <stdio.h>
#include <nips_vm/nipsvm.h>

typedef struct SearchContext
{
    nipsvm_t        *vm;
    Deque           *queue;
    Set             *visited;
    long            expanded;
    long            transitions;

    /* To capture VM errors: */
    int             err_code;
    nipsvm_pid_t    err_pid;
    nipsvm_pc_t     err_pc;

} SearchContext;

/* For debugging */
void hex_print(FILE *fp, void *data, size_t size)
{
    unsigned char *p = data;
    size_t i;

    for (i = 0; i < size; ++i)
    {
        fputc("0123456789abcdef"[*p++&15], fp);
        fputc("0123456789abcdef"[*p++>>4], fp);
        if (i%16 == 0)
        {
            fputc('\n', fp);
        }
        else
        {
            fputc(' ', fp);
            if (i%8 == 0)
                fputc(' ', fp);
        }
    }
    fputc('\n', fp);
}

/* For debugging */
void base64_print(FILE *fp, void *data, size_t size)
{
    unsigned char *p = data;
    unsigned int v;
    static const char *digits =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while (size > 0)
    {
        v = (size > 0 ? (p[0] << 16) : 0) |
            (size > 1 ? (p[1] << 8)  : 0) |
            (size > 2 ? (p[2] << 0)  : 0);
        fputc(           digits[(v >> 18)&63]      , fp);
        fputc(           digits[(v >> 12)&63]      , fp);
        fputc(size > 1 ? digits[(v >>  6)&63] : '=', fp);
        fputc(size > 2 ? digits[(v >>  0)&63] : '=', fp);
        if (size < 3)
        {
            p += size;
            size = 0;
        }
        else
        {
            p    += 3;
            size -= 3;
        }
    }

    fputc('\n', fp);
}

/* Makes a copy of the given state with specified size in dynamic memory
   which must be freed by the caller using free(). */
nipsvm_state_t *duplicate_state(nipsvm_state_t *state, size_t state_size)
{
    nipsvm_state_t *copy;
    unsigned long buf_size;
    char *buf_data;

    /* Allocate memory for copy */
    buf_size = (unsigned long)state_size;
    buf_data = malloc(state_size);
    if (buf_data == NULL)
        return NULL;

    /* Copy state */
    copy = nipsvm_state_copy(state_size, state, &buf_data, &buf_size);
    if (copy == NULL)
    {
        free(buf_data);
        return NULL;
    }

    return copy;
}

static nipsvm_status_t scheduler_callback(
    size_t succ_size, nipsvm_state_t *succ,
    nipsvm_transition_information_t *ti, void *context )
{
    SearchContext *sc = context;
    bool b;

    sc->transitions += 1;

    if (sc->visited->insert(sc->visited, succ, succ_size) == false)
    {
        /* Unvisited successor state! Add it to the queue. */
        b = sc->queue->push_back(sc->queue, succ, succ_size);
        assert(b);
    }

    return IC_CONTINUE;
}

static nipsvm_status_t error_callback( nipsvm_errorcode_t err,
    nipsvm_pid_t pid, nipsvm_pc_t pc, void *context )
{
    SearchContext *sc = (SearchContext*)context;

    /* Register error */
    sc->err_code = err;
    sc->err_pid  = pid;
    sc->err_pc   = pc;

    return IC_STOP;
}

static bool expand_state(SearchContext *sc, nipsvm_state_t *state)
{
    sc->expanded += 1;

    nipsvm_scheduler_iter(sc->vm, state, sc);
    if (sc->err_code != -1)
        return false;

    return true;
}

/* Depth-first searches the search space and returns the number of states
   expanded, or -1 on error. The queue should initially be non-empty (or
   the result will be zero). */
static int depth_first_search(SearchContext *sc)
{
    Deque *queue = sc->queue;
    nipsvm_state_t *state;
    size_t state_size;

    while (!queue->empty(queue))
    {
        /* Remove state from the queue */
        if (!queue->get_back(queue, (void**)&state, &state_size))
        {
            return -1;
        }
        state = duplicate_state(state, state_size);
        if (state == NULL)
        {
            return -1;
        }
        if (!queue->pop_back(queue))
        {
            free(state);
            return -1;
        }

        /* Expand state */
        if (!expand_state(sc, state))
        {
            free(state);
            return -1;
        }

        free(state);
    }

    return 0;
}

/* Breadth-first searches the search space and returns the number of states
   expanded, or -1 on error. The queue should initially be non-empty (or
   the result will be zero). */
static int breadth_first_search(SearchContext *sc)
{
    Deque *queue = sc->queue;
    nipsvm_state_t *state;
    size_t state_size;

    while (!queue->empty(queue))
    {
        if (!queue->get_front(queue, (void**)&state, &state_size))
            return -1;

        base64_print(stdout, state, state_size);
        fflush(stdout);

        if (!expand_state(sc, state))
            return -1;

        if (!queue->pop_front(queue))
            return -1;
    }

    return 0;
}

int search( st_bytecode *bytecode,
             Deque *queue, Set *visited, bool dfs,
             long *expanded, long *transitions )
{
    SearchContext sc;
    nipsvm_t vm;
    nipsvm_state_t *state = NULL;
    size_t state_size;
    int status;

    /* Initialize output variables */
    *expanded    = 0;
    *transitions = 0;
    status       = 0;

    /* Initialize VM */
    nipsvm_module_init();
    if (nipsvm_init(&vm, bytecode, scheduler_callback, error_callback) != 0)
    {
        perror("Could not initialize NIPS VM");
        return -1;
    }

    /* Obtain initial state */
    state = nipsvm_initial_state(&vm);
    if (state == NULL)
    {
        perror("Could not obtain initial state");
        status = -1;
        goto cleanup;
    }
    state_size = nipsvm_state_size(state);
    state = duplicate_state(state, state_size);
    if (state == NULL)
    {
        perror("Could not duplicate initial state");
        status = -1;
        goto cleanup;
    }

    /* Initialize search context */
    sc.vm          = &vm;
    sc.queue       = queue;
    sc.visited     = visited;
    sc.transitions =  0;
    sc.expanded    =  0;
    sc.err_code    = -1;

    /* Add initial state to the queue */
    visited->insert(visited, state, state_size);
    if (!queue->push_back(queue, state, state_size))
    {
        perror("Could not add initial state to queue");
        status = -1;
        goto cleanup;
    }

    /* Do bfs/dfs search */
    if (dfs)
        status = depth_first_search(&sc);
    else
        status = breadth_first_search(&sc);

    /* Print VM error */
    if (sc.err_code != -1)
    {
        char err_msg[256];
        nipsvm_errorstring( err_msg, sizeof(err_msg),
                            sc.err_code, sc.err_pid, sc.err_pc );
        fprintf(stderr, "VM encountered an error: %s\n", err_msg);
        return -1;
    }

cleanup:
    if (expanded != NULL)
        *expanded = sc.expanded;
    if (transitions != NULL)
        *transitions = sc.transitions;
    nipsvm_finalize(&vm);

    return status;
}
