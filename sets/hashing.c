#include <stdlib.h>

/*  32-bit FNV-1a hash function.
    Glenn Fowler, Phong Vo, Landon Curt Noll.

    See also:
    http://isthe.com/chongo/tech/comp/fnv/
*/
unsigned default_hash(const void *ignored, const void *data, size_t size)
{
    unsigned hash = 2166136261;

    while (size-- > 0)
    {
        hash ^= *(unsigned char*)data++;
        hash *= 16777619;
    }

    return hash;
}
