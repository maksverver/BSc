#include <stdio.h>
#include "Bender_Impl.h"

int main()
{
    Bender_Impl bi;
    Bender_Impl_create(&bi, "db-test", 16);

    /*
    printf("%d\n", Bender_Impl_insert(&bi, "foo", 3));
    printf("%d\n", Bender_Impl_insert(&bi, "bar", 3));
    printf("%d\n", Bender_Impl_insert(&bi, "baz", 3));
    printf("%d\n", Bender_Impl_insert(&bi, "quux", 4));
    printf("%d\n", Bender_Impl_insert(&bi, "woei", 4));
    printf("%d\n", Bender_Impl_insert(&bi, "blaat", 5));
    */

    printf("%d\n", Bender_Impl_insert(&bi, "a", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "b", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "c", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "d", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "e", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "f", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "g", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "h", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "i", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "j", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "k", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "l", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "m", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "n", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "o", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "p", 1));
    printf("%d\n", Bender_Impl_insert(&bi, "q", 1));

    Bender_Impl_destroy(&bi);

    return 0;
}
