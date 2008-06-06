#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "search.h"
#include "nips_vm/bytecode.h"

static const char   *opt_bytecode_path      = NULL;
static long         opt_max_iterations      = 0;
static long         opt_report_interval     = 0;
static bool         opt_dfs                 = false;
static Set          *set                    = NULL;

static void usage()
{
    printf(
        "Usage:\n"
        "    search [<options>] (set description)\n"
        "Options:\n"
        "    -B          -- use breadth-first search (default)\n"
        "    -D          -- use depth-first search\n"
        "    -m model    -- path to model bytecode file\n"
        "    -l cnt      -- iteration limit\n"
        "    -i cnt      -- reporting interval\n"
        );
    exit(1);
}

static void parse_args(int argc, char *argv[])
{
    int ch;
    bool bfs = false, dfs = false;

    if (argc < 2) usage();

    while ((ch = getopt(argc, argv, "BDm:l:i:")) >= 0)
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

        case 'l':
            opt_max_iterations = atol(optarg);
            if (opt_max_iterations <= 0)
            {
                printf("Iteration limit must be a positive integer!\n\n");
                usage();
            }
            break;

        case 'i':
            opt_report_interval = atol(optarg);
            if (opt_report_interval <= 0)
            {
                printf("Reporting interval must be a positive integer!\n\n");
                usage();
            }
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
    if (bfs || dfs)
    {
        opt_dfs = dfs;
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
        printf("Could not create set (invalid description?)\n\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    int status = 0;
    struct SearchParams params;

    parse_args(argc, argv);

    params.visited         = set;
    params.dfs             = opt_dfs;
    params.max_iterations  = opt_max_iterations;
    params.report_fp       = stdout;
    params.report_interval = opt_report_interval;

    /* Load bytecode from file */
    params.model = bytecode_load_from_file(opt_bytecode_path, NULL);
    if (params.model == NULL)
    {
        fprintf( stderr, "Could not load bytecode from file \"%s\": ",
                         opt_bytecode_path );
        perror(NULL);
        status = 1;
        goto cleanup;
    }

    /* Create deque data structure */
    params.queue = File_Deque_create(NULL);
    if (params.queue == NULL)
    {
        perror("Could not create deque");
        status = 1;
        goto cleanup;
    }

    /* Do search */
    if (search(&params) < 0)
    {
        fprintf(stderr, "State space search failed!\n");
        status = 1;
    }

    /* Clean-up */
cleanup:
    if (params.visited != NULL)
        params.visited->destroy(params.visited);
    if (params.queue != NULL)
        params.queue->destroy(params.queue);
    if (params.model != NULL)
        bytecode_unload(params.model);

    return status;
}

