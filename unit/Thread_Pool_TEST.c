#include <stdio.h>
#include <assert.h>

#include "Thread_Pool.h"

#define WORKERS_COUNT 2

#define RANDOM_POINTER_VALUE 0x10000

int x = 0;

void* thread_inc_x(void* data)
{
    (void)data;
    printf("The thread %ld is incrementing x... - x old value %d\n", pthread_self(), x);
    /* This thread must be synchronous with who invoke him */
    x += 1;
    return (void*)RANDOM_POINTER_VALUE;
}

int main()
{
    /* Inside the unit test, we decide to create the pool at the stack
     * this must be discouraged because the thread pool needs to be independent 
     * from the main thread!
    */
    tpool_t stack_pool;

    tpool_init(WORKERS_COUNT, &stack_pool);

    void* result_value = tpool_wait_for_result(thread_inc_x, NULL, &stack_pool);

    tpool_stop(&stack_pool);

    assert(result_value == (void*)RANDOM_POINTER_VALUE);

    tpool_finalize(&stack_pool);

    return 0;
}

