#include <stdint.h>
#include <pthread.h>

#include "data/FIFO_Queue.h"

#define TPOOL_USES_DETACHED 1

typedef void* (*function_task_t)(void* task_data);

typedef struct worker_thread
{
    /* Worker thread id, used for maintenance and identification purposes
     * All worker thread will get your own identifier and will be handler his own
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
    /* Store the count of workers actually running a task */
    size_t workers_running;

    size_t workers_in_waiting;

    worker_thread_t* worker_threads;

    pthread_mutex_t tpool_lock;
    pthread_mutex_t workers_lock;

    pthread_cond_t tpool_sync_tasks;

    _Atomic uint_fast8_t thread_pool_run;

    #if TPOOL_USES_DETACHED
    _Atomic uint_least8_t pool_begin_destroyed;
    #endif

    struct timespec* __wait;
    FIFO_queue_t* task_queue_safe;
} tpool_t;

bool tpool_sync(tpool_t* thread_pool);

bool tpool_init(int worker_count, tpool_t* thread_pool);

bool tpool_stop(tpool_t* thread_pool);

bool tpool_finalize(tpool_t* thread_pool);

size_t tpool_workers(const tpool_t* thread_pool);

int tpool_wait_ava(tpool_t* thread_pool);

bool tpool_execute(function_task_t task_operation, void* task_data, tpool_t* thread_pool);

void* tpool_wait_for_result(function_task_t task_operation, void* task_data, tpool_t* thread_pool);
