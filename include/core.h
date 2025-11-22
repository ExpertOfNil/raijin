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
#include <cglm/vec2.h>

#include "webgpu.h"
#include "cimpl_core.h"
#include "cimpl_string.h"

#define VEC_MAX_WRITE 64
#define EPSILON 1e-9
#define DEFAULT_ARRAY_CAPACITY 64

#ifndef RAIJIN_ASSETS_DIR
#define RAIJIN_ASSETS_DIR "assets"
#endif

// Helper macro to get just the filename (not full path)
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

DEFINE_DYNAMIC_ARRAY(char, CharArray)

typedef enum {
    RETURN_SUCCESS,
    RETURN_FAILURE,
} ReturnStatus;

typedef struct MouseState {
    bool button_left;
    bool button_middle;
    bool button_right;
    vec2 position;
} MouseState;

/* Function Prototypes */

WGPUBuffer create_buffer(
    WGPUDevice device,
    const u32 size,
    const WGPUBufferUsage usage,
    const char* label
);
ReturnStatus load_shader(const char* path, String* buffer);

/* Functions */

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
    return buffer;
}

/** Load a shader from path
 *
 * @param[in] path          File path
 * @param[in,out] buffer    Buffer to write file contents to
 * @returns                 Return status
 */
// TODO (mmckenna): add validation
ReturnStatus load_shader(const char* path, String* buffer) {
    log_debug("Loading shader from %s", path);
    FILE* f = fopen(path, "rb");
    if (!f) {
        log_error("Failed to open file: %s", path);
        return RETURN_SUCCESS;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to read file: %s", path);
        return RETURN_FAILURE;
    }

    size_t file_size = ftell(f);
    if (file_size < 0) {
        log_error("Failed to get file size");
        fclose(f);
        return RETURN_FAILURE;
    }
    rewind(f);

    String_reserve(buffer, file_size);
    if (!buffer->items) {
        log_error("Failed to allocate memory for buffer");
        fclose(f);
        return RETURN_FAILURE;
    }

    if (fread(buffer->items, 1, file_size, f) != file_size) {
        log_error("Failed to read file");
        fclose(f);
        return RETURN_FAILURE;
    }
    buffer->count += file_size;
    fclose(f);
    log_trace("Shader contents:\n%.*s", (int)buffer->count, buffer->items);
    return RETURN_SUCCESS;
}

#endif /* TYPES_H */
