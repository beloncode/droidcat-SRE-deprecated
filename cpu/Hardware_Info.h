
#ifndef CPU_HARDWARE_INFO_H
#define CPU_HARDWARE_INFO_H

typedef struct physical_CPU
{
    char *vendor_name;

} physical_CPU_t;

int cpu_init(physical_CPU_t* physical_CPU);

/* Retrieves the number of existence cores in the host physical CPU */
int cpu_sched_cores(const physical_CPU_t* physical_CPU);

int cpu_finalize(physical_CPU_t* physical_CPU);

#endif


