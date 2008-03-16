#include "Set.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static Set *set = NULL;

static const char *make_tempfile()
{
    int res;
    static char path[] = "db-XXXXXX";

    res = mkstemp(path);
    assert(res >= 0);
    close(res);

    return path;
}

static void parse_args(int argc, char *argv[])
{
    int opt;
    const char *path = NULL;
    bool keepdb = false;
    int type = -1, parameter;

    while ((opt = getopt(argc, argv, "bhc:d:k")) != -1)
    {
        switch (opt)
        {
        case 'b':
            assert(type == -1);
            type = 0;
            break;

        case 'h':
            assert(type == -1);
            type = 1;
            break;

        case 'c':
            assert(type == -1);
            type = 2;
            parameter = atoi(optarg);
            break;

        case 'd':
            assert(type == -1);
            type = 3;
            parameter = atoi(optarg);
            break;

        case 'k':
            keepdb = true;
            break;

        default:
            fprintf(stderr, "Unknown option: %c\n", opt);
            exit(1);
        }
    }

    if (type == -1)
    {
        printf("Usage:\n"
               "\ttest -b [opts]            (BerkeleyDB B-tree backed)\n"
               "\ttest -h [opts]            (BerkeleyDB hash table backed)\n"
               "\ttest -c pagesize [opts]   (Custom B-tree backed)\n"
               "\ttest -d capacity [opts]   (Custom hash table backed)\n"
               "Options:\n"
               "\t-k      Keep database file.\n");
        exit(0);
    }

    /* Create set */
    path = make_tempfile();
    switch (type)
    {
    case 0:
        set = BDB_Btree_Set_create(path);
        break;

    case 1:
        set = BDB_Hash_Set_create(path);
        break;

    case 2:
        set = Btree_Set_create(path, parameter);
        break;

    case 3:
        set = Hash_Set_create(path, parameter);
        break;
    }
    if (!keepdb)
        unlink(path);
}

int main(int argc, char *argv[])
{
    char line[4096];

    parse_args(argc, argv);

    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        if (line[0] == '?')
        {
            fputs( set->contains(set, line + 1, strlen(line) - 2) ?
                    "Present.\n" : "Absent.\n",  stdout );
        }
        else
        {
            fputs( set->insert(set, line, strlen(line) - 1) ?
                    "Replaced.\n" : "Added.\n",  stdout );
        }
    }

    set->destroy(set);

    return 0;
}
