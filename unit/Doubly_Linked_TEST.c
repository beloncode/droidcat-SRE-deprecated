#include <stdio.h>
#include <assert.h>

#include "data/Doubly_Linked.h"

int main()
{
    /* Wether no one preallocate value as specifies, 
     * the doubly linked system must allocate 2 nodes by default 
    */
    doubly_linked_t* linked_new = doubly_create(0);

    assert(doubly_capacity(linked_new) == 2);

    doubly_destroy(linked_new);

    return 0;
}

