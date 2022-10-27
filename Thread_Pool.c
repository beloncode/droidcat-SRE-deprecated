#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "Thread_Pool.h"
#include "cpu/CPU_Time.h"

#define TPOOL_SYNC_NANO 1e+8 /* 10 miliseconds */

struct thread_task
{
    void* task_data;

    void* task_result;

    _Atomic uint_least8_t task_completed;

    _Atomic uint_least8_t task_in_wait;

    pthread_cond_t task_condition; 

    function_task_t task_function;
};

worker_thread_t* tpool_retrieve(pthread_t thread_native_id, tpool_t* thread_pool)
{
    pthread_mutex_lock(&thread_pool->workers_lock);

    worker_thread_t* worker_data = NULL;

    for (size_t worker_cur = 0; worker_cur < thread_pool->worker_cnt; worker_cur++)
    {
        if (thread_pool->worker_threads[worker_cur].worker_sched == thread_native_id)
        {
            worker_data = &thread_pool->worker_threads[worker_cur];

            break;
        }
    }

    pthread_mutex_unlock(&thread_pool->workers_lock);

    return worker_data;
}

static worker_thread_t* tpool_retrieve_self(tpool_t* thread_pool)
{
    return tpool_retrieve(pthread_self(), thread_pool);
}

static void* tpool_worker_routine(void* tpool)
{
    tpool_t* thread_pool = (tpool_t*)tpool;
    
    worker_thread_t* worker_content = tpool_retrieve_self(thread_pool);

    while (1)
    {
        worker_content->can_cancel = 1;

        pthread_mutex_lock(&thread_pool->workers_lock);
        pthread_cond_wait(&thread_pool->tpool_sync_tasks, &thread_pool->workers_lock);

        #if TPOOL_USES_DETACHED
        if (thread_pool->pool_begin_destroyed)
        {        
            pthread_mutex_lock(&thread_pool->tpool_lock);
            
            thread_pool->worker_cnt--;
            
            pthread_mutex_unlock(&thread_pool->tpool_lock);
            
            pthread_mutex_unlock(&thread_pool->workers_lock);
            
            pthread_exit(NULL);
        }
        #endif
    
        struct thread_task* acquired_task = queue_dequeue(thread_pool->task_queue_safe);
        
        /* From this stage, the worker thread can't be canceled */
        worker_content->can_cancel = 0;

        pthread_mutex_unlock(&thread_pool->workers_lock);

        if (acquired_task == NULL) continue;
        
        pthread_mutex_lock(&thread_pool->tpool_lock);

        thread_pool->workers_running++;

        pthread_mutex_unlock(&thread_pool->tpool_lock);

        acquired_task->task_result = acquired_task->task_function(acquired_task->task_data);

        if (acquired_task->task_in_wait)
        {
            /* Someone is waiting to the task begin completed */
            acquired_task->task_completed = 1;
            pthread_mutex_lock(&thread_pool->workers_lock);
            
            pthread_cond_signal(&acquired_task->task_condition);
            
            pthread_mutex_unlock(&thread_pool->workers_lock);
        }
        else
        {
            memset(acquired_task, 0, sizeof(*acquired_task));

            free((void*)acquired_task);
        }

        pthread_mutex_lock(&thread_pool->tpool_lock);

        thread_pool->workers_running--;

        pthread_mutex_unlock(&thread_pool->tpool_lock);
    }

    return NULL;
}

/* Will lock the caller thread for wait until the task 'task_operation' is finished 
 * result code from task_operation will be delivery for the caller as result!
*/
bool tpool_init(int worker_count, tpool_t* thread_pool) 
{
    memset(thread_pool, 0, sizeof(*thread_pool));
    
    thread_pool->worker_threads = calloc((int8_t)worker_count, sizeof(*thread_pool->worker_threads));

    thread_pool->worker_cnt = worker_count;

    pthread_mutex_init(&thread_pool->tpool_lock, NULL);

    pthread_mutex_init(&thread_pool->workers_lock, NULL);

    pthread_mutex_lock(&thread_pool->tpool_lock);

    pthread_cond_init(&thread_pool->tpool_sync_tasks, NULL);

    /* Preallocate all needed tasks */
    thread_pool->task_queue_safe = queue_create(worker_count);

    assert(thread_pool->task_queue_safe != NULL);

    /* Enable the safe-lock into the queue, every enqueue/dequeue operation will have a valid mutex */ 
    queue_safe_lock(thread_pool->task_queue_safe);

    if (thread_pool->worker_threads == NULL)
    {
        
    }

    worker_thread_t* worker_cur = thread_pool->worker_threads;

    for (uint8_t worker_index = 0; worker_index != worker_count; worker_index++)
    {
        worker_cur[worker_index].worker_id = worker_index;
        
        worker_cur[worker_index].can_cancel = 0;

        pthread_t* thread_posix = &worker_cur[worker_index].worker_sched;
        
        int posix_result = pthread_create(thread_posix, NULL, tpool_worker_routine, (void*)thread_pool);

        if (posix_result != 0) {}

        #if TPOOL_USES_DETACHED
        
        /* Detach the thread when his as done */
        
        int detach_ret = pthread_detach(*thread_posix);
        
        assert(detach_ret == 0);
        
        #endif

    }

    thread_pool->thread_pool_run = 1;

    pthread_mutex_unlock(&thread_pool->tpool_lock);

    tpool_sync(thread_pool);

    return true;
}

bool tpool_stop(tpool_t* thread_pool)
{
    pthread_mutex_t* mutex_lock = &thread_pool->tpool_lock;

    pthread_mutex_lock(mutex_lock);

    thread_pool->thread_pool_run = 0;

    pthread_mutex_unlock(mutex_lock);

    bool sync_ret = tpool_sync(thread_pool);

    return sync_ret;
}

size_t tpool_workers(const tpool_t* thread_pool)
{
    return thread_pool->worker_cnt;
}

/* Returns the number of canceled worker threads */
static int tpool_cancel(tpool_t* thread_pool)
{
    size_t worker_cur = 0;

    int workers_total = tpool_workers(thread_pool);

    #if TPOOL_USES_DETACHED

    while (tpool_workers(thread_pool) != 0)
    {
        pthread_cond_broadcast(&thread_pool->tpool_sync_tasks);
    }

    worker_cur = workers_total;
    
    #else

    pthread_mutex_lock(&thread_pool->tpool_lock);

    for (; worker_cur < thread_pool->worker_cnt; worker_cur++)
    {
        pthread_t thread_worker_id = thread_pool->worker_threads[worker_cur].worker_sched;

        thread_pool->worker_cnt--;

        /* Ensure that the thread as been canceled */
        pthread_cancel(thread_worker_id);
    }

    pthread_mutex_unlock(&thread_pool->tpool_lock);

    #endif

    return workers_total - worker_cur;
}

bool tpool_finalize(tpool_t* thread_pool)
{
    assert(thread_pool->thread_pool_run == 0);

    /* All the mutexes must be destroyed in locked staged for avoid
     * undefined behaviour!
    */
    
    #if TPOOL_USES_DETACHED
    
    thread_pool->pool_begin_destroyed = 1;
    
    #endif

    int cancel_ret = tpool_cancel(thread_pool);

    /* This must be equal to 1, because shouldn't exist any workers alive */
    assert(cancel_ret == tpool_workers(thread_pool));

    pthread_mutex_destroy(&thread_pool->tpool_lock);

    pthread_mutex_destroy(&thread_pool->workers_lock);

    pthread_cond_destroy(&thread_pool->tpool_sync_tasks);

    free((void*)thread_pool->worker_threads);

    queue_destroy(thread_pool->task_queue_safe);

    return true;
}

/* Checks and wait until a worker is done for proccess our task
 * this function should returns the count of workers available in the idle state
 * and the thread_pool lock must be locked after and before this function call
*/
int tpool_wait_ava(tpool_t* thread_pool)
{
    while (1)
    {
        pthread_mutex_lock(&thread_pool->tpool_lock);

        if (thread_pool->workers_running != thread_pool->worker_cnt)
        {
            pthread_mutex_unlock(&thread_pool->tpool_lock);
            return thread_pool->worker_cnt - thread_pool->workers_running;
        }

        pthread_mutex_unlock(&thread_pool->tpool_lock);

        cpu_sleep_nano(TPOOL_SYNC_NANO);
    }

    return 0;
}

static bool tpool_add(struct thread_task* task, tpool_t* thread_pool)
{
    pthread_mutex_lock(&thread_pool->tpool_lock);

    int enqueue_ret = queue_enqueue((void*)task, thread_pool->task_queue_safe);
    
    assert(enqueue_ret == 0);

    /* Wait until a worker is available for do our work */
    
    pthread_mutex_unlock(&thread_pool->tpool_lock);

    tpool_wait_ava(thread_pool);

    /* Notify all workers that there's a task to be done */

    pthread_mutex_lock(&thread_pool->workers_lock);

    pthread_cond_broadcast(&thread_pool->tpool_sync_tasks);

    pthread_mutex_unlock(&thread_pool->workers_lock);

    return true;
}

void* tpool_wait_for_result(function_task_t task_operation, void* task_data, tpool_t* thread_pool)
{
    if (thread_pool->thread_pool_run == 0)
    {
        return NULL;
    }

    struct thread_task new_task;

    new_task.task_function = task_operation;
    new_task.task_data = task_data;
    
    pthread_cond_init(&new_task.task_condition, NULL);

    new_task.task_in_wait = 1;
    
    tpool_add(&new_task, thread_pool);
    
    /* Wait until some worker finished our task */
    pthread_mutex_lock(&thread_pool->workers_lock);

    pthread_cond_wait(&new_task.task_condition, &thread_pool->workers_lock);
    
    pthread_mutex_unlock(&thread_pool->workers_lock);

    pthread_cond_destroy(&new_task.task_condition);

    void* result_data = new_task.task_result;

    return result_data;
}

bool tpool_execute(function_task_t task_operation, void* task_data, tpool_t* thread_pool)
{
    if (thread_pool->thread_pool_run == 0)
    {
        return false;
    }

    struct thread_task* new_task = calloc(1, sizeof(struct thread_task));

    if (new_task == NULL) {}

    new_task->task_function = task_operation;
    new_task->task_data = task_data;

    bool add_red = tpool_add(new_task, thread_pool);

    /* The task will be freed inside the worker code */

    return add_red;
}

/* Wait for all tasks begin finished */
bool tpool_sync(tpool_t* thread_pool)
{
    while (1)
    {
        pthread_mutex_lock(&thread_pool->tpool_lock);
        if (queue_empty(thread_pool->task_queue_safe) && thread_pool->workers_running == 0)
        {
            pthread_mutex_unlock(&thread_pool->tpool_lock);
            break;
        }

        pthread_mutex_unlock(&thread_pool->tpool_lock);

        cpu_sleep_nano(TPOOL_SYNC_NANO);
    }

    for (size_t worker_cur = 0; worker_cur < thread_pool->worker_cnt; worker_cur++)
    {
        /* Wait until we can cancel the thread */
        while (thread_pool->worker_threads[worker_cur].can_cancel == 0) {}
        /* At this moment, the thead is waiting until the next task begins pushed */
    }

    return 0;
}

