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

    assert(doubly_count(linked_new) == false);
    assert(doubly_exist(NULL, linked_new) == false);

    /* Manually pushing test, simulating a queue based linked list */
    static int values[10] = { 22, 324, 54, 23, 7433, 324, 23432, 1243257344, 5765, 34678 };
    doubly_insert(&values[0], DOUBLY_INSERT_BEGIN, 0, linked_new);
    doubly_insert(&values[2], DOUBLY_INSERT_BEGIN, 0, linked_new);
    /* This is the head now! */
    doubly_insert(&values[1], DOUBLY_INSERT_BEGIN, 0, linked_new);

    // removing doubly_insert(&values[0], DOUBLY_INSERT_BEGIN, 0, linked_new);
    int* value = (int*)doubly_remove(doubly_by_id(2, linked_new), linked_new);
    // removing doubly_insert(&values[1], DOUBLY_INSERT_BEGIN, 0, linked_new);
    doubly_remove(doubly_head(linked_new), linked_new);
    // removing doubly_insert(&values[2], DOUBLY_INSERT_BEGIN, 0, linked_new);
    value = (int*)doubly_remove(doubly_head(linked_new), linked_new);
    assert(*value == values[2]);

    doubly_destroy(linked_new);

    return 0;
}

