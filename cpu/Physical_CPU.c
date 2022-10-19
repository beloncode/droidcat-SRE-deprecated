#include <string.h>

#include "Hardware_Info.h"

int cpu_init(physical_CPU_t* physical_CPU)
{
    memset(physical_CPU, 0, sizeof(*physical_CPU));
    return 0;
}

/* Retrieves the number of existence cores in the host physical CPU */

int cpu_finalize(physical_CPU_t* physical_CPU)
{
    return 0;
}

