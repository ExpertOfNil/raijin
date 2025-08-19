#ifndef TYPES_H
#define TYPES_H

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "webgpu.h"

#define VEC_MAX_WRITE 64
#define EPSILON 1e-9
#define DEFAULT_ARRAY_CAPACITY 64
#define DEG2RAD M_PI/180.0f
#define RAD2DEG 180.0f/M_PI

#ifndef RAIJIN_REALLOC
#include <stdlib.h>
#define RAIJIN_REALLOC realloc
#endif

#ifndef RAIJIN_FREE
#include <stdlib.h>
#define RAIJIN_FREE free
#endif

#ifndef RAIJIN_ASSERT
#include <assert.h>
#define RAIJIN_ASSERT assert
#endif

#ifndef RAIJIN_ASSETS_DIR
#define RAIJIN_ASSETS_DIR "assets"
#endif

#define DEFINE_DYNAMIC_ARRAY(type, name)                               \
    typedef struct name {                                              \
        type* items;                                                   \
        size_t count;                                                  \
        size_t capacity;                                               \
    } name;                                                            \
                                                                       \
    static inline void name##_init(name* arr) {                        \
        arr->items = NULL;                                             \
        arr->count = 0;                                                \
        arr->capacity = 0;                                             \
    }                                                                  \
                                                                       \
    static inline void name##_reserve(name* arr, size_t cap) {         \
        if ((cap) > (arr)->capacity) {                                 \
            if ((arr)->capacity == 0) {                                \
                (arr)->capacity = DEFAULT_ARRAY_CAPACITY;              \
            }                                                          \
            while ((cap) > (arr)->capacity) {                          \
                (arr)->capacity *= 2;                                  \
            }                                                          \
            (arr)->items = RAIJIN_REALLOC(                             \
                (arr)->items, (arr)->capacity * sizeof(*(arr)->items)  \
            );                                                         \
            RAIJIN_ASSERT(                                             \
                (arr)->items != NULL && "ARRAY_RESERVE: Out of memory" \
            );                                                         \
        }                                                              \
    }                                                                  \
                                                                       \
    static inline void name##_push(name* arr, type item) {             \
        name##_reserve((arr), (arr)->count + 1);                       \
        arr->items[arr->count++] = item;                               \
    }                                                                  \
                                                                       \
    static inline void name##_push_many(                               \
        name* arr, const type* new_items, u32 new_items_count          \
    ) {                                                                \
        name##_reserve((arr), (arr)->count + (new_items_count));       \
        memcpy(                                                        \
            (arr)->items + (arr)->count,                               \
            (new_items),                                               \
            (new_items_count) * sizeof(*(arr)->items)                  \
        );                                                             \
        (arr)->count += (new_items_count);                             \
    }                                                                  \
                                                                       \
    static inline void name##_free(name* arr) {                        \
        if (arr->items) {                                              \
            RAIJIN_FREE(arr->items);                                   \
            arr->items = NULL;                                         \
        }                                                              \
        arr->count = 0;                                                \
        arr->capacity = 0;                                             \
    }

#define ARRAY_COUNT(arr) (sizeof((arr)) / sizeof((arr)[0]))

typedef enum {
    RETURN_SUCCESS,
    RETURN_FAILURE,
} ReturnStatus;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ssize_t isize;

typedef float f32;
typedef double f64;

typedef enum LogLevel {
    LOG_LEVEL_NONE,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
} LogLevel;

#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY LOG_LEVEL_INFO
#endif

// Helper macro to get just the filename (not full path)
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Base logging macro with timestamp, file, and line
#define LOG(level, level_str, fmt, ...)                                        \
    do {                                                                       \
        if (LOG_VERBOSITY <= level) {                                          \
            time_t now = time(NULL);                                           \
            struct tm* tm_info = localtime(&now);                              \
            char timestamp[64];                                                \
            strftime(                                                          \
                timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info     \
            );                                                                 \
            if ((level) == LOG_LEVEL_ERROR || (level) == LOG_LEVEL_CRITICAL) { \
                fprintf(                                                       \
                    stderr,                                                    \
                    "[%s] [%s] %s:%d: " fmt "\n",                              \
                    timestamp,                                                 \
                    level_str,                                                 \
                    __FILENAME__,                                              \
                    __LINE__,                                                  \
                    ##__VA_ARGS__                                              \
                );                                                             \
            } else {                                                           \
                printf(                                                        \
                    "[%s] [%s] %s:%d: " fmt "\n",                              \
                    timestamp,                                                 \
                    level_str,                                                 \
                    __FILENAME__,                                              \
                    __LINE__,                                                  \
                    ##__VA_ARGS__                                              \
                );                                                             \
            }                                                                  \
        }                                                                      \
    } while (0)

// Specific log level macros
#define LOG_TRACE(fmt, ...) LOG(LOG_LEVEL_TRACE, "TRACE", fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG(LOG_LEVEL_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(LOG_LEVEL_INFO, "INFO", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG(LOG_LEVEL_WARN, "WARN", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG_LEVEL_ERROR, "ERROR", fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) \
    LOG(LOG_LEVEL_CRITICAL, "CRITICAL", fmt, ##__VA_ARGS__)

/** Create a WGPUBuffer
 *
 * @param[in,out] device    Device on which to create the buffer
 * @param[in] data          Pointer to the data to put into the buffer
 * @param[in] size          Size of the data, in bytes
 * @param[in] usage         Usage flags for how the buffer will be used
 * @param[in] label         Buffer label
 * @returns                 Pointer to the created buffer
 */
WGPUBuffer create_buffer(
    WGPUDevice device,
    const void* data,
    const u32 size,
    const WGPUBufferUsage usage,
    const char* label
) {
    WGPUBufferDescriptor buffer_desc = {
        .label = {label, WGPU_STRLEN},
        .usage = usage,
        .size = size,
        .mappedAtCreation = false,
    };

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &buffer_desc);
    if (!buffer) {
        fprintf(stderr, "Failed to create vertex buffer");
        return NULL;
    }

    wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), buffer, 0, data, size);
    LOG_INFO("%s created successfully", label);
    return buffer;
}

char* load_shader(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to read file: %s", path);
        return NULL;
    }

    size_t file_size = ftell(f);
    if (file_size < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    rewind(f);

    char* buffer = malloc(file_size + 1);
    if (!buffer) {
        perror("malloc");
        fclose(f);
        return NULL;
    }

    if (fread(buffer, 1, file_size, f) != file_size) {
        perror("fread");
        free(buffer);
        fclose(f);
        return NULL;
    }
    buffer[file_size] = '\0';
    fclose(f);
    return buffer;
}

#endif /* TYPES_H */
