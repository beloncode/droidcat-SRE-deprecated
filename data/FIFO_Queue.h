#ifndef DATA_FIFO_QUEUE_H
#define DATA_FIFO_QUEUE_H

#include <pthread.h>

#include "Doubly_Linked.h"

typedef void (*at_FIFO_destroy_t)(void* fifo_context);
typedef void (*at_FIFO_dequeue_t)(void* fifo_node);

typedef struct FIFO_queue
{
    doubly_linked_t* node_head;
    
    doubly_linked_t* node_tail;

    _Atomic size_t queue_length;

    _Atomic size_t queue_curr_cap;

    pthread_mutex_t* queue_lock;

    _Atomic uint_fast8_t begin_destroy;

    at_FIFO_destroy_t destroy_callback;

    at_FIFO_dequeue_t dequeue_callback;

} FIFO_queue_t;

FIFO_queue_t* fifo_queue_create(int64_t preallocate);

int fifo_queue_at_destroy(at_FIFO_destroy_t new_callback, FIFO_queue_t* fifo_queue);

int fifo_queue_at_dequeue(at_FIFO_dequeue_t new_callback, FIFO_queue_t* fifo_queue);

int fifo_queue_safe_lock(FIFO_queue_t* fifo_queue);

int fifo_enqueue(void* user_data, FIFO_queue_t* fifo_queue);

void* queue_dequeue(FIFO_queue_t* fifo_queue);

int fifo_queue_destroy(FIFO_queue_t *fifo_queue);

size_t fifo_queue_length(FIFO_queue_t *fifo_queue);

int fifo_queue_empty(FIFO_queue_t *fifo_queue);

int fifo_queue_full(FIFO_queue_t *fifo_queue);

#endif
