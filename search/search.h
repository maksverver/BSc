#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include <stdbool.h>
#include <datastructures/Set.h>
#include <datastructures/Deque.h>
#include <nips_vm/nipsvm.h>
#include <nips_vm/bytecode.h>

/* Does a state space search and returns the total number of states visited,
   or -1 if an error occurs while executing. Initially,  ``queue'' and
   ``visited'' must be empty.

   If ``dfs'' is set to true, a depth-first search is performed; otherwise,
   a breadth-first search is performed.
*/
long search( nipsvm_t *vm, nipsvm_state_t *initial_state,
             Deque *queue, Set *visited, bool dfs );

#endif /* ndef SEARCH_H_INCLUDED */
