#ifndef TYPES_H
#define TYPES_H

#include <cglm/vec2.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cimpl_core.h"
#include "webgpu.h"

#define VEC_MAX_WRITE 64
#define EPSILON 1e-9
#define DEFAULT_ARRAY_CAPACITY 64

// Helper macro to get just the filename (not full path)
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

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
    const u64 size,
    const WGPUBufferUsage usage,
    const char* label
);

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
    const u64 size,
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

#endif /* TYPES_H */
