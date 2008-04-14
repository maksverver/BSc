#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "search.h"
#include "nips_vm/nipsvm.h"
#include "nips_vm/bytecode.h"


/* Makes a copy of the given state in dynamic memory which must be freed
   by the caller using free(). */
nipsvm_state_t *duplicate_state(nipsvm_state_t *state)
{
    nipsvm_state_t *copy;
    size_t state_size;
    unsigned long buf_size;
    char *buf_data;

    if (state == NULL)
        return NULL;
    state_size = nipsvm_state_size(state);

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

int main(int argc, char *argv[])
{
    nipsvm_t vm;
    st_bytecode *bytecode;
    nipsvm_state_t *initial_state;
    int res;
    long states;

    /* Load bytecode from file */
    /* TODO: get bytecode path from command line arguments */
    bytecode = bytecode_load_from_file("test.b", NULL);
    if (bytecode == NULL)
    {
        perror("Could not load bytecode");
        exit(1);
    }

    /* Initialize VM */
    nipsvm_module_init();
    res = nipsvm_init(&vm, bytecode, NULL, NULL);
    assert(res == 0);

    /* Obtain initial state */
    initial_state = duplicate_state(nipsvm_initial_state(&vm));
    assert(initial_state != NULL);

    /* Do search */
    /* TODO: proper parameters */
    states = search(&vm, initial_state, NULL, NULL, false);
    if (states < 0)
    {
        fprintf(stderr, "State space search failed!\n");
        exit(1);
    }
    fprintf(stdout, "States expanded: %ld\n", states);
    fprintf(stdout, "States visited:  %ld\n", states);

    /* Clean-up */
    free(initial_state);
    bytecode_unload(bytecode);
    nipsvm_finalize(&vm);

    return 0;
}

