#include <malloc.h>
#include <assert.h>

#include "FIFO_Queue.h"

FIFO_queue_t* queue_create(int64_t preallocate)
{
    FIFO_queue_t* heap_queue = (FIFO_queue_t*)calloc(1, sizeof(FIFO_queue_t));

    if (heap_queue == NULL) {}

    heap_queue->node_head = doubly_head(preallocate);

    heap_queue->node_tail = doubly_last(heap_queue->node_head);

    return heap_queue;
}

int queue_at_destroy(at_FIFO_destroy_t new_callback, FIFO_queue_t* fifo_queue)
{
    fifo_queue->destroy_callback = new_callback;
    return 0;
}

int queue_at_dequeue(at_FIFO_dequeue_t new_callback, FIFO_queue_t* fifo_queue)
{
    fifo_queue->dequeue_callback = new_callback;
    return 0;
}

int queue_safe_lock(FIFO_queue_t* fifo_queue)
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

    return 0;
}

static int queue_sync(FIFO_queue_t* fifo_queue)
{
    /* The mutex must be locked for does this */
    int trylock_ret = pthread_mutex_trylock(fifo_queue->queue_lock);
    
    assert(trylock_ret != 0);
    
    fifo_queue->queue_curr_cap = doubly_capacity(fifo_queue->node_head);
    
    fifo_queue->queue_length = doubly_count(fifo_queue->node_head);
    
    fifo_queue->node_tail = doubly_last(fifo_queue->node_head);

    return 0;
}

int queue_enqueue(void* user_data, FIFO_queue_t* fifo_queue)
{
    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_lock(fifo_queue->queue_lock);
    }

    doubly_insert(user_data, fifo_queue->node_head);

    queue_sync(fifo_queue);

    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_unlock(fifo_queue->queue_lock);
    }

    return 0;
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
    
    doubly_linked_t* next_valid = doubly_next_valid(fifo_queue->node_head);

    if (next_valid != NULL)
    {
        node_data = doubly_remove(next_valid, fifo_queue->node_head);
    } 

    queue_sync(fifo_queue);

    if (fifo_queue->queue_lock != NULL)
    {
        pthread_mutex_unlock(fifo_queue->queue_lock);
    }

    return node_data;
}

/* Dequeue and remove his allocated memory! */
void* queue_remove(FIFO_queue_t* fifo_queue)
{
    return NULL;
}

/* Resize the capacity of the queue */
int queue_resize(int desired_capacity, FIFO_queue_t* fifo_queue)
{
    return 0;
}

int queue_destroy(FIFO_queue_t *fifo_queue)
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

    doubly_destroy_head(fifo_queue->node_head);

    fifo_queue->queue_length = fifo_queue->queue_curr_cap = 0;
    fifo_queue->node_head = fifo_queue->node_tail = NULL;

    if (*queue_mutex_ptr)
    {
        pthread_mutex_unlock(*queue_mutex_ptr);
        pthread_mutex_destroy(*queue_mutex_ptr);

        free((void*)*queue_mutex_ptr);
    }

    free((void*)fifo_queue);

    return 0;

}

size_t queue_length(const FIFO_queue_t *fifo_queue)
{
    return fifo_queue->queue_length;
}

int queue_empty(const FIFO_queue_t *fifo_queue)
{
    return queue_length(fifo_queue) != 0;
}

int queue_full(const FIFO_queue_t *fifo_queue)
{
    return fifo_queue->queue_length == fifo_queue->queue_curr_cap;
}

