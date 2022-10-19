#include <malloc.h>
#include <assert.h>

#include "Doubly_Linked.h"

doubly_linked_t* doubly_head(int64_t preallocate)
{
    doubly_linked_t* heap_doubly = (doubly_linked_t*) calloc(1, sizeof(doubly_linked_t));

    size_t preallocate_size = preallocate;

    if (preallocate_size == 0)
    {
        preallocate_size+=2;
    }

    heap_doubly->node_bank_size = preallocate_size;
    heap_doubly->node_bank = (doubly_linked_t*)calloc(preallocate_size, sizeof(*heap_doubly));

    if (heap_doubly->node_bank == NULL) {}

    return heap_doubly;
}

int doubly_destroy_head(doubly_linked_t* doubly_head)
{
    if (doubly_head->node_bank != NULL)
    {
        free((void*)doubly_head->node_bank);
    }

    free((void*)doubly_head);

    return 0;
}

doubly_linked_t* doubly_invalid_node(doubly_linked_t* doubly_head)
{
    if (doubly_head->node_bank == NULL)
    {
        return NULL;
    }

    for (size_t node_cur = 0; node_cur < doubly_head->node_bank_size; node_cur++)
    {
        if (doubly_head->node_bank[node_cur].node_valid == 0)
        {
            return &doubly_head->node_bank[node_cur];
        }
    }
    return NULL;
}

doubly_linked_t* doubly_reserve(void* user_data, doubly_linked_t* doubly_head)
{
    doubly_linked_t* reserved_node = doubly_invalid_node(doubly_head);
    
    if (reserved_node != NULL)
    {
        reserved_node->node_valid = 1;
        
        reserved_node->user_data = user_data;

        return reserved_node;
    }

    doubly_linked_t** doubly_bank_ptr = &doubly_head->node_bank;
    
    /* Reallocates more memory, more specif twice the memory */
    assert(*doubly_bank_ptr != NULL);
    
    doubly_head->node_bank_size *= 2;

    doubly_linked_t* new_bank = realloc(doubly_head->node_bank, doubly_head->node_bank_size);

    if (new_bank == NULL) {}

    doubly_head->node_bank = new_bank;

    return doubly_reserve(user_data, doubly_head);

}

static int doubly_insert_at_end(doubly_linked_t* node_link, void* node_info)
{
    doubly_linked_t* node_item = (doubly_linked_t*)node_info;
    
    if (node_link->node_next != NULL)
    {
        return 0;
    }

    node_link->node_next = node_item;

    node_item->node_prev = node_link;

    return 1;
}

doubly_linked_t* doubly_retr(size_t node_index, doubly_linked_t* doubly_head)
{
    if (node_index >= doubly_head->node_bank_size)
    {
        return NULL;
    }
    return &doubly_head->node_bank[node_index];
}

doubly_linked_t* doubly_next(doubly_linked_t* node_item)
{
    return node_item->node_next;
}

doubly_linked_t* doubly_prev(doubly_linked_t* node_item)
{
    return node_item->node_prev;
}

int doubly_foreach(doubly_foreach_t callback, void* call_data, doubly_linked_t* doubly_head)
{
    int nodes_cur = 0;

    int call_ret = callback(doubly_head, call_data);

    if (call_ret) return call_ret;

    for (size_t bank_cur = 0; bank_cur < doubly_head->node_bank_size; bank_cur++)
    {
        int call_ret = callback(doubly_retr(nodes_cur++, doubly_head), call_data);

        if (call_ret) return call_ret;
    }

    return -1;
}

doubly_linked_t* doubly_next_valid(doubly_linked_t* node_item)
{
    node_item = node_item->node_next;
    while (node_item != NULL)
    {
        if (node_item->node_valid)
        {
            return node_item;
        }
        node_item = node_item->node_next;
    }
    return NULL;
}

size_t doubly_count(doubly_linked_t* doubly_head)
{
    if (doubly_head == NULL)
    {
        return 0;
    }

    doubly_linked_t* real_link = doubly_retr(0, doubly_head);

    if (real_link == NULL)
    {
        return 1;
    }

    int valid_nodes = 1;

    while ((real_link = doubly_next_valid(real_link))) valid_nodes++;

    return valid_nodes;
}

static int doubly_test_element(doubly_linked_t* node_link, void* node_info)
{
    doubly_linked_t* node_item = (doubly_linked_t*)node_info;

    if (node_item != node_link) return 0;

    return 1;
}

size_t doubly_capacity(const doubly_linked_t* doubly_head)
{
    return doubly_head->node_bank_size;
}

int doubly_exist(doubly_linked_t* exist_node, doubly_linked_t* doubly_head)
{
    return doubly_foreach(doubly_test_element, exist_node, doubly_head) != -1;
}

int doubly_node_clean(doubly_linked_t* node_item)
{
    node_item->user_data = NULL;
    node_item->node_valid = 0;

    node_item->node_next = node_item->node_prev = NULL;

    return 0;
}

static int doubly_clean_element(doubly_linked_t* node_link, void* node_info)
{
    return doubly_node_clean(node_link);
}

static int doubly_select_last(doubly_linked_t* node_link, void* user_data)
{
    doubly_linked_t** last_ptr = (doubly_linked_t**)user_data;
    
    if (node_link->node_valid != 1) return 0;

    if (node_link->node_next != NULL) return 0;

    *last_ptr = node_link;

    return 1;
}

doubly_linked_t* doubly_last(doubly_linked_t* head_doubly)
{
    doubly_linked_t* last_node = NULL;
    doubly_foreach(doubly_select_last, &last_node, head_doubly);
    return last_node;
}

int doubly_clean(doubly_linked_t* doubly_head)
{
    return doubly_foreach(doubly_clean_element, NULL, doubly_head);
}

void* doubly_remove(doubly_linked_t* remove_node, doubly_linked_t* doubly_head)
{
    if (doubly_exist(remove_node, doubly_head) == 0) return NULL;

    assert(remove_node->node_valid == 1);

    if (remove_node->node_next != NULL)
    {
        remove_node->node_next->node_prev = remove_node->node_prev;
    }

    if (remove_node->node_prev != NULL)
    {
        remove_node->node_prev->node_next = remove_node->node_next;
    }

    void* node_content = remove_node->user_data;

    doubly_node_clean(remove_node);

    return node_content;
}

int doubly_insert(void* user_data, doubly_linked_t* doubly_head)
{
    if (doubly_head->node_prev == NULL)
    {
        /* |-<[NODE 0]>-| */
        /* |------------| */

        doubly_head->node_prev = doubly_head->node_next;
        doubly_head->node_next = doubly_head->node_prev;

        doubly_head->user_data = user_data;
        doubly_head->node_valid = 1;
        return 0;
    }

    doubly_linked_t* element_node = doubly_reserve(user_data, doubly_head);

    int each_ret = doubly_foreach(doubly_insert_at_end, (void*)element_node, doubly_head);

    return each_ret;
}

