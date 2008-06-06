#define _GNU_SOURCE

#ifndef HAVE_MREMAP
#  ifdef __linux__
#    define HAVE_MREMAP 1	
#  else
#    define HAVE_MREMAP 0
#  endif
#endif 

#define ALLOC_CHUNK_SIZE 4096
