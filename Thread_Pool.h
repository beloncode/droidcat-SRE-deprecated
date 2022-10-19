#include <stdint.h>
#include <pthread.h>

#include "data/FIFO_Queue.h"

typedef void* (*function_task_t)(void* task_data);

typedef struct worker_thread
{
    /* Worker thread id, used for maintainence and identification purposes 
     * All worker thread will got your own identifier and will be handler his own
     * resources!
    */
    uint32_t worker_id;
    /* Thread control identifier from POSIX thread */
    pthread_t worker_sched;

    _Atomic uint_least8_t can_cancel;

} worker_thread_t;

typedef struct tpool 
{
    size_t worker_cnt;

    worker_thread_t* worker_threads;

    pthread_mutex_t tpool_lock;

    pthread_cond_t tpool_sync_tasks;

    _Atomic uint_fast8_t thread_pool_run;

    FIFO_queue_t* task_queue_safe;
    
} tpool_t;

int tpool_sync(tpool_t* thread_pool);

int tpool_init(int worker_count, tpool_t* thread_pool);

int tpool_stop(tpool_t* thread_pool);

int tpool_finalize(tpool_t* thread_pool);
