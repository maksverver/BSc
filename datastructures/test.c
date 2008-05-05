#include <stdio.h>
#include "Bender_Impl.h"

int main()
{
    Bender_Impl bi;
    Bender_Impl_create(&bi, "db-test", 16);

    printf("%d\n", Bender_Impl_insert(&bi, "foo", 3));
    printf("%d\n", Bender_Impl_insert(&bi, "bar", 3));
    printf("%d\n", Bender_Impl_insert(&bi, "baz", 3));
    printf("%d\n", Bender_Impl_insert(&bi, "quux", 4));
    printf("%d\n", Bender_Impl_insert(&bi, "woei", 4));
    printf("%d\n", Bender_Impl_insert(&bi, "blaat", 5));

    Bender_Impl_destroy(&bi);

    return 0;
}
