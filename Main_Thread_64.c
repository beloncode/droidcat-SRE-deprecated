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

} __attribute__((aligned(4))) medusa_level_t;

static_assert(sizeof(medusa_level_t) == 4, "There's a problem within medusa_level structure alignment");

typedef struct medusa_conf
{
    uint8_t displayable_level;

    uint16_t format_buffer_sz;

    uint16_t output_buffer_sz;

    /* When needed, automaticaly resize the format_buffer and output buffer 
     * without warnings, this is needed for ensure that the string will not be truncated
    */
    bool adjust_buffers_size;

    const char* message_format;

    bool use_format;

    const char* program_name;

    bool display_program_name;

    bool display_status;

    #if defined(_WIN64)

    #else defined(__unix__)

    int default_output_fd;

    #endif

    bool display_to_default_output;

} medusa_conf_t;

char* string_generate_format(char* dest_output, const char* to_format)
{
    while (*to_format != '\0')
    {
        if (isalpha(*to_format))
        {
            if (!isalpha(*to_format))
                dest_output++ = '%c';
            else
                dest_output++ = '%s';
            while (isalpha(to_format)) to_format++;
        }

        dest_output++ = *to_format; 

    }
    
    return dest_output;
}

int string_sanitize_wout(const char* destination_str, const char** wout_string)
{
    int wout_position;
    
    for (wout_position = 0; wout_string[wout_position] != NULL, wout_position++)
    {
        if (strstr(destination_str, wout_string) != NULL) break;
    }

    return wout_position;
}

#define _DROIDCAT_DEBUG_MODE_ 0

static const medusa_conf_t medusa_default_conf = {

    #if _DROIDCAT_DEBUG_MODE_
    
    #define _MEDUSA_DEFAULT_LEVEL_ 0xff
    
    .displayable_level = _MEDUSA_DEFAULT_LEVEL_

    #else
    
    #define _MEDUSA_USER_LEVEL_ 0xff & 0x1f
    
    .displayable_level = _MEDUSA_USER_LEVEL_,

    #endif

    .format_buffer_sz = 0x7f,

    .output_buffer_sz = 0x11f,

    .adjust_buffers_size = true,

    .default_output_fd = fileno(stdout),

    .display_to_default_output = true,
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

    const char* status_string;

    const char* color_string;
};

struct medusa_produce_collect
{
    struct medusa_produce_info* collect_info;

    const medusa_type_e collect_level;

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

    pthread_mutex_t medusa_central_mutex;

    bool medusa_is_running;

    #define _FORMAT_BUFFER_SZ_

    char* medusa_format;
    
    #define _OUTPUT_BUFFER_SZ_ _FORMAT_BUFFER_SZ_ * 2

    char* medusa_output;

} medusa_ctx_t;

typedef enum medusa_message_cond {

    MESSAGE_COND_WOUT,

    MESSAGE_COND_AWAIT_VALUE,

    MESSAGE_COND_AWAIT_SLEEP

} medusa_message_cond_e;

typedef struct
{
    medusa_message_cond_e message_condition;

    _Atomic int_least8_t condition_value;

    struct timespec condition_time;

} medusa_cond_t;

struct medusa_message_bucket
{
    medusa_cond_t message_condition;
    struct medusa_produce_info* message_info;    
};

bool medusa_should_log(const medusa_type_e id, medusa_ctx_t* medusa_ctx)
{
    pthread_mutex_lock(&medusa_ctx->medusa_central_mutex);

    medusa_conf_t* medusa_config = &medusa_ctx->medusa_config;

    const uint8_t* medusa_level = (uint8_t*)&medusa_config->displayable_level;

    bool should_be = *medusa_level & (uint8_t)id;

    pthread_mutex_unlock(&medusa_ctx->medusa_central_mutex);

    return should_be;
}

typedef struct hardtree_ctx 
{

} hardtree_ctx_t;

typedef void* (*droidcat_event_t)(const char* event_string, int32_t event_code, const void* event_ptr);

typedef struct droidcat_ctx 
{
    const char* install_dir;
    
    hardtree_ctx_t* working_dir_ctx;

    medusa_ctx_t* medusa_log_ctx;

    droidcat_event_t droidcat_event;

} droidcat_ctx_t;

void medusa_raise_event(medusa_type_e current_level, const struct medusa_message_bucket* message,
    droidcat_ctx_t* droidcat_ctx)
{
    if (droidcat_ctx == NULL) return;
    
    #define _MEDUSA_EVENT_STR_ "Medusa Event"

    assert(droidcat_ctx != NULL);
    
    droidcat_ctx->droidcat_event(_MEDUSA_EVENT_STR_, (int)current_level, (void*)message);
}

static int64_t medusa_fprintf(FILE* output_file, droidcat_ctx_t *droidcat_ctx, const char* fmt, ...);

static int64_t medusa_va(FILE* output_file, droidcat_ctx_t* droidcat_ctx, const char* fmt, va_list va)
{
    if (output_file == NULL) return -1;

    #define _STACK_FORMAT_BUFFER_SZ_ 0x5f

    char local_format[_STACK_FORMAT_BUFFER_SZ_];

    const int64_t needed_size = vsprintf(NULL, fmt, va);

    medusa_ctx_t* medusa_ctx = droidcat_ctx->medusa_log_ctx;

    if (droidcat_ctx != NULL)
    {
        if (medusa_ctx == NULL) goto generate;

        if (medusa_should_log(MEDUSA_LOG_ADVICE, medusa_ctx) == false) return -1;
    }

    generate:
    
    if (needed_size > sizeof(local_format))
    {
        medusa_fprintf(stderr, droidcat_ctx, "Format message will be truncated in %d bytes", needed_size);
    }

    vsnprintf(local_format, sizeof(local_format), fmt, va);

    struct medusa_produce_info local_produce = {
        .info_format_buffer = local_format,
        .message_thread_context = medusa_current_context(medusa_ctx);

    };

    struct medusa_message_bucket current_bucket = {
        .message_info = &local_produce
    };

    medusa_raise_event((medusa_type_e)0, &current_bucket, droidcat_ctx);

    const int64_t medusa_ret = fprintf(output_file, "%s", local_produce.info_format_buffer);

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

typedef bool (*string_expand_t)(const char* item_format, char** write_output, void* user_data);

struct medusa_generate_info
{
    medusa_conf_t* gen_conf;

    struct medusa_produce_info* gen_info;

    struct medusa_thread_context* gen_context;

};

static bool medusa_generate(const char* item_format, char** write_output, void* user_data)
{
    struct medusa_generate_info* generate_info = (struct medusa_generate_info*)user_data;

    medusa_conf_t* medusa_conf = generate_info->gen_conf;
    
    struct medusa_produce_info* produce_info = generate_info->gen_info;

    struct medusa_thread_context* produce_context = generate_info->gen_context;

    #define MIN_STRLEN(x, y)\
    do\
    {\
        int64_t __x_len = strlen(x);\
        int64_t __y_len = strlen(y);\
        if (__x_len > __y_len)\
            return __y_len;\
        return __x_len;\
    }\
    while(0)

    #define CHECK_STR_IS(str, compare_with)\
        if (strncmp(str, compare_with, MIN_STRLEN(str, compare_with)) == 0)

    #define _FORMAT_MESSAGE_STR_ "FORMAT_MESSAGE"

    CHECK_STR_IS(item_format, _FORMAT_MESSAGE_STR_)
    {
        *write_output = generate_info->info_format_buffer;

        goto generate_ok;
    }

    #define _STATUS_STR_ "STATUS"

    CHECK_STR_IS(item_format, _STATUS_STR_)
    {
        if (medusa_conf->display_status == false) goto generate_err;

        *write_output = produce_info->status_string;

        goto generate_ok;
    }

    #define _TIME_STR_ "TIME"

    CHECK_STR_IS(item_format, _TIME_STR_)
    {
        *write_output = NULL;

        goto generate_err;
    }

    #define BACKTRACE_STR_ "BACKTRACE"

    CHECK_STR_IS(item_format, BACKTRACE_STR_)
    {
        *write_output = NULL;

        goto generate_err;
    }

    #define _SOURCE_TRACE_STR_ "SOURCETRACE"

    CHECK_STR_IS(item_format, _SOURCE_TRACE_STR_)
    {
        *write_output = NULL;

        goto generate_err;
    }

    #define _CONTEXT_STR_ "CONTEXT"

    CHECK_STR_IS(item_format, _CONTEXT_STR_)
    {
        *write_output = NULL;

        goto generate_err;
    }

    #define _PROGRAM_NAME_STR_ "PROGRAM_NAME"

    CHECK_STR_IS(item_format, _PROGRAM_NAME_STR_)
    {
        if (medusa_config->display_program_name == false) goto generate_err;
        
        *write_output = medusa_config->program_name;
        
        goto generate_ok;
    }

    generate_ok:

    return true;

    generate_err:
    
    return false;
}

int32_t string_expand_format(char** format_values, size_t format_capacity, 
    const char** format_ctx_array, string_expand_t expand_call, void* user_data)
{    
    uintptr_t format_values_index = 0;

    #define PUSH_FORMAT_VALUE(value)\
        if (!(format_values_index >= format_capacity))\
            (format_values[format_values_index++] = value)

    char* format_value;

    for (format_value = *format_values; format_values != NULL; format_values++)
    {
        char* result_value = NULL;
        // (const char* item_format, char** write_output, void* user_data)
        const bool result_format = expand_call(format_value, &result_value, user_data);

        if (result_format == false)
            result_value = "(null)";

        PUSH_FORMAT_VALUE(result_format);        
    }

    return format_values_index;
}

static int64_t string_entries_count(char** string_array)
{
    int64_t actual_capacity;

    for (actual_capacity = 0; *string_array++ != NULL; actual_capacity++) ;

    return actual_capacity;
}

static char* string_populate(char* output_buffer, size_t output_size,
    string_format, format_values)
{
    return output_buffer;
}


static int64_t medusa_produce(struct medusa_produce_collect* produce_collect,
    medusa_ctx_t* medusa_ctx)
{
    const medusa_type_e message_type = produce_collect->collect_level;

    medusa_conf_t* medusa_conf = medusa_ctx->medusa_config;

    struct medusa_produce_info* produce_info = produce_collect->collect_info;

    struct medusa_thread_context* produce_context = produce_info->message_thread_context;

    char* output_buffer = produce_info->info_format_buffer;

    assert(output_buffer != NULL);

    const char* message_str = NULL;
    const char* color_scheme = NULL;

    switch (message_type)
    {
    case MEDUSA_LOG_INFO: message_str = "Info"; color_scheme = ""; break;
    
    case MEDUSA_LOG_WARNING: message_str = "Warning"; color_scheme = ""; break;
    
    case MEDUSA_LOG_BUG: message_str = "Bug"; color_scheme = ""; break;
    
    case MEDUSA_LOG_DEV: message_str = "Dev"; color_scheme = ""; break;
    
    case MEDUSA_LOG_ADVICE: message_str = "Advice"; color_scheme = ""; break;
    
    case MEDUSA_LOG_SUCCESS: message_str = "Success"; color_scheme = ""; break;
    
    case MEDUSA_LOG_ASSERT: message_str = "Assert"; color_scheme = ""; break;
    
    case MEDUSA_LOG_FATAL: message_str = "Fatal"; color_scheme = ""; break;
    
    default: case MEDUSA_LOG_ERROR: message_str = "Error"; color_scheme = ""; break;
    }

    produce_info->status_string = message_str;

    produce_info->color_string = color_scheme;

    static const char* default_message_format = "(PROGRAM_NAME:CONTEXT).[SOURCETRACE:BACKTRACE]\tTIME:STATUS -> FORMAT_MESSAGE";

    const char* format_in_use = NULL;

    if (medusa_conf->use_format)
    {
        format_in_use = medusa_conf->message_format;
    }
    else
    {
        format_in_use = default_message_format;    
    }

    uintptr_t actual_cursor = 0;

    #define INC_CURSOR(inc)\
        (actual_cursor += inc)
    
    #define INC_LOCATION\
        (output_buffer + actual_cursor)
    
    #define REMAIN_SIZE\
        (produce_collect->collect_output_sz - actual_cursor)
    
    #define WRITE_BUFFER(fmt, ...)\
        INC_CURSOR(snprintf(INC_LOCATION, REMAIN_SIZE, fmt, __VA_ARGS__))

    WRITE_BUFFER("?");

    #define _OUTPUT_FINAL_FORMAT_ 0x6f

    char output_format[_OUTPUT_FINAL_FORMAT_];

    string_generate_format(output_format, format_in_use);

    static const char* invalid_formats[] = {
        "%d", "%u", "%ld", "%lu", "%x", "%c", NULL
    };
    if (string_sanitize_wout(output_format, invalid_formats) == false) {}

    #define _OUTPUT_FINAL_MAX_SEQUENCE_ 10

    char format_ctx_string[_OUTPUT_FINAL_MAX_SEQUENCE_ + 1][_OUTPUT_FINAL_FORMAT_ / 2];

    format_ctx_string[_OUTPUT_FINAL_MAX_SEQUENCE_] = NULL;

    string_strip_with_format_array(format_ctx_string, sizeof(format_ctx_string), _OUTPUT_FINAL_MAX_SEQUENCE_,
        output_format);

    char format_values[_OUTPUT_FINAL_MAX_SEQUENCE_];

    struct medusa_generate_info generate_info = {
        
        .gen_conf = medusa_conf,
        .gen_produce = produce_info,
        .gen_context = produce_context

    };

    const int32_t expand_result = string_expand_format(format_values, _OUTPUT_FINAL_MAX_SEQUENCE_ - 1, format_ctx_string, medusa_generate, &generate_info);

    int64_t populate_ret = string_populate(output_buffer, produce_collect->collect_output_sz output_format, format_values);

    assert(string_entries_count(format_values) == expand_result);

    return populate_ret;
}

int medusa_send(struct medusa_message_bucket* message_bucket, medusa_ctx_t* medusa_ctx)
{
    struct medusa_produce_info* info = message_bucket->message_info; 

    medusa_cond_t* condition = message_bucket->message_condition;

    int send_ret = 0;

    if (condition->message_condition != MESSAGE_COND_WOUT)
    {
        assert(info->output_is_allocated);
        assert(info->info_output_buffer != medusa_ctx->output_buffer);

    }

    if (medusa_ctx != NULL)
    {
        medusa_conf_t* conf = &medusa_ctx->medusa_conf;

        if (conf->display_to_default_output)
        {
            #if defined(_WIN64)

            #elif defined(__unix__)

            send_ret = (int)write(conf->default_output_fd, info->info_output_buffer, strlen(info->info_output_buffer));

            #endif
        }
    }

    if (info->info_output_buffer != medusa_ctx->output_buffer)
    {
        assert(info->info_format_buffer != medusa_ctx->format_buffer);

        free((void*)info->info_output_buffer);

        free((void*)info->info_format_buffer);

    }

    return send_ret;
}

int medusa_dispatch_message(struct medusa_message_bucket* message_bucket, medusa_ctx_t* medusa_ctx)
{
    medusa_cond_t condition = message_bucket->message_condition;

    struct medusa_produce_info* info = message_bucket->message_info; 

    if (condition->message_condition != MESSAGE_COND_WOUT)
    {
        assert(medusa_ctx != NULL);

        pthread_mutex_lock(&medusa_ctx->medusa_central_mutex);

        pthread_mutex_lock(&medusa_ctx->medusa_central_mutex);
    }
    else
    {
        medusa_send(message_bucket, medusa_ctx);
    }

    return 1;
}

static int64_t medusa_do(const medusa_type_e level_id, medusa_sourcetrace_t* current_source, 
    medusa_cond_t* current_cond, droidcat_ctx_t* droidcat_ctx, const char* fmt, ...) 
{
    va_list va_format;

    va_start(va_format, fmt);

    medusa_ctx_t* medusa_ctx = NULL;
    
    medusa_conf_t* medusa_conf = NULL;

    char medusa_format[_STACK_FORMAT_BUFFER_SZ_];

    char stack_output[_STACK_OUTPUT_BUFFER_SZ_];

    char* format_use = medusa_format;

    char* output_use = stack_output;

    if (droidcat_ctx != NULL) 
    {
        medusa_ctx = droidcat_ctx->medusa_log_ctx;
        
        medusa_conf = &medusa_ctx->medusa_config;

        format_use = medusa->medusa_format;

        output_use = medusa->medusa_output;
    }

    struct medusa_produce_info produce_result;

    struct medusa_produce_collect local_produce = {
        
        .collect_info = &produce_result,

        .collect_level = level_id
    }; 

    if (medusa_ctx != NULL)
    {
        local_produce.info_format_buffer = format_use;
        local_produce.info_output_buffer = output_use;

        if (medusa_should_log(level_id, medusa_ctx) == false)
        {
            return -1;
        }

        local_produce.collect_format_sz = medusa_conf->format_buffer_sz;
        local_produce.collect_output_sz = medusa_conf->format_output_sz;
    }
    else
    {
        local_produce.info_format_buffer = stack_format;
        local_produce.info_output_buffer = stack_output;

        local_produce.collect_format_sz = sizeof(stack_format);
        local_produce.collect_output_sz = sizeof(stack_output);
    }

    memcpy(&produce_result.message_source, current_source, sizeof(*current_source));

    const int needed_fmt_size = vsprintf(NULL, fmt, va_format);

    if (needed_fmt_size > stack_produce.collect_format_sz)
    {
        medusa_fprintf(stderr, droidcat_ctx, "The next log event will be truncated in %d bytes\n", 
            stack_produce.collect_format_sz);
        
        if (medusa_conf == NULL) goto produce;

        if (medusa_conf->adjust_buffers_size == false) goto produce;

        if (produce_result.format_is_allocated == false) goto produce;

        #define MEDUSA_PRODUCE_REALLOC(ptr, new_size)\
            do\
            {\
                char* lagger_buffer = realloc(ptr, new_size);\
                if (lagger_buffer == NULL)\
                {\
                    medusa_fprintf(stderr, droidcat_ctx, "Can't reallocate %#lx "\
                        "for format buffer\n", new_size);\
                    goto produce;\
                }\
                ptr = lagger_buffer;\
            }\
            while (0)\
        
        MEDUSA_PRODUCE_REALLOC(produce_result.info_format_buffer, needed_fmt_size);

        stack_produce.collect_format_sz = needed_fmt_size;

        #define _GROWABLE_PERCENTAGE_ 1.65 

        const int new_output_size = needed_fmt_size * _GROWABLE_PERCENTAGE_;

        MEDUSA_PRODUCE_REALLOC(produce_result.info_output_buffer, new_output_size);

        stack_produce.collect_output_sz = new_output_size;
    }

    produce:

    vsnprintf(produce_result.info_format_buffer, stack_produce.collect_format_sz, fmt, va_format);

    produce_result.message_thread_context = medusa_current_context(medusa_ctx);

    const int64_t medusa_ret = medusa_produce(&stack_produce, medusa_ctx);

    struct medusa_message_bucket stack_bucket = {

        .message_info = &produce_result
    
    };

    if (current_cond != NULL)
    {
        assert(medusa_ctx != NULL);

        memcpy(&stack_bucket.message_condition, stack_bucket, sizeof(*stack_bucket));
    }

    medusa_raise_event(level_id, &stack_bucket, droidcat_ctx);

    const bool medusa_ok = (bool)medusa_dispatch_message(&stack_bucket, medusa_ctx);

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
        medusa_go(level, &source_trace, NULL, droidcat, format, __VA_ARGS__);\
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

int16_t droidcat_init(droidcat_ctx_t* droidcat_ctx)
{
    droidcat_ctx->working_dir_ctx = (hardtree_ctx_t*)calloc(1, sizeof(hardtree_ctx_t));
    
    droidcat_ctx->medusa_log_ctx = (medusa_ctx_t*)calloc(1, sizeof(medusa_ctx_t));

    return 0;
}

int16_t droidcat_destroy(droidcat_ctx_t* droidcat_ctx) 
{
    free((void*)droidcat_ctx->working_dir_ctx);
    
    free((void*)droidcat_ctx->medusa_log_ctx);
    
    return 0;
}

void* main_event_log(const char* event_string, int32_t event_code, const void* event_ptr)
{
    return NULL;
}


int32_t medusa_activate(medusa_conf_t* medusa_conf, medusa_ctx_t* medusa_ctx)
{
    assert(medusa_ctx->medusa_is_running == false);

    if (medusa_conf == NULL)
    {
        memcpy(&medusa_ctx->medusa_config, medusa_conf, sizeof(*medusa_conf));
    }
    else
    {
        memcpy(&medusa_ctx->medusa_config, &medusa_default_conf, sizeof(medusa_default_conf));
    }

    medusa_conf->medusa_format = (char*)calloc(sizeof(char), medusa_conf->format_buffer_sz);

    medusa_conf->medusa_output = (char*)calloc(sizeof(char), medusa_conf->output_buffer_sz);
    
    pthread_mutex_init(&medusa_ctx->medusa_central_mutex, NULL);

    medusa_ctx->medusa_is_running = true;

    return 0;
}

/* The thread/method thats wait for medusa will acquire his lock */
int32_t medusa_wait(int time_out, medusa_ctx_t* medusa_ctx)
{
    assert(medusa_ctx->medusa_is_running == false);

    pthread_mutex_lock(&medusa_ctx->medusa_central_mutex);

    return 0;
}

int32_t medusa_deactivate(medusa_ctx_t* medusa_ctx)
{
    #define _MEDUSA_WAIT_TIME_OUT_ 100
    
    medusa_wait(_MEDUSA_WAIT_TIME_OUT_, medusa_ctx);

    medusa_ctx->medusa_is_running = false;

    free((void*)medusa_conf->medusa_format);

    free((void*)medusa_conf->medusa_output);

    pthread_mutex_unlock(&medusa_ctx->medusa_central_mutex);

    pthread_mutex_destroy(&medusa_ctx->medusa_central_mutex);

}

int32_t droidcat_session_start(droidcat_ctx_t* droidcat_ctx)
{
    droidcat_ctx->droidcat_event = main_event_log;

    medusa_ctx_t* medusa_ctx = droidcat_ctx->medusa_log_ctx;

    /* Uses default medusa configuration (all options at the configuration
     * level byt the program should be activate) 
    */
    medusa_activate(NULL, medusa_ctx);

    return 0;
}

int32_t droidcat_session_stop(droidcat_ctx_t* droidcat_ctx)
{
    medusa_ctx_t* medusa_ctx = droidcat_ctx->medusa_log_ctx;

    medusa_deactivate(medusa_ctx);

    return 0;
}

int main()
{
    droidcat_ctx_t* main_droidcat = (droidcat_ctx_t*)calloc(1, sizeof(droidcat_ctx_t));

    if (main_droidcat == NULL) 
    {
        medusa_fprintf(stderr, NULL, "Can't allocate droidcat context, malloc returns NULL\n");
        return -1;
    }

    const int64_t droid_start = droidcat_init(main_droidcat);

    if (droid_start != 0) {}

    droidcat_session_start(main_droidcat);

    struct rlimit local_limits;
    
    getrlimit(RLIMIT_STACK, &local_limits);
    
    const rlim_t max_stack = local_limits.rlim_cur;
    
    medusa_fprintf(stderr, main_droidcat, "[*] droidcat %s has started with 1 actual real thread (%lu) "
            "and %#lx of maximum stack size\n", g_version, pthread_self(), max_stack);
        
    droidcat_session_stop(main_droidcat);

    droidcat_destroy(main_droidcat);

    free((void*)main_droidcat);
}

