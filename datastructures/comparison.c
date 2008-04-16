#include <stdlib.h>
#include <string.h>

/* Lexicographical comparison. */
int default_compare( const void *ignored,
                     const void *d1, size_t s1,
                     const void *d2, size_t s2 )
{
    int dif;

    dif = memcmp(d1, d2, s1 < s2 ? s1 : s2);
    if (dif == 0)
    {
        if (s1 < s2)
            dif = -1;
        else
        if (s1 > s2)
            dif = +1;
    }
    return dif;
}
