#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include <stdbool.h>
#include <stdio.h>
#include <datastructures/Set.h>
#include <datastructures/Deque.h>
#include <nips_vm/bytecode.h>

/* A structure describing parameters used for searching.

    bytecode            NIPS VM bytecode of the model to use.
    queue               Deque instance to use for the BFS queue/DFS stack.
    visited             Set instance to use to record visited states.
    dfs                 Use depth-first search instead of breadth-first search.
    max_iterations      Maximum number of iterations to perform (0: no limit).
    report_fp           File to write status reports to.
    report_interval     Number of iterations between reporting.

    In the above, an iteration is a single state expansion.
*/
struct SearchParams
{
    st_bytecode *model;
    Deque       *queue;
    Set         *visited;
    bool        dfs;
    long        max_iterations;
    FILE        *report_fp;
    long        report_interval;
};

/* Does a state space search and returns 0, or -1 if an error occurs while
   executing. */
int search(const struct SearchParams *params);

#endif /* ndef SEARCH_H_INCLUDED */
