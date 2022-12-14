#include <malloc.h>
#include <assert.h>

#include "Doubly_Linked.h"

doubly_linked_t* doubly_create(int64_t preallocate)
{
    doubly_linked_t* doubly_ctx = (doubly_linked_t*)calloc(1, sizeof(doubly_linked_t));

    size_t preallocate_size = preallocate;

    if (preallocate_size == 0)
    {
        preallocate_size+=2;
    }

    doubly_resize(preallocate_size, doubly_ctx);

    if (doubly_ctx->node_bank == NULL) {}

    return doubly_ctx;
}

static bool doubly_copy(doubly_node_t* dest, doubly_node_t* head_node, int64_t max_size)
{
    assert(head_node->doubly_id == 0);

    doubly_node_t* src = head_node;

    if (src == NULL || doubly_is_valid(src) == false)
    {
        return false;
    }

    doubly_node_t* node_prev = NULL;
    static int node_id;

    for (node_id = 0; max_size != 0 && src != NULL; max_size--)
    {
        dest->doubly_id = node_id++;
        dest->node_valid = src->node_valid;
        dest->user_data = src->user_data;
        /* Making the linking between the thw nodes */
        dest->node_prev = node_prev;
        if (dest->node_prev != NULL)
        {
            dest->node_prev->node_next = dest;
        }

        src = doubly_next(src);
        node_prev = dest++;
    }

    return true;
}

bool doubly_resize(int64_t new_capacity, doubly_linked_t* doubly_ctx)
{
    if (doubly_ctx->nodes_valid_cnt > new_capacity)
    {
        return false;
    }

    doubly_ctx->node_bank_size = new_capacity;
    
    if (doubly_ctx->node_bank == NULL)
    {
        doubly_ctx->node_bank = (doubly_node_t*)calloc(doubly_ctx->node_bank_size, sizeof(doubly_node_t));
        if (doubly_ctx->node_bank == NULL) {}
        return true;
    }
        
    /* Reallocates a new buffer and copies all valid elements into the new buffer field */
    doubly_node_t* new_node = (doubly_node_t*)calloc(doubly_ctx->node_bank_size, sizeof(doubly_node_t));
    
    if (new_node == NULL) {}
    
    assert(doubly_copy(new_node, doubly_head(doubly_ctx), new_capacity) != false);
    
    free((void*)doubly_ctx->node_bank);

    doubly_ctx->node_bank = new_node;
    
    return doubly_sync(doubly_ctx);
}

bool doubly_destroy(doubly_linked_t* doubly_ctx)
{
    if (doubly_ctx->node_bank != NULL)
    {
        free((void*)doubly_ctx->node_bank);
    }

    free((void*)doubly_ctx);

    return true;
}

/* Returns an invalid node, whether exist one */
doubly_node_t* doubly_invalid_node(doubly_linked_t* doubly_ctx)
{
    if (doubly_ctx->node_bank == NULL)
    {
        return NULL;
    }

    for (size_t node_cur = 0; node_cur < doubly_ctx->node_bank_size; node_cur++)
    {
        if (doubly_ctx->node_bank[node_cur].node_valid == 0)
        {
            return &doubly_ctx->node_bank[node_cur];
        }
    }
    return NULL;
}

doubly_node_t* doubly_reserve(void* user_data, doubly_linked_t* doubly_ctx)
{
    doubly_node_t* reserved_node = doubly_invalid_node(doubly_ctx);
    
    if (reserved_node != NULL)
    {
        reserved_node->node_valid = 1;
        
        reserved_node->user_data = user_data;

        return reserved_node;
    }
     
    /* Reallocates more memory, more specific twice the count of nodes inside the node */
    doubly_resize(doubly_ctx->node_bank_size * 2, doubly_ctx);

    return doubly_reserve(user_data, doubly_ctx);
}

static bool doubly_insert_at_end(doubly_node_t* node_link, void* node_info)
{
    doubly_node_t* node_item = (doubly_node_t*)node_info;
    
    if (node_link->node_next != NULL)
    {
        return false;
    }

    node_link->node_next = node_item;

    node_item->node_prev = node_link;

    return true;
}

static bool doubly_search_id(doubly_node_t* node_link, void* desired_node)
{
    /* This node must be fill with the desired ID, so on we can retrieve him and overwrite the pointer content */
    doubly_node_t** desired_node_item = (doubly_node_t**)desired_node;    

    if (node_link->doubly_id == (*desired_node_item)->doubly_id && node_link->node_valid)
    {
        *desired_node_item = node_link;
        return true;
    }

    return false;
}

doubly_node_t* doubly_by_id(size_t node_id, doubly_linked_t* doubly_ctx)
{
    static doubly_node_t desired_node_id = {};
    desired_node_id.doubly_id = node_id;

    doubly_node_t* found_node_ptr = &desired_node_id;

    doubly_foreach(doubly_search_id, &found_node_ptr, doubly_ctx);

    /* No one node has been found */
    if (found_node_ptr == &desired_node_id) found_node_ptr = NULL;
    return found_node_ptr;
}

doubly_node_t* doubly_head(doubly_linked_t* doubly_ctx)
{
    #define HEAD_NODE_INDEX 0
    return doubly_by_id(HEAD_NODE_INDEX, doubly_ctx);
}

doubly_node_t* doubly_by_index(size_t node_index, doubly_linked_t* doubly_ctx)
{
    if (node_index >= doubly_ctx->node_bank_size)
    {
        return NULL;
    }
    return &doubly_ctx->node_bank[node_index];
}

doubly_node_t* doubly_next(doubly_node_t* node_item)
{
    return node_item->node_next;
}

doubly_node_t* doubly_prev(doubly_node_t* node_item)
{
    return node_item->node_prev;
}

int doubly_foreach(doubly_foreach_t callback, void* call_data, doubly_linked_t* doubly_ctx)
{
    int nodes_cur;

    for (nodes_cur = 0; nodes_cur < doubly_ctx->node_bank_size; nodes_cur++)
    {
        if (call_data == NULL)
        {
            call_data = (void*)(uintptr_t)nodes_cur;
        }
        bool call_ret = callback(doubly_by_index(nodes_cur, doubly_ctx), call_data);

        if (call_ret) return call_ret;
    }

    return nodes_cur;
}

doubly_node_t* doubly_next_valid(doubly_node_t* node_item)
{
    node_item = node_item->node_next;
    
    while (node_item != NULL)
    {
        if (node_item->node_valid != 0)
        {
            return node_item;
        }
        node_item = node_item->node_next;
    }
    return NULL;
}

size_t doubly_count(doubly_linked_t* doubly_ctx)
{
    if (doubly_ctx == NULL)
    {
        return 0;
    }

    doubly_node_t* first_link = doubly_head(doubly_ctx);

    if (first_link == NULL) return 0;

    size_t valid_nodes = 0;

    if (first_link->node_valid != 0)
    {
        valid_nodes++;
    }

    while ((first_link = doubly_next_valid(first_link))) valid_nodes++;

    return valid_nodes;
}

static bool doubly_test_element(doubly_node_t* node_link, void* node_info)
{
    doubly_node_t* node_item = (doubly_node_t*)node_info;

    if (node_item != node_link) return false;

    return true;
}

size_t doubly_capacity(const doubly_linked_t* doubly_ctx)
{
    return doubly_ctx->node_bank_size;
}

bool doubly_exist(doubly_node_t* exist_node, doubly_linked_t* doubly_ctx)
{
    return doubly_foreach(doubly_test_element, exist_node, doubly_ctx) == true;
}

bool doubly_node_clean(doubly_node_t* node_item)
{
    node_item->user_data = NULL;
    node_item->node_valid = 0;

    node_item->node_next = node_item->node_prev = NULL;

    return true;
}

static bool doubly_clean_element(doubly_node_t* node_link, void* node_info)
{
    (void)node_info;
    return doubly_node_clean(node_link) == false;
}

static bool doubly_select_last(doubly_node_t* node_link, void* user_data)
{
    doubly_node_t** last_ptr = (doubly_node_t**)user_data;
    
    if (node_link->node_valid != 1) return false;

    *last_ptr = node_link;

    if (node_link->node_next == NULL) return false;

    return true;
}

doubly_node_t* doubly_last(doubly_linked_t* doubly_ctx)
{
    doubly_node_t* last_node = NULL;
    doubly_foreach(doubly_select_last, &last_node, doubly_ctx);
    return last_node;
}

int doubly_clean(doubly_linked_t* doubly_ctx)
{
    return doubly_foreach(doubly_clean_element, NULL, doubly_ctx);
}

void* doubly_remove(doubly_node_t* remove_node, doubly_linked_t* doubly_ctx)
{
    if (doubly_exist(remove_node, doubly_ctx) == 0) return NULL;

    assert(remove_node->node_valid == 1);

    if (remove_node->node_next != NULL)
    {
        remove_node->node_next->node_prev = remove_node->node_prev;
        /* We are removing the head node, then, passing the ownership id of the head for the next element node */
        if (doubly_head(doubly_ctx) == remove_node)
        {
            remove_node->node_next->doubly_id = remove_node->doubly_id;
        }
    }

    if (remove_node->node_prev != NULL)
    {
        remove_node->node_prev->node_next = remove_node->node_next;
    }

    void* node_content = remove_node->user_data;

    doubly_node_clean(remove_node);

    doubly_ctx->nodes_valid_cnt--;

    doubly_sync(doubly_ctx);

    return node_content;
}

static bool doubly_clean_id(doubly_node_t* node_link, void* user_data)
{
    (void)user_data;
    node_link->doubly_id = 0;
    return false;
}

bool doubly_sync(doubly_linked_t* doubly_ctx)
{
    int64_t actual_id;
    doubly_node_t* index_node = doubly_head(doubly_ctx);

    doubly_foreach(doubly_clean_id, NULL, doubly_ctx);

    for (actual_id = 0; index_node != NULL; actual_id++)
    {
        index_node->doubly_id = actual_id;
        index_node = index_node->node_next;
    }

    return true;
}

int doubly_insert(void* user_data, doubly_insert_e at, int location_opt, doubly_linked_t* doubly_ctx)
{
    doubly_node_t* selected_node = doubly_reserve(user_data, doubly_ctx);
    int each_ret = 0;
    
    switch (at)
    {
        default: case DOUBLY_INSERT_END:

        assert(location_opt == 0);

        if (doubly_head(doubly_ctx) == selected_node)
        {
            break;
        }

        each_ret = doubly_foreach(doubly_insert_at_end, (void*)selected_node, doubly_ctx);

        break;
        
        case DOUBLY_INSERT_BEGIN:

        assert(location_opt == 0);

        doubly_node_t* node_location = doubly_head(doubly_ctx);

        if (node_location == NULL) {}

        if (selected_node == node_location)
        {
            /* Node already placed at the correct location */
            break;
        }

        selected_node->doubly_id = 0;
        node_location->doubly_id = 1;
        selected_node->node_next = node_location;
        node_location->node_prev = selected_node;
        
        break;
    }

    doubly_ctx->nodes_valid_cnt++;
    doubly_sync(doubly_ctx);
    return each_ret;
}
