#include <malloc.h>
#include <assert.h>

#include "FIFO_Queue.h"

FIFO_queue_t* queue_create(int64_t preallocate)
{
    FIFO_queue_t* heap_queue = (FIFO_queue_t*)calloc(1, sizeof(FIFO_queue_t));

    if (heap_queue == NULL) {}

    heap_queue->doubly_context = doubly_create(preallocate);

    heap_queue->node_head = doubly_head(heap_queue->doubly_context);

    heap_queue->node_tail = doubly_last(heap_queue->doubly_context);

    return heap_queue;
}

void queue_at_destroy(at_FIFO_destroy_t new_callback, FIFO_queue_t* fifo_queue)
{
    fifo_queue->destroy_callback = new_callback;
}

void queue_at_dequeue(at_FIFO_dequeue_t new_callback, FIFO_queue_t* fifo_queue)
{
    fifo_queue->dequeue_callback = new_callback;
}

bool queue_safe_lock(FIFO_queue_t* fifo_queue)
{
    pthread_mutex_t** queue_lock_ptr = &fifo_queue->queue_lock;

    assert(*queue_lock_ptr == NULL);

    *queue_lock_ptr = calloc(1, sizeof(pthread_mutex_t));

    if (*queue_lock_ptr == NULL)
    {

    }

    pthread_mutex_init(*queue_lock_ptr, NULL);

    /* Attempting to test the lock */
    pthread_mutex_lock(*queue_lock_ptr);

    pthread_mutex_unlock(*queue_lock_ptr);

    return true;
}

bool queue_sync(FIFO_queue_t* fifo_queue)
{
    /* The mutex must be locked for does this */
    int trylock_ret = pthread_mutex_trylock(fifo_queue->queue_lock);
    
    assert(trylock_ret != 0);
    
    fifo_queue->queue_actual_capacity = doubly_capacity(fifo_queue->doubly_context);
    
    fifo_queue->queue_length = doubly_count(fifo_queue->doubly_context);

    fifo_queue->node_head = doubly_head(fifo_queue->doubly_context);

    fifo_queue->node_tail = doubly_last(fifo_queue->doubly_context);

    return true;
}

bool queue_enqueue(void* user_data, FIFO_queue_t* fifo_queue)
{
    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_lock(fifo_queue->queue_lock);
    }

    doubly_insert(user_data, DOUBLY_INSERT_END, 0, fifo_queue->doubly_context);

    queue_sync(fifo_queue);

    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_unlock(fifo_queue->queue_lock);
    }

    return true;
}

bool queue_enqueue_inverse(void* user_data, FIFO_queue_t* fifo_queue)
{
    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_lock(fifo_queue->queue_lock);
    }

    doubly_insert(user_data, DOUBLY_INSERT_BEGIN, 0, fifo_queue->doubly_context);

    queue_sync(fifo_queue);

    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_unlock(fifo_queue->queue_lock);
    }

    return true;
}

/* Dequeue the element from the queue, but doesn't deallocate
 * the node memory (performance purposes)!
*/
void* queue_dequeue(FIFO_queue_t* fifo_queue)
{
    if (fifo_queue->queue_length == 0) return NULL;

    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_lock(fifo_queue->queue_lock);
    }
    
    void* node_data = NULL;
    
    doubly_node_t* node_head = fifo_queue->node_head;
    
    if (node_head != NULL)
    {
        node_data = doubly_remove(node_head, fifo_queue->doubly_context);
    }
    
    queue_sync(fifo_queue);

    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_unlock(fifo_queue->queue_lock);
    }

    return node_data;
}

void* queue_dequeue_inverse(FIFO_queue_t* fifo_queue)
{
    if (fifo_queue->queue_length == 0) return NULL;
    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_lock(fifo_queue->queue_lock);
    }
    void* node_data = NULL;

    doubly_node_t* node_tail = fifo_queue->node_tail;

    if (node_tail == NULL)
    {
        return NULL;
    }
    node_data = doubly_remove(node_tail, fifo_queue->doubly_context);
    
    queue_sync(fifo_queue);
    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_unlock(fifo_queue->queue_lock);
    }
    return node_data;
}

/* Resize the capacity of the queue */
bool queue_resize(int desired_capacity, FIFO_queue_t* fifo_queue)
{
    return doubly_resize(desired_capacity, fifo_queue->doubly_context);
}

bool queue_destroy(FIFO_queue_t *fifo_queue)
{
    pthread_mutex_t** queue_mutex_ptr = &fifo_queue->queue_lock;
    
    if (*queue_mutex_ptr)
    {
        pthread_mutex_lock(*queue_mutex_ptr);
    }

    fifo_queue->begin_destroy = 1;

    if (fifo_queue->destroy_callback)
    {
        fifo_queue->destroy_callback(fifo_queue);
    }

    doubly_destroy(fifo_queue->doubly_context);

    fifo_queue->queue_length = fifo_queue->queue_actual_capacity = 0;
    fifo_queue->node_head = fifo_queue->node_tail = NULL;

    if (*queue_mutex_ptr)
    {
        pthread_mutex_unlock(*queue_mutex_ptr);
        pthread_mutex_destroy(*queue_mutex_ptr);

        free((void*)*queue_mutex_ptr);
    }

    free((void*)fifo_queue);

    return true;

}

size_t queue_length(const FIFO_queue_t *fifo_queue)
{
    return fifo_queue->queue_length;
}

size_t queue_capacity(const FIFO_queue_t *fifo_queue)
{
    return fifo_queue->queue_actual_capacity;
}

bool queue_empty(const FIFO_queue_t *fifo_queue)
{
    return queue_length(fifo_queue) == 0;
}

bool queue_full(const FIFO_queue_t *fifo_queue)
{
    return queue_length(fifo_queue) == queue_capacity(fifo_queue);
}

