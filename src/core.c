#define CIMPL_IMPLEMENTATION
#include "core.h"

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
    if (buffer == NULL) {
        log_error("Failed to create buffer: %s", label);
    }
    return buffer;
}
