#include <stdio.h>
#include <malloc.h>

#include "Core_Context.h"

int main()
{
    droidcat_ctx_t* droidcat_main = (droidcat_ctx_t*) calloc(1, sizeof(droidcat_ctx_t));

    if (droidcat_main == NULL) {}

    droidcat_main->main_thread_pool = (tpool_t*) calloc(1, sizeof(tpool_t));
    droidcat_main->main_CPU = (physical_CPU_t*) calloc(1, sizeof(physical_CPU_t));

    tpool_t* main_pool = droidcat_main->main_thread_pool;
    physical_CPU_t* main_CPU = droidcat_main->main_CPU;

    cpu_init(main_CPU);

    tpool_init(4, main_pool);

    /* Stopping the threads pool service 
     * TODO: Creates the tpool_resume function 
    */
    tpool_stop(main_pool);

    tpool_finalize(main_pool);

    cpu_finalize(main_CPU);

    free((void*)droidcat_main->main_thread_pool);
    free((void*)droidcat_main->main_CPU);

    droidcat_main->main_thread_pool = NULL;
    droidcat_main->main_CPU = NULL;

    free((void*)droidcat_main);

    return 0;
}
