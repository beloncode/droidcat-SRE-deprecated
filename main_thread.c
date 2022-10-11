#define _OPTION_WOUT_ARGS_ 0
#define _OPTION_WITH_ARGS_ 1
#define _OPTION_OPTIONAL_ARGS_ 2

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct option_entry {
    char option_char;
    #define _OPTION_LONG_SZ_ 0x20
    const char* option_long/*[_OPTION_LONG_SZ_]*/;
    #define _OPTION_DESC_SZ_ 0x50
    const char* option_desc/*[_OPTION_DESC_SZ_]*/;
    int* option_flag;
    int_fast8_t option_has;
    #define _OPTION_VALUE_SZ_ 0x30
    char option_value[_OPTION_VALUE_SZ_];
} option_entry_t;

typedef struct cursor_status {
    #define _CURSOR_MESSAGE_SZ_ 0x35
    char cursor_message[_CURSOR_MESSAGE_SZ_];
} cursor_status_t;

typedef struct option_cursor {
    const option_entry_t* cur_entry;
    int cur_index;
    const char* cur_sel;
    int cur_argc;
    char** cur_argv;
    cursor_status_t status;
} option_cursor_t;

#define CURSOR_ENTRY_FOREACH(entries, entry_ptr)\
    for (option_entry_t* entry_ptr = entries; entry_ptr->option_long != 0; entry_ptr++)

static void help_option_re(option_entry_t entry_options[])
{
    CURSOR_ENTRY_FOREACH(entry_options, entry) {} 
}

static void option_cursor_emplace(option_cursor_t* cursor, int argc, char** argv)
{
    option_cursor_t cursor_info = {
        .cur_entry = NULL, .cur_argc = argc, .cur_argv = argv
    };
    memcpy(cursor, &cursor_info, sizeof (cursor_info));
}

#define ENTRY_OPT_COMPARE(option, entry)\
    (strncmp(option, entry->option_long, 0x8) == 0)
typedef enum {CURSOR_HAS_END = 0, CURSOR_NOT_AN_OPTION} cursor_status_type_e;
static int32_t option_next(option_cursor_t* cursor, option_entry_t entries[])
{
    #define CURSOR_ERROR -1

    cursor_status_t* status = &cursor->status;
    cursor->cur_entry = NULL;

    if (cursor->cur_argc <= 0 && cursor->cur_argc >= cursor->cur_index) {
        snprintf(status->cursor_message, sizeof (status->cursor_message), "cursor is already parsed this args (%d)", cursor->cur_index);
        return CURSOR_HAS_END;
    }
    
    #define NEXT_CURSOR_SELECTION(cursor, ret_failed)\
        cursor->cur_sel = cursor->cur_argv[cursor->cur_index++];\
        if (cursor->cur_sel == NULL)\
            return ret_failed;

    NEXT_CURSOR_SELECTION(cursor, CURSOR_HAS_END);

    if (*cursor->cur_sel != '-') {
        snprintf(status->cursor_message, sizeof (status->cursor_message), "%s isn't a argument and an option value", cursor->cur_sel);
        return CURSOR_NOT_AN_OPTION;
    }
    else
        cursor->cur_sel++;
        
    CURSOR_ENTRY_FOREACH(entries, entry_opt) {
        if (strlen(cursor->cur_sel) == 1) goto simple_option;
        if (ENTRY_OPT_COMPARE(cursor->cur_sel, entry_opt) == false) continue;
        simple_option: 
        if (entry_opt->option_char != *cursor->cur_sel) continue;

        if (entry_opt->option_has == _OPTION_WOUT_ARGS_) goto wout;

        if ((cursor->cur_sel = strchr(cursor->cur_sel, '='))) {
            /* Option value is inside the actual option, we don't need to look at another option */
            strncpy(entry_opt->option_value, ++cursor->cur_sel, sizeof (entry_opt->option_value) - 1);
        } else {
            NEXT_CURSOR_SELECTION(cursor, CURSOR_HAS_END);
            strncpy(entry_opt->option_value, cursor->cur_sel, sizeof (entry_opt->option_value) - 1);
        }
        wout:
        cursor->cur_entry = entry_opt;
        return cursor->cur_index;
    }
    snprintf(status->cursor_message, sizeof (status->cursor_message), "%s isn't an option or something like that!", cursor->cur_sel);
    return CURSOR_NOT_AN_OPTION;
}

int main(int main_argc, char** main_argv)
{
    #define OPT_ENTRY_FILL(...)\
        {__VA_ARGS__}
    static option_entry_t droidcat_options[] = {
        #define _OPTION_LONG_HELP_ "help"
        OPT_ENTRY_FILL('h', _OPTION_LONG_HELP_, "Display help table content"),
        OPT_ENTRY_FILL(0, 0, 0)
    };

    option_cursor_t cursor_opt;
    option_cursor_emplace(&cursor_opt, main_argc, main_argv);

    #define OPTION_COMPARE_AND_CALL(option, entry_opt, call, ...)\
        if (ENTRY_OPT_COMPARE(option, entry_opt)) call(__VA_ARGS__);

    while(option_next(&cursor_opt, droidcat_options) != CURSOR_HAS_END) {
        const option_entry_t* entry = cursor_opt.cur_entry;
        if (entry == NULL) {
            /* This option doesn't exist or is wrong and maybe be reported to the user */
            continue;
        }

        OPTION_COMPARE_AND_CALL(_OPTION_LONG_HELP_, entry, help_option_re, droidcat_options);
    }

    (void)droidcat_options;
}
