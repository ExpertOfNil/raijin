#ifndef CIMPL_CORE_H
#define CIMPL_CORE_H

#include <memory.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 1
#endif

#ifndef CIMPL_ALLOC
#include <stdlib.h>
#define CIMPL_ALLOC malloc
#endif

#ifndef CIMPL_FREE
#include <stdlib.h>
#define CIMPL_FREE free
#endif

#ifndef CIMPL_REALLOC
#include <stdlib.h>
#define CIMPL_REALLOC realloc
#endif

#ifndef CIMPL_ASSERT
#include <assert.h>
#define CIMPL_ASSERT assert
#endif

typedef size_t usize;
typedef ptrdiff_t isize;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef unsigned char uchar;
typedef signed char ichar;

typedef enum {
    RETURN_OK,
    RETURN_ERR,
} CimplReturn;

#define DEFAULT_ARRAY_CAPACITY 64
#define ARRAY_COUNT(arr) (sizeof((arr)) / sizeof((arr)[0]))
#define DEFINE_DYNAMIC_ARRAY(type, name)                               \
    typedef struct name {                                              \
        type* items;                                                   \
        u32 count;                                                     \
        u32 capacity;                                                  \
    } name;                                                            \
                                                                       \
    static inline void name##_init(name* arr) {                        \
        arr->items = NULL;                                             \
        arr->count = 0;                                                \
        arr->capacity = 0;                                             \
    }                                                                  \
                                                                       \
    static inline CimplReturn name##_reserve(name* arr, size_t cap) {  \
        if ((cap) > (arr)->capacity) {                                 \
            if ((arr)->capacity == 0) {                                \
                (arr)->capacity = DEFAULT_ARRAY_CAPACITY;              \
            }                                                          \
            while ((cap) > (arr)->capacity) {                          \
                (arr)->capacity *= 2;                                  \
            }                                                          \
            (arr)->items = CIMPL_REALLOC(                              \
                (arr)->items, (arr)->capacity * sizeof(*(arr)->items)  \
            );                                                         \
            if ((arr)->items == NULL) {                                \
                fprintf(stderr, "ARRAY_RESERVE: Out of memory\n");     \
                return RETURN_ERR;                                     \
            }                                                          \
        }                                                              \
        return RETURN_OK;                                              \
    }                                                                  \
                                                                       \
    static inline CimplReturn name##_push(name* arr, type item) {      \
        if (name##_reserve((arr), (arr)->count + 1) != RETURN_OK) {    \
            return RETURN_ERR;                                         \
        }                                                              \
        arr->items[arr->count++] = item;                               \
        return RETURN_OK;                                              \
    }                                                                  \
                                                                       \
    static inline CimplReturn name##_push_many(                        \
        name* arr, const type* new_items, u32 new_items_count          \
    ) {                                                                \
        if (name##_reserve((arr), (arr)->count + (new_items_count)) != \
            RETURN_OK) {                                               \
            return RETURN_ERR;                                         \
        }                                                              \
        memcpy(                                                        \
            (arr)->items + (arr)->count,                               \
            (new_items),                                               \
            (new_items_count) * sizeof(*(arr)->items)                  \
        );                                                             \
        (arr)->count += (new_items_count);                             \
        return RETURN_OK;                                              \
    }                                                                  \
                                                                       \
    static inline void name##_clear(name* arr) {                       \
        memset(arr->items, 0, sizeof(*(arr)->items) * arr->count);     \
        arr->count = 0;                                                \
    }                                                                  \
                                                                       \
    static inline void name##_free(name* arr) {                        \
        if (arr->items) {                                              \
            CIMPL_FREE(arr->items);                                    \
            arr->items = NULL;                                         \
        }                                                              \
        arr->count = 0;                                                \
        arr->capacity = 0;                                             \
    }

/*** FUNCTION DECLARATIONS ***/

inline static u32 randi(u32 index);
const char* get_timestamp(void);
void log_message(const char* level_str, const char* format, va_list args);
void log_trace(const char* format, ...);
void log_debug(const char* format, ...);
void log_info(const char* format, ...);
void log_warn(const char* format, ...);
void log_error(const char* format, ...);

/*** FUNCTION DEFINITIONS ***/

#ifdef CIMPL_IMPLEMENTATION
inline static u32 randi(u32 index) {
    index = (index << 13) ^ index;
    return (index * (index * index * 15731 + 789221) + 1276312589) & 0x7fffffff;
}

const char* get_timestamp(void) {
    static char timestamp[20];
    static time_t last_time = 0;

    time_t now = time(NULL);
    if (now != last_time) {
        last_time = now;
        struct tm* timeinfo = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%H:%M:%S", timeinfo);
    }
    return timestamp;
}

void log_message(const char* level_str, const char* format, va_list args) {
    printf("[%s] %s: ", get_timestamp(), level_str);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);

    va_end(args);
}
void log_trace(const char* format, ...) {
    const u8 level = 0;
    if (LOG_LEVEL > level) return;
    va_list args;
    va_start(args, format);
    log_message("TRACE", format, args);
}
void log_debug(const char* format, ...) {
    const u8 level = 1;
    if (LOG_LEVEL > level) return;
    va_list args;
    va_start(args, format);
    log_message("DEBUG", format, args);
}
void log_info(const char* format, ...) {
    const u8 level = 2;
    if (LOG_LEVEL > level) return;
    va_list args;
    va_start(args, format);
    log_message("INFO", format, args);
}
void log_warn(const char* format, ...) {
    const u8 level = 3;
    if (LOG_LEVEL > level) return;
    va_list args;
    va_start(args, format);
    log_message("WARN", format, args);
}
void log_error(const char* format, ...) {
    const u8 level = 4;
    if (LOG_LEVEL > level) return;
    va_list args;
    va_start(args, format);

    fprintf(stderr, "[%s] %s: ", get_timestamp(), "ERROR");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    fflush(stderr);
    va_end(args);
}
#endif /* CIMPL_IMPLEMENTATION */

#endif /* CIMPL_CORE_H */
