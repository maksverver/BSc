#include "Set.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    Set *set;
    char line[4096];

    set = Set_create_from_args(argc - 1, (const char**)argv + 1);

    if (set == NULL)
    {
        fprintf(stderr, "Unable to create set with given parameters!\n");
        exit(1);
    }

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
