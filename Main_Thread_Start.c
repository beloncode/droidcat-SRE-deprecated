#include <malloc.h>
#include <stdio.h>

int main()
{
    int* x;

    x = malloc(100);

    *(x + 5) = 5;

    printf("%d - %p\n", *x, x);

    free(x);

    return x[5];
}
