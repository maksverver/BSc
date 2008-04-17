#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include <stdbool.h>
#include <datastructures/Set.h>
#include <datastructures/Deque.h>
#include <nips_vm/bytecode.h>

/* Does a state space search and returns the total number of states expanded,
   and returns 0, or -1 if an error occurs while executing.
   Initially,  ``queue'' and ``visited'' should be empty.

   If ``dfs'' is set to true, a depth-first search is performed; otherwise,
   a breadth-first search is performed.
*/
int search( st_bytecode *bytecode, Deque *queue, Set *visited, bool dfs,
            long *expanded, long *transitions );

#endif /* ndef SEARCH_H_INCLUDED */
