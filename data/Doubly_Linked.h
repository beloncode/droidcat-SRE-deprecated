#ifndef DATA_DOUBLY_LINKED_H
#define DATA_DOUBLY_LINKED_H

#include <stdint.h>
#include <stdatomic.h>

typedef struct doubly_linked
{
    int doubly_id;

    void* user_data;

    _Atomic uint_fast8_t node_valid;

    size_t node_bank_size;

    struct doubly_linked* node_bank;

    struct doubly_linked* node_next;
    struct doubly_linked* node_prev;

} doubly_linked_t;

typedef int (*doubly_foreach_t)(doubly_linked_t* node_link, void* call_data);

doubly_linked_t* doubly_head(int64_t preallocate);

int doubly_destroy_head(doubly_linked_t* doubly_head);

doubly_linked_t* doubly_invalid_node(doubly_linked_t* doubly_head);

doubly_linked_t* doubly_reserve(void* user_data, doubly_linked_t* doubly_head);

doubly_linked_t* doubly_retr(size_t node_index, doubly_linked_t* doubly_head);

int doubly_insert(void* user_data, doubly_linked_t* doubly_head);

doubly_linked_t* doubly_next(doubly_linked_t* node_item);

doubly_linked_t* doubly_prev(doubly_linked_t* node_item);

int doubly_foreach(doubly_foreach_t callback, void* call_data, doubly_linked_t* doubly_head);

doubly_linked_t* doubly_next_valid(doubly_linked_t* node_item);

void* doubly_remove(doubly_linked_t* remove_node, doubly_linked_t* doubly_head);

size_t doubly_count(doubly_linked_t* doubly_head);

size_t doubly_capacity(const doubly_linked_t* doubly_head);

int doubly_exist(doubly_linked_t* exist_node, doubly_linked_t* doubly_head);

int doubly_node_clean(doubly_linked_t* node_item);

doubly_linked_t* doubly_last(doubly_linked_t* head_doubly);

#endif

