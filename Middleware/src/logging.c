#include "logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LEVEL_PREFIX_LENGTH     25
#define TIME_LENGTH             24
#define FILE_NAME_LENGTH        255
#define FULL_LOG_MAX_LENGTH     (LOGGING_MAX_MSG_LENGTH + LEVEL_PREFIX_LENGTH + TIME_LENGTH + FILE_NAME_LENGTH + 1)

#define GET_IMPL(LOGGER) ((logger_impl_t*)LOGGER->_private)

#define BLACK_COLOR(STRING_VAL) "\033[30m" STRING_VAL "\033[0m"
#define RED_COLOR(STRING_VAL) "\033[31m" STRING_VAL "\033[0m"
#define GREEN_COLOR(STRING_VAL) "\033[32m" STRING_VAL "\033[0m"
#define YELLOW_COLOR(STRING_VAL) "\033[33m" STRING_VAL "\033[0m"
#define BLUE_COLOR(STRING_VAL) "\033[34m" STRING_VAL "\033[0m"
#define PURPLE_COLOR(STRING_VAL) "\033[35m" STRING_VAL "\033[0m"
#define AQUA_COLOR(STRING_VAL) "\033[36m" STRING_VAL "\033[0m"
#define WHITE_COLOR(STRING_VAL) "\033[37m" STRING_VAL "\033[0m"


typedef struct {
    logging_level_t level;
} logger_impl_t;
_Static_assert(sizeof(logger_impl_t) == sizeof(((logging_logger_t*)0)->_private), "Invalid context size");

static const char* LEVEL_PREFIX[] = {
    [LOGGING_LEVEL_DEFAULT] =  WHITE_COLOR("?????"), // should never be printed
    [LOGGING_LEVEL_DEBUG] =    BLUE_COLOR("DEBUG"),
    [LOGGING_LEVEL_INFO] =     WHITE_COLOR("INFO"),
    [LOGGING_LEVEL_WARN] =     YELLOW_COLOR("WARN"),
    [LOGGING_LEVEL_ERROR] =    RED_COLOR("ERROR"),
};

static logging_init_t m_init;
static char m_write_buffer[FULL_LOG_MAX_LENGTH];

bool logging_init(const logging_init_t* init) {
    if ((!init->write_function && !init->raw_write_function) || init->default_level == LOGGING_LEVEL_DEFAULT) {
        return false;
    }
    m_init = *init;
    m_init.write_function("\n\r");
    return true;
}

void logging_log_impl(logging_logger_t* logger,
                      logging_level_t level,
                      logging_color_t color,
                      char* file, int line, const char* fmt, ...) {
    memset(m_write_buffer, 0, FULL_LOG_MAX_LENGTH);
    logger_impl_t* impl = GET_IMPL(logger);
    const logging_level_t min_level = impl->level == LOGGING_LEVEL_DEFAULT ? m_init.default_level : impl->level;
    if (level < min_level) {
        return;
    }

    if (m_init.lock_function) {
        m_init.lock_function(true);
    }

    // time
    if (m_init.get_time_string) {
        char datetime_str[25] = {0};
        m_init.get_time_string(datetime_str);
        sniprintf(&m_write_buffer[strlen(m_write_buffer)],
                  FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
                  GREEN_COLOR("%s"), datetime_str);
    }

    // level

    snprintf(&m_write_buffer[strlen(m_write_buffer)],
    FULL_LOG_MAX_LENGTH-strlen(m_write_buffer),
    " | %-15s | ", LEVEL_PREFIX[level]);

    // module (if set)
    if (logger->module_prefix) {
        snprintf(&m_write_buffer[strlen(m_write_buffer)],
        FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
        "%s", logger->module_prefix);
    }

    // file name
    snprintf(&m_write_buffer[strlen(m_write_buffer)],
             FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
             AQUA_COLOR("%s"), file);

    // line number
    snprintf(&m_write_buffer[strlen(m_write_buffer)],
             FULL_LOG_MAX_LENGTH-strlen(m_write_buffer),
             ":" AQUA_COLOR("%d") " - ", line);

    // log message
    snprintf(&m_write_buffer[strlen(m_write_buffer)], 6, "\033[%dm", color);
    va_list args;
    va_start(args, fmt);
    vsnprintf(&m_write_buffer[strlen(m_write_buffer)],
              FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
              fmt, args);
    va_end(args);
    snprintf(&m_write_buffer[strlen(m_write_buffer)], 5, "\033[0m\n\r");

    // newline
    snprintf(&m_write_buffer[strlen(m_write_buffer)],
             FULL_LOG_MAX_LENGTH - strlen(m_write_buffer), "\n\r");

    if (m_init.write_function) {
        m_init.write_function(m_write_buffer);
    }
    if (m_init.raw_write_function) {
        m_init.raw_write_function(level, logger->module_prefix, m_write_buffer);
    }

    if (m_init.lock_function) {
        m_init.lock_function(false);
    }
}

void logging_set_level_impl(logging_logger_t* logger, logging_level_t level) {
    GET_IMPL(logger)->level = level;
}


// #include "logging.h"

// #include <stdarg.h>
// #include <stdio.h>
// #include <string.h>

// #define LEVEL_PREFIX_LENGTH     25
// #define TIME_LENGTH             24
// #define FILE_NAME_LENGTH        255
// #define FULL_LOG_MAX_LENGTH     (LOGGING_MAX_MSG_LENGTH + LEVEL_PREFIX_LENGTH + TIME_LENGTH + FILE_NAME_LENGTH + 1)

// #define GET_IMPL(LOGGER) ((logger_impl_t*)LOGGER->_private)

// #define BLACK_COLOR(STRING_VAL) "\033[30m" STRING_VAL "\033[0m"
// #define RED_COLOR(STRING_VAL) "\033[31m" STRING_VAL "\033[0m"
// #define GREEN_COLOR(STRING_VAL) "\033[32m" STRING_VAL "\033[0m"
// #define YELLOW_COLOR(STRING_VAL) "\033[33m" STRING_VAL "\033[0m"
// #define BLUE_COLOR(STRING_VAL) "\033[34m" STRING_VAL "\033[0m"
// #define PURPLE_COLOR(STRING_VAL) "\033[35m" STRING_VAL "\033[0m"
// #define AQUA_COLOR(STRING_VAL) "\033[36m" STRING_VAL "\033[0m"
// #define WHITE_COLOR(STRING_VAL) "\033[37m" STRING_VAL "\033[0m"


// typedef struct {
//     logging_level_t level;
// } logger_impl_t;
// _Static_assert(sizeof(logger_impl_t) == sizeof(((logging_logger_t*)0)->_private), "Invalid context size");

// static const char* LEVEL_PREFIX[] = {
//     [LOGGING_LEVEL_DEFAULT] =  "?????", // should never be printed
//     [LOGGING_LEVEL_DEBUG] =    "DEBUG",
//     [LOGGING_LEVEL_INFO] =     "INFO",
//     [LOGGING_LEVEL_WARN] =     "WARN",
//     [LOGGING_LEVEL_ERROR] =    "ERROR",
// };

// static logging_init_t m_init;
// static char m_write_buffer[FULL_LOG_MAX_LENGTH];

// bool logging_init(const logging_init_t* init) {
//     if ((!init->write_function && !init->raw_write_function) || init->default_level == LOGGING_LEVEL_DEFAULT) {
//         return false;
//     }
//     m_init = *init;
//     return true;
// }

// void colored(char* buffer, size_t buffer_size, logging_color_t color, const char* fmt, ...){
//     snprintf(buffer + strlen(m_write_buffer), 6, "\033[%dm", color);
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer + strlen(m_write_buffer), FULL_LOG_MAX_LENGTH - strlen(buffer), fmt, args);
//     va_end(args);
//     snprintf(buffer + strlen(m_write_buffer), 4, "\033[0m");
// }

// void logging_log_impl(logging_logger_t* logger,
//                       logging_level_t level,
//                       logging_color_t color,
//                       char* file, int line, const char* fmt, ...) {
//     memset(m_write_buffer, 0, FULL_LOG_MAX_LENGTH);
//     logger_impl_t* impl = GET_IMPL(logger);
//     const logging_level_t min_level = impl->level == LOGGING_LEVEL_DEFAULT ? m_init.default_level : impl->level;
//     if (level < min_level) {
//         return;
//     }

//     if (m_init.lock_function) {
//         m_init.lock_function(true);
//     }

//     // time
//     if (m_init.get_time_string) {
//         char datetime_str[25] = {0};
//         m_init.get_time_string(datetime_str);
//         colored(m_write_buffer, FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
//                 LOGGING_COLOR_GREEN, "%s", datetime_str);
//     }

//     // level
//     colored(m_write_buffer, FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
//             color, " | %-10s | ", LEVEL_PREFIX[level]);

//     // module (if set)
//     if (logger->module_prefix) {
//         snprintf(&m_write_buffer[strlen(m_write_buffer)],
//         FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
//         "%s", logger->module_prefix);
//     }

//     // file name
//     colored(m_write_buffer, FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
//             LOGGING_COLOR_AQUA, "%s", file);

//     // line number
//     colored(m_write_buffer, FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
//             LOGGING_COLOR_AQUA, ":%d - ", line);

//     // log message
//     va_list args;
//     colored(m_write_buffer, FULL_LOG_MAX_LENGTH - strlen(m_write_buffer),
//             color, fmt, args);

//     // newline
//     snprintf(&m_write_buffer[strlen(m_write_buffer)],
//              FULL_LOG_MAX_LENGTH - strlen(m_write_buffer), "\n");

//     if (m_init.write_function) {
//         m_init.write_function(m_write_buffer);
//     }
//     if (m_init.raw_write_function) {
//         m_init.raw_write_function(level, logger->module_prefix, m_write_buffer);
//     }

//     if (m_init.lock_function) {
//         m_init.lock_function(false);
//     }
// }

// void logging_set_level_impl(logging_logger_t* logger, logging_level_t level) {
//     GET_IMPL(logger)->level = level;
// }