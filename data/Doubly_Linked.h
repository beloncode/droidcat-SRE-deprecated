#ifndef DATA_DOUBLY_LINKED_H
#define DATA_DOUBLY_LINKED_H

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct doubly_node
{
    void* user_data;

    int doubly_id;

    _Atomic uint_fast8_t node_valid;

    struct doubly_node* node_next;
    struct doubly_node* node_prev;

} doubly_node_t;

typedef struct doubly_linked
{
    size_t node_bank_size;

    size_t nodes_valid_cnt;

    doubly_node_t* node_bank;

} doubly_linked_t;

typedef enum { DOUBLY_INSERT_BEGIN, DOUBLY_INSERT_END } doubly_insert_e;

typedef bool (*doubly_foreach_t)(doubly_node_t* node_link, void* call_data);

doubly_linked_t* doubly_create(int64_t preallocate);
bool doubly_destroy(doubly_linked_t* doubly_ctx);

static inline bool doubly_is_valid(doubly_node_t* check_node)
{
    return check_node->node_valid != 0;
}

doubly_node_t* doubly_invalid_node(doubly_linked_t* doubly_ctx);

doubly_node_t* doubly_reserve(void* user_data, doubly_linked_t* doubly_ctx);

doubly_node_t* doubly_by_index(size_t node_index, doubly_linked_t* doubly_ctx);
doubly_node_t* doubly_by_id(size_t node_id, doubly_linked_t* doubly_ctx);

void* doubly_remove(doubly_node_t* remove_node, doubly_linked_t* doubly_ctx);
int doubly_insert(void* user_data, doubly_insert_e at, int location_opt, doubly_linked_t* doubly_ctx);

/* Gets the next or the previous node from a node */
doubly_node_t* doubly_next(doubly_node_t* node_item);
doubly_node_t* doubly_prev(doubly_node_t* node_item);

doubly_node_t* doubly_next_valid(doubly_node_t* node_item);

int doubly_clean(doubly_linked_t* doubly_ctx);
int doubly_foreach(doubly_foreach_t callback, void* call_data, doubly_linked_t* doubly_ctx);

size_t doubly_count(doubly_linked_t* doubly_ctx);
size_t doubly_capacity(const doubly_linked_t* doubly_ctx);

bool doubly_exist(doubly_node_t* exist_node, doubly_linked_t* doubly_ctx);

bool doubly_node_clean(doubly_node_t* node_item);

doubly_node_t* doubly_last(doubly_linked_t* doubly_ctx);
doubly_node_t* doubly_head(doubly_linked_t* doubly_ctx);

/* Resizes the queue capacity, how biggest the initial value be, more performance will have,
 * but more memory is needed!
*/
bool doubly_resize(int64_t new_capacity, doubly_linked_t* doubly_ctx);
bool doubly_sync(doubly_linked_t* doubly_ctx);

#endif

