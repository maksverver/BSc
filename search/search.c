#include "search.h"
#include <nips_vm/nipsvm.h>
#include <nips_vm/bytecode.h>

typedef struct SearchContext
{
    nipsvm_t        *vm;
    Deque           *queue;
    Set             *visited;
    long            expanded;

    /* Capture last error */
    int             err_code;
    nipsvm_pid_t    err_pid;
    nipsvm_pc_t     err_pc;

} SearchContext;

static nipsvm_status_t scheduler_callback(
    size_t succ_size, nipsvm_state_t *succ,
    nipsvm_transition_information_t *ti, void *context )
{
    printf("scheduler_callback()\n");
    /* TODO: Insert into set and queue (if not yet present) */
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
    ++sc->expanded;

    nipsvm_scheduler_iter(sc->vm, state, NULL);
    if (sc->err_code != -1)
        return false;

    return true;
}

long search( nipsvm_t *vm, nipsvm_state_t *initial_state,
             Deque *queue, Set *visited, bool dfs )
{
    SearchContext sc;

    sc.vm       = vm;
    sc.queue    = queue;
    sc.visited  = visited;
    sc.expanded =  0;
    sc.err_code = -1;

    /* HACK */
    /* FIXME: move VM initialization here? */
    sc.vm->callback       = scheduler_callback;
    sc.vm->error_callback = error_callback;

    /* TODO: Insert initial state into queue */
    while (!sc.queue->empty(sc.queue))
    {
        /* TODO: Get the state at the front/back of the queue (depending on ``dfs'') */
        /* TODO: Expand state */
        if (!expand_state(&sc, NULL))
            break;
        /* TODO: remove the state at the front/back of the queue (depending on ``dfs'') */
    }

    if (sc.err_code != -1)
    {
        char err_msg[256];
        nipsvm_errorstring( err_msg, sizeof(err_msg),
                            sc.err_code, sc.err_pid, sc.err_pc );
        fprintf(stderr, "VM encountered an error: %s\n", err_msg);
        return -1;
    }

    return 0;
}
