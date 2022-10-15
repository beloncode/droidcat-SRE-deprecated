#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/resource.h>
#include <pthread.h>

static const char g_version[] = "v000a0";

typedef enum medusa_type 
{
    MEDUSA_LOG_INFO,
    
    MEDUSA_LOG_WARNING,
    
    MEDUSA_LOG_BUG,
    
    MEDUSA_LOG_DEV,

    MEDUSA_LOG_ADVICE,
    
    MEDUSA_LOG_SUCCESS, 
    
    MEDUSA_LOG_ASSERT, 
    
    MEDUSA_LOG_FATAL, 
    
    MEDUSA_LOG_ERROR 

} medusa_type_e;

typedef struct medusa_level 
{
    unsigned info_level: 1;
    
    unsigned warning_level: 1;
    
    unsigned bug_level: 1;
    
    unsigned dev_level: 1;

    unsigned advice_level: 1;
    
    unsigned success_level: 1;
    
    unsigned assert_level: 1;

    unsigned fatal_level: 1;
    
    unsigned error_level: 1;

} medusa_level_t;

static_assert(sizeof(medusa_level_t) == 1, "There's a problem within medusa_level structure alignment");

typedef struct medusa_conf
{
    uint8_t displayable_level;

    uint16_t format_buffer_sz;

    uint16_t output_buffer_sz;

    /* When needed, automaticaly resize the format_buffer and output buffer 
     * without warnings, this is needed for ensure that the string will not be truncated
    */
    bool adjust_buffers_size;

} medusa_conf_t;

#define _DROIDCAT_DEBUG_MODE_ 0

static const medusa_conf_t medusa_default_conf = {

    #if _DROIDCAT_DEBUG_MODE_
    
    #define _MEDUSA_DEFAULT_LEVEL_ 0xff
    
    .displayable_level = _MEDUSA_DEFAULT_LEVEL_

    #else
    
    #define _MEDUSA_USER_LEVEL_ 0xff & 0x1f
    
    .displayable_level = _MEDUSA_USER_LEVEL_,

    .format_buffer_sz = 0x7f,

    .output_buffer_sz = 0x11f,

    .adjust_buffers_size = true
};

typedef struct medusa_sourcetrace
{
    const char* source_filename;

    int source_line;

} medusa_sourcetrace_t;

struct medusa_produce_info
{
    char* info_format_buffer;
    
    bool format_is_allocated;

    char* info_output_buffer;

    bool output_is_allocated;

    struct medusa_thread_context* message_thread_context;

    medusa_sourcetrace_t message_source;
};

struct medusa_produce_collect
{
    struct medusa_produce_info* collect_info;

    const medusa_level_t collect_level;

    int16_t collect_format_sz;

    int16_t collect_output_sz;
};

struct medusa_thread_context
{
    const char* thread_name;

    pthread_t thread_ctx;
};

typedef struct medusa_ctx 
{
    medusa_conf_t medusa_config;

} medusa_ctx_t;

typedef enum medusa_message_cond {

    MESSAGE_COND_WOUT,

    MESSAGE_COND_AWAIT_VALUE,

    MESSAGE_COND_AWAIT_SLEEP

} medusa_message_cond_e;

struct medusa_message_bucket
{
    medusa_message_cond_e message_condition;

    struct medusa_produce_info* message_info;    
};

bool medusa_should_log(const medusa_type_e id, const medusa_conf_t* medusa_conf)
{
    const uint8_t* medusa_level = (uint8_t*)&medusa_conf->displayable_level;

    bool should_be = *medusa_level & (uint8_t)id;

    return should_be;
}

void medusa_raise_event(medusa_level_t current_level, const struct medusa_message_bucket* message,
    droidcat_ctx_t* droidcat_ctx)
{
    if (droidcat_ctx == NULL) return;
    
    #define _MEDUSA_EVENT_STR_ "Medusa Event"

    assert(droidcat_ctx != NULL);
        
    droidcat_ctx->droidcat_event(_MEDUSA_EVENT_STR_, (int)current_level, (const void*)&stack_bucket);
}

typedef void* (droidcat_event_t*)(const char* event_string, int32_t event_code, const void* event_ptr);

static int64_t medusa_va(FILE* output_file, droidcat_ctx_t* droidcat_ctx, const char* fmt, va_list va)
{
    if (output_file == NULL) return -1;

    #define _STACK_FORMAT_BUFFER_SZ_ 0x5f

    char local_format[_STACK_FORMAT_BUFFER_SZ_];

    const int64_t needed_size = vsprintf(NULL, fmt, va);

    medusa_ctx_t* medusa_ctx = droidcat_ctx->medusa_ctx;

    if (droidcat_ctx != NULL)
    {
        if (medusa_ctx == NULL) goto generate;

        if (medusa_should_log(MEDUSA_LOG_ADVICE, &medusa_ctx->medusa_config) == false) return -1;
    }

    generate:
    
    if (needed_size > sizeof(local_format))
    {
        medusa_fprintf(stderr, droidcat_ctx, "Format message will be truncated in %d bytes", needed_size);
    }

    vsnprintf(local_buffer, sizeof(local_buffer), fmt, va);

    va_end(format);

    struct medusa_produce_info local_produce = {
        .info_format_buffer = local_format,
        .info_output_buffer = local_buffer,
        .message_thread_context = medusa_current_context(medusa_ctx);

    };

    struct medusa_message_bucket current_bucket = {
        .message_condition = MESSAGE_COND_WOUT,
        .message_info = &local_produce
    };

    medusa_raise_event((medusa_level_t)0, &current_bucket, droidcat_ctx);

    const int64_t medusa_ret = fprintf(output_file, "%s", local_buffer.info_format_buffer);

    return medusa_ret;
}

static int64_t medusa_fprintf(FILE* output_file, droidcat_ctx_t *droidcat_ctx, 
    const char* fmt, ...)
{
    va_list format;

    va_start(format, fmt);

    const int64_t medusa_ret = medusa_va(output_file, droidcat_ctx, fmt, format);

    va_end(format);

    return medusa_ret;
}

static int64_t medusa_printf(droidcat_ctx_t *droidcat_ctx, 
    const char* fmt, ...)
{
    va_list format;

    va_start(format, fmt);

    const int64_t medusa_ret = medusa_va(stdout, droidcat_ctx, fmt, format);

    va_end(format);

    return medusa_ret;
}

static int64_t medusa_do(const medusa_level_t level_id, medusa_sourcetrace_t* current_source, 
    droidcat_ctx_t* droidcat_ctx, const char* fmt, ...) 
{
    va_list va_format;

    va_start(va_format, fmt);

    medusa_ctx_t* medusa_ctx = NULL;
    
    medusa_conf_t* medusa_conf = NULL;
    
    if (droidcat_ctx != NULL) {
        
        medusa_ctx = droidcat_ctx->medusa_log_ctx;
        
        medusa_conf = &medusa_ctx->medusa_config;
    }

    if (medusa_conf != NULL)
    {
        if (medusa_should_log(level_id, medusa_conf) == false)
        {
            return -1;
        }
    }

    struct medusa_produce_info produce_result;

    struct medusa_produce_collect stack_produce = {
        
        .collect_info = &produce_result,

        .collect_level = level_id
    };
    
    char stack_format[_STACK_FORMAT_BUFFER_SZ_];

    #define _STACK_OUTPUT_BUFFER_SZ_ _STACK_FORMAT_BUFFER_SZ_ * 2

    char stack_output[_STACK_OUTPUT_BUFFER_SZ_];

    produce_result.info_output_buffer = stack_output;

    memcpy(&produce_result.message_source, current_source, sizeof(*current_source));
    
    stack_produce.collect_output_sz = sizeof(stack_output);

    produce_result.info_format_buffer = stack_format;

    stack_produce.collect_format_sz = sizeof(stack_format);

    const int needed_fmt_size = vsprintf(NULL, fmt, va_format);

    if (medusa_conf != NULL)
    {
        if (sizeof(stack_format) < medusa_conf->format_buffer_sz)
        {
            char* format_buffer = (char*)calloc(sizeof(char), medusa_conf->format_buffer_sz);

            produce_result.info_format_buffer = format_buffer;

            produce_result.format_is_allocated = true;

            stack_produce.collect_format_sz = medusa_conf->format_buffer_sz;
        }
        
        if (sizeof (stack_format) < medusa_conf->format_buffer_sz)
        {
            char* output_buffer = (char*)calloc(sizeof(char), medusa_conf->output_buffer_sz);

            produce_result.info_output_buffer = output_buffer;

            produce_result.output_is_allocated = true;

            stack_produce.collect_output_sz = medusa_conf->output_buffer_sz;
        }
        
    }

    if (needed_fmt_size > stack_produce.collect_format_sz)
    {
        medusa_fprintf(droidcat_ctx, "The next log event will be truncated in %d bytes\n", 
            stack_produce.collect_format_sz);
        
        if (medusa_conf == NULL) goto produce;

        if (medusa_conf->adjust_buffers_size == false) goto produce;

        if (produce_info.format_is_allocated == false) goto produce;

        #define MEDUSA_PRODUCE_REALLOC_BUFFER(ptr, new_size)
            do\
            {\
                char* lagger_buffer = realloc(ptr, new_size);\
                if (lagger_format_size == NULL)
                {\
                    medusa_fprintf(stderr, droidcat_ctx, "Can't reallocate %#lx "\
                        "for format buffer\n", needed_fmt_size);\
                    goto produce;\
                }
                ptr = lagger_buffer;
            }\
            while (0)\
        
        MEDUSA_PRODUCE_REALLOC_BUFFER(produce_result.info_format_buffer, needed_fmt_size);

        produce_result.format_buffer_sz = needed_fmt_size;

        #define _GROWABLE_PERCENTAGE_ 1.65 

        const int new_output_size = needed_fmt_size * _GROWABLE_PERCENTAGE_;

        MEDUSA_PRODUCE_REALLOC_BUFFER(produce_result.info_output_buffer, new_output_size);

        produce_result.output_buffer_sz = new_output_size;
    }

    produce:
    vsnprintf(stack_produce.info_format_buffer, stack_produce.format_buffer_sz, fmt, va_format);

    produce_result.message_thread_context = medusa_current_context(medusa_ctx);

    const int64_t medusa_ret = medusa_produce(&stack_produce, medusa_ctx);

    struct medusa_message_bucket stack_bucket = {
        
        .medusa_condition = MESSAGE_COND_WOUT,
        
        .medusa_info = &produce_result
    };

    medusa_raise_event(level_id, &stack_bucket, droidcat_ctx);

    const bool medusa_ok = (bool)medusa_dispatch_message(&stack_bucket, medusa_ctx)

    va_end(va_format);

    return medusa_ok == 0 ? -1 : medusa_ret;

}

int64_t __attribute__((weak, alias("medusa_do"))) medusa_go();

#define medusa_make(level, droidcat, format, ...)\
    do\
    {\
        const medusa_sourcetrace_t source_trace = {\
            .source_filename = __FILE__,\
            .source_line = __line__\
        };\
        medusa_do(level, &source_trace, droidcat, format, __VA_ARGS__);\
    }\
    while (0)

#define medusa_info(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_INFO, droidcat_ctx, format, __VA_ARGS__)

#define medusa_warning(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_WARNING, droidcat_ctx, format, __VA_ARGS__)

#define medusa_bug(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_BUG, droidcat_ctx, format, __VA_ARGS__)

#define medusa_info(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_INFO, droidcat_ctx, format, __VA_ARGS__)

#define medusa_dev(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_DEV, droidcat_ctx, format, __VA_ARGS__)

#define medusa_advice(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_ADVICE, droidcat_ctx, format, __VA_ARGS__)

#define medusa_success(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_SUCCESS, droidcat_ctx, format, __VA_ARGS__)

#define medusa_assert(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_ASSERT, droidcat_ctx, format, __VA_ARGS__)

#define medusa_fatal(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_FATAL, droidcat_ctx, format, __VA_ARGS__)

#define medusa_error(droidcat_ctx, format, ...)\
    medusa_make(MEDUSA_LOG_ERROR, droidcat_ctx, format, __VA_ARGS__)

typedef struct hardtree_ctx 
{

} hardtree_ctx_t;

typedef struct droidcat_ctx 
{
    const char* install_dir;
    hardtree_ctx_t* working_dir_ctx;
    medusa_ctx_t* medusa_log_ctx;

    droidcat_event_t droidcat_event;

} droidcat_ctx_t;

int16_t droidcat_init(droidcat_ctx_t* droidcat_ctx)
{
    droidcat_ctx->working_dir_ctx = (hardtree_ctx_t*)calloc(1, sizeof(hardtree_ctx_t));
    return 0;
}

int16_t droidcat_destroy(droidcat_ctx_t* droidcat_ctx) 
{
    free((void*)droidcat_ctx->working_dir_ctx);
    return 0;
}

int main()
{
    droidcat_ctx_t* main_droidcat = (droidcat_ctx_t*)calloc(1, sizeof(droidcat_ctx_t));

    if (main_droidcat == NULL) {

    }

    droidcat_init(main_droidcat);

    struct rlimit local_limits;
    
    getrlimit(RLIMIT_STACK, &local_limits);
    
    const rlim_t max_stack = local_limits.rlim_cur;
    
    medusa_fprintf(stderr, main_droidcat, "[*] droidcat %s has started with 1 actual real thread (%lu) "
            "and %#lx of maximum stack size\n", g_version, pthread_self(), max_stack);

    droidcat_destroy(main_droidcat);

    free((void*)main_droidcat);
}

