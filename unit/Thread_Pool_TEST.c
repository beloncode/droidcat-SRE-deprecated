#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "Thread_Pool.h"

#define WORKERS_COUNT 2

#define EXPECTED_X_VALUE 50

#define RANDOM_POINTER_VALUE 0x10000

int x_sync_value = 0;

pthread_mutex_t inc_mutex = PTHREAD_MUTEX_INITIALIZER;

void* thread_inc_x(void* data)
{
    pthread_mutex_lock(&inc_mutex);
    (void)data;
    printf("[%ld] Thread in use\n", pthread_self());
    printf("The thread %ld is incrementing x... - x old value %d\n", pthread_self(), x_sync_value);
    /* This thread must be synchronous with who invoke him */
    x_sync_value += 1;
    pthread_mutex_unlock(&inc_mutex);
    return (void*)RANDOM_POINTER_VALUE + x_sync_value;
}

int main()
{
    /* Inside the unit test, we decide to create the pool at the stack
     * this must be discouraged because the thread pool needs to be independent 
     * from the main thread!
    */

    tpool_t stack_pool;

    tpool_init(WORKERS_COUNT, &stack_pool);
    
    //void* result_value = NULL;

    for (int index_cur = 0; index_cur != 50; index_cur+=5)
    {
        tpool_wait_for_result(thread_inc_x, NULL, &stack_pool);
        tpool_execute(thread_inc_x, NULL, &stack_pool);
        tpool_execute(thread_inc_x, NULL, &stack_pool);
        tpool_wait_for_result(thread_inc_x, NULL, &stack_pool);
        tpool_execute(thread_inc_x, NULL, &stack_pool);
    }

    //result_value = (void*)x_sync_value;

    tpool_stop(&stack_pool);

    printf("X final value %d - expected value %d\n", x_sync_value, EXPECTED_X_VALUE);

    //assert(result_value == (void*)RANDOM_POINTER_VALUE + EXPECTED_X_VALUE);

    tpool_finalize(&stack_pool);

    return 0;
}

