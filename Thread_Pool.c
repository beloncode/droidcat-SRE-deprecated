#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "Thread_Pool.h"
#include "cpu/CPU_Time.h"

#define TPOOL_SYNC_NANO 5e+6 /* 5 miliseconds */

struct thread_task
{
    void* task_data;

    void* task_result;

    _Atomic uint_least8_t task_completed;

    _Atomic uint_least8_t task_in_wait;

    pthread_mutex_t task_mutex;
    
    pthread_cond_t task_finished;

    function_task_t task_function;
};

static void tpool_task_init(function_task_t task_operation, void* task_data, struct thread_task* task)
{
    task->task_function = task_operation;
    task->task_data = task_data;

    pthread_mutex_init(&task->task_mutex, NULL);
    pthread_cond_init(&task->task_finished , NULL);
}

static void tpool_task_deinit(struct thread_task* task)
{
    pthread_mutex_destroy(&task->task_mutex);
    pthread_cond_destroy(&task->task_finished);
    memset(task, 0, sizeof(*task));
}

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

static const struct timespec __wait_time_limit = { .tv_nsec = TPOOL_SYNC_NANO, .tv_sec = 0 };

static void __force_worker_execution(tpool_t* thread_pool)
{
    pthread_mutex_trylock(&thread_pool->workers_lock);
    pthread_cond_broadcast(&thread_pool->tpool_sync_tasks);
    pthread_mutex_unlock(&thread_pool->workers_lock);
}

static void* tpool_worker_routine(void* tpool)
{
    tpool_t* thread_pool = (tpool_t*)tpool;
    
    worker_thread_t* worker_content = tpool_retrieve_self(thread_pool);

    while (1)
    {
        worker_content->can_cancel = 1;
        
        pthread_mutex_lock(&thread_pool->tpool_lock);
        #if TPOOL_USES_DETACHED
        if (thread_pool->pool_begin_destroyed)
        {
            thread_pool->worker_cnt--;
            pthread_mutex_unlock(&thread_pool->tpool_lock);
            pthread_exit(NULL);
        }
        else
        {
            thread_pool->workers_in_waiting++;
        }
        #else
        thread_pool->workers_in_waiting++;
        #endif
        pthread_mutex_unlock(&thread_pool->tpool_lock);

        pthread_mutex_lock(&thread_pool->workers_lock);
        /* Worker thread is waiting for a broadcast signal */
        pthread_cond_timedwait(&thread_pool->tpool_sync_tasks, &thread_pool->workers_lock, &__wait_time_limit);
        /* We're received the signal */
        pthread_mutex_unlock(&thread_pool->workers_lock);

        pthread_mutex_lock(&thread_pool->tpool_lock);
        thread_pool->workers_in_waiting--;
        thread_pool->workers_running++;
        pthread_mutex_unlock(&thread_pool->tpool_lock);

        struct thread_task* acquired_task = queue_dequeue(thread_pool->task_queue_safe);

        /* From this stage, the worker thread can't be canceled */
        worker_content->can_cancel = 0;

        if (acquired_task == NULL) continue;

        /* The task mutex must be locked */
        void* acquired_result = acquired_task->task_function(acquired_task->task_data);
        
        /* This copy is node here, because after unlock, the task may be destroyed if him reside on the stack,
         * the ownership will be transferred to the thread how created this task!
        */
        bool will_wait = acquired_task->task_in_wait;

        if (will_wait)
        {
            acquired_task->task_result = acquired_result;
            acquired_task->task_completed = 1;

            assert(pthread_mutex_lock(&acquired_task->task_mutex) == 0);
            pthread_cond_signal(&acquired_task->task_finished);
            pthread_mutex_unlock(&acquired_task->task_mutex);
        }
        else
        {
            tpool_task_deinit(acquired_task);
            free((void*)acquired_task);
        }

        /* Worker routine has finished the actual task, waiting for another */
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
    if (thread_pool->worker_threads == NULL) {}

    worker_thread_t* worker_cur = thread_pool->worker_threads;

    for (uint8_t worker_index = 0; worker_index != worker_count; worker_index++)
    {
        worker_cur[worker_index].worker_id = worker_index;
        
        worker_cur[worker_index].can_cancel = 0;

        pthread_t* thread_posix = &worker_cur[worker_index].worker_sched;
        
        int posix_result = pthread_create(thread_posix, NULL, tpool_worker_routine, (void*)thread_pool);

        if (posix_result != 0) {}
        #if TPOOL_USES_DETACHED
        /* Detach the thread when his has done */
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

size_t tpool_running_now(const tpool_t* thread_pool)
{
    return thread_pool->workers_running;
}

/* Returns the number of canceled worker threads */
static int tpool_cancel(tpool_t* thread_pool)
{
    #if TPOOL_USES_DETACHED

    while (tpool_workers(thread_pool) != 0)
    {
        __force_worker_execution(thread_pool);
    }

    #endif

    int workers_total = tpool_workers(thread_pool);
    int worker_cur;

    pthread_mutex_lock(&thread_pool->tpool_lock);

    for (worker_cur = 0; worker_cur < thread_pool->worker_cnt; worker_cur++)
    {
        pthread_t thread_worker_id = thread_pool->worker_threads[worker_cur].worker_sched;
        thread_pool->worker_cnt--;
        /* Ensure that the thread has been canceled */
        pthread_cancel(thread_worker_id);
    }

    pthread_mutex_unlock(&thread_pool->tpool_lock);

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
    int waiting_var = 0;

    while (waiting_var == 0)
    {
        pthread_mutex_lock(&thread_pool->workers_lock);
        
        pthread_mutex_lock(&thread_pool->tpool_lock);
        waiting_var = thread_pool->worker_cnt - thread_pool->workers_running;
        pthread_mutex_unlock(&thread_pool->tpool_lock);
        pthread_mutex_unlock(&thread_pool->workers_lock);
        /* Sleep until a task is ready of receive a broadcast signal */

        cpu_sleep_nano(TPOOL_SYNC_NANO);
    }

    return waiting_var;
}

static bool tpool_add(struct thread_task* task, tpool_t* thread_pool)
{
    assert(tpool_wait_ava(thread_pool) != 0);
    int enqueue_ret = queue_enqueue((void*)task, thread_pool->task_queue_safe);
    assert(enqueue_ret != false);

    return true;
}

bool tpool_evaluate(struct thread_task* task, tpool_t* thread_pool)
{
    /* Notify all workers that there's a task to be done */    
    /* It's know that there's one or more threads waiting for a task, so on we can notify they */
    /* The workers_lock must be acquired by tpool_wait_ava */

    bool will_wait = task->task_in_wait;
    __force_worker_execution(thread_pool);

    if (will_wait != 0)
    {
        pthread_mutex_lock(&task->task_mutex);
        pthread_cond_wait(&task->task_finished, &task->task_mutex);
        pthread_mutex_unlock(&task->task_mutex);

        assert(task->task_completed != 0);
        /* After this point, we can delete mutex without any problems */
        tpool_task_deinit(task);
    }

    return true;
}

void* tpool_wait_for_result(function_task_t task_operation, void* task_data, tpool_t* thread_pool)
{
    if (thread_pool->thread_pool_run == 0)
    {
        return NULL;
    }

    struct thread_task new_task_stack = { .task_in_wait = 1 };
    tpool_task_init(task_operation, task_data, &new_task_stack);
    tpool_add(&new_task_stack, thread_pool);

    /* Wait until some worker finished our task */
    tpool_evaluate(&new_task_stack, thread_pool);

    void* result_data = new_task_stack.task_result;
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
    tpool_task_init(task_operation, task_data, new_task);

    bool add_red = tpool_add(new_task, thread_pool);
    tpool_evaluate(new_task, thread_pool);

    /* The task will be freed inside the worker code */

    return add_red;
}

/* Wait for all tasks begin finished */
bool tpool_sync(tpool_t* thread_pool)
{
    bool sync_done = false;

    while (sync_done == false)
    {
        pthread_mutex_lock(&thread_pool->tpool_lock);
        if (queue_empty(thread_pool->task_queue_safe))
        {
            sync_done = true;
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
    return sync_done;
}

