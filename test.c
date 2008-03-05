#include "Set.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *filepath = "test.db";

#define PAGESIZE 128

int main(int argc, char *argv[])
{
    char line[4096];
    Set *set;

    set = Btree_Set_create(filepath, PAGESIZE);
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
    /*unlink(filepath);*/

    return 0;
}
