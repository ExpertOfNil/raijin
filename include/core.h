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
#define WGPU_INIT_TIMEOUT_NS UINT64_C(10000000000)
#define WGPU_READBACK_TIMEOUT_NS UINT64_C(30000000000)
#define WGPU_WAIT_POLL_INTERVAL_NS UINT64_C(1000000)
#define WGPU_WAIT_SLICE_NS UINT64_C(50000000)

// Helper macro to get just the filename (not full path)
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

typedef enum {
    RETURN_SUCCESS,
    RETURN_FAILURE,
    RETURN_TIMEOUT,
    RETURN_DEVICE_LOST,
    RETURN_SURFACE_RECONFIGURE,
    RETURN_SURFACE_LOST,
} ReturnStatus;

typedef struct MouseState {
    bool button_left;
    bool button_middle;
    bool button_right;
    vec2 position;
} MouseState;

typedef struct WgpuWaitConfig {
    bool timed_wait_enabled;
    size_t timed_wait_max_count;
} WgpuWaitConfig;

/* Function Prototypes */

WGPUBuffer create_buffer(
    WGPUDevice device,
    const u64 size,
    const WGPUBufferUsage usage,
    const char* label
);

ReturnStatus wait_for_gpu_futures(
    WGPUInstance instance,
    const WgpuWaitConfig* config,
    WGPUFutureWaitInfo* wait_infos,
    size_t wait_info_count,
    const bool* const* completion_flags,
    size_t completion_count,
    uint64_t timeout_ns
);

#endif /* TYPES_H */
