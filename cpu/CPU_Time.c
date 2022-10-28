#include <time.h>

#if _XOPEN_SOURCE >= 500

#include <unistd.h>

#endif

#include "CPU_Time.h"

int cpu_sleep_nano(size_t nanoseconds)
{
    #if _POSIX_C_SOURCE >= 199309L

    const struct timespec sleep_data = {
        .tv_nsec = (long)nanoseconds, .tv_sec = 0
    };

    nanosleep(&sleep_data, NULL);

    #elif _XOPEN_SOURCE >= 500
    
    usleep(nanoseconds * 1e-6);

    #endif

    return 0;
}
