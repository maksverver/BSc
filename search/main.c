#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "search.h"
#include "nips_vm/bytecode.h"

static const char *opt_bytecode_path;
static bool opt_bfs = false;
static Set *set = NULL;

static void usage()
{
    printf(
        "Usage:\n"
        "    search [<options>] (set description)\n"
        "Options:\n"
        "    -B          -- use breadth-first search (default)\n"
        "    -D          -- use depth-first search\n"
        "    -m model    -- path to model bytecode file\n");
    exit(1);
}

static void parse_args(int argc, char *argv[])
{
    int ch;
    bool bfs = false, dfs = false;

    if (argc < 2) usage();

    while ((ch = getopt(argc, argv, "BDm:")) >= 0)
    {
        switch (ch)
        {
        case 'B':
            bfs = true;
            break;

        case 'D':
            dfs = true;
            break;

        case 'm':
            if (opt_bytecode_path != NULL)
            {
                printf("At most one model can be specified!\n\n");
                usage();
            }
            opt_bytecode_path = optarg;
            break;

        case '?':
            usage();
        }
    }

    if (bfs && dfs)
    {
        printf("At most one of -B or -D may be given!\n\n");
        usage();
    }
    else
    {
        opt_bfs = bfs;
    }

    if (opt_bytecode_path == NULL)
    {
        printf("A model must be specified!\n\n");
        usage();
    }

    argc -= optind;
    argv += optind;
    if (argc < 1)
    {
        printf("A set description must be given!\n\n");
        usage();
    }

    set = Set_create_from_args(argc, (const char**)argv);
    if (set == NULL)
    {
        perror("Could not create set (invalid description?)\n\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    Deque *deque = NULL;
    st_bytecode *bytecode = NULL;
    int status = 0;
    long expanded, transitions;

    parse_args(argc, argv);

    /* Load bytecode from file */
    bytecode = bytecode_load_from_file(opt_bytecode_path, NULL);
    if (bytecode == NULL)
    {
        fprintf(stderr, "Could not load bytecode from file \"%s\": ", argv[1]);
        perror(NULL);
        status = 1;
        goto cleanup;
    }

    /* Create deque data structure */
    deque = File_Deque_create("/dev/zero");
    if (deque == NULL)
    {
        perror("Could not create deque");
        status = 1;
        goto cleanup;
    }

    /* Do search */
    if (search(bytecode, deque, set, opt_bfs, &expanded, &transitions) < 0)
    {
        fprintf(stderr, "State space search failed!\n");
        status = 1;
    }
    fprintf(stdout, "States:      %10ld\n", expanded);
    fprintf(stdout, "Transitions: %10ld\n", transitions);

    /* Clean-up */
cleanup:
    if (set != NULL)
        set->destroy(set);
    if (deque != NULL)
        deque->destroy(deque);
    if (bytecode != NULL)
        bytecode_unload(bytecode);

    return status;
}

