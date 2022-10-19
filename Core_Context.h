#ifndef CORE_CONTEXT_H
#define CORE_CONTEXT_H

#include "Thread_Pool.h"
#include "cpu/Hardware_Info.h"

typedef struct droidcat_ctx
{
    /* The main thread pool used during all droidcat process
     * alive time.
     * Some specific threads will have his affinity changes and will be used for an specific types of task!
    */
    tpool_t* main_thread_pool;

    physical_CPU_t* main_CPU;

} droidcat_ctx_t;

#endif
