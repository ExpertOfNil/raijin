#define _POSIX_C_SOURCE 200809L
#define CIMPL_IMPLEMENTATION
#include "core.h"

#include <errno.h>

#include "logging.h"

static ReturnStatus monotonic_time_ns(uint64_t* out) {
    struct timespec now;
    if (out == NULL) return RETURN_FAILURE;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        log_error("Failed to read monotonic clock\n");
        return RETURN_FAILURE;
    }

    if (now.tv_sec < 0 || now.tv_nsec < 0) {
        log_error("Monotonic clock returned a negative value\n");
        return RETURN_FAILURE;
    }

    const uint64_t seconds = (uint64_t)now.tv_sec;
    const uint64_t nanoseconds = (uint64_t)now.tv_nsec;

    if (seconds > (UINT64_MAX - nanoseconds) / UINT64_C(1000000000)) {
        log_error("Monotonic clock value overflow\n");
        return RETURN_FAILURE;
    }

    *out = seconds * UINT64_C(1000000000) + nanoseconds;
    return RETURN_SUCCESS;
}

static bool any_callback_completed(
    const bool* const* completion_flags, size_t completion_count
) {
    for (size_t i = 0; i < completion_count; ++i) {
        if (completion_flags[i] != NULL && *completion_flags[i]) return true;
    }
    return false;
}

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
        log_error("Failed to create buffer: %s\n", label);
    }
    return buffer;
}

ReturnStatus wait_for_gpu_futures(
    WGPUInstance instance,
    const WgpuWaitConfig* config,
    WGPUFutureWaitInfo* wait_infos,
    size_t wait_info_count,
    const bool* const* completion_flags,
    size_t completion_count,
    uint64_t timeout_ns
) {
    if (instance == NULL || config == NULL || wait_infos == NULL ||
        wait_info_count == 0 || completion_flags == NULL ||
        completion_count == 0 || timeout_ns == 0) {
        return RETURN_FAILURE;
    }
    uint64_t now_ns;
    if (monotonic_time_ns(&now_ns) != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }
    const uint64_t deadline_ns =
        timeout_ns > UINT64_MAX - now_ns ? UINT64_MAX : now_ns + timeout_ns;
    bool use_timed_wait = config->timed_wait_enabled &&
                          wait_info_count <= config->timed_wait_max_count;
    for (;;) {
        if (any_callback_completed(completion_flags, completion_count)) {
            return RETURN_SUCCESS;
        }

        if (monotonic_time_ns(&now_ns) != RETURN_SUCCESS) return RETURN_FAILURE;

        if (now_ns >= deadline_ns) return RETURN_TIMEOUT;

        const uint64_t remaining_ns = deadline_ns - now_ns;

        if (use_timed_wait) {
            uint64_t wait_ns = remaining_ns < WGPU_WAIT_SLICE_NS
                                   ? remaining_ns
                                   : WGPU_WAIT_SLICE_NS;
            WGPUWaitStatus wait_status = wgpuInstanceWaitAny(
                instance, wait_info_count, wait_infos, wait_ns
            );
            switch (wait_status) {
                case WGPUWaitStatus_Success:
                    if (!any_callback_completed(
                            completion_flags, completion_count
                        )) {
                        log_error(
                            "WaitAny succeeded without invoking a supplied "
                            "callback\n"
                        );
                        return RETURN_FAILURE;
                    }
                    return RETURN_SUCCESS;
                case WGPUWaitStatus_TimedOut:
                    break;
                case WGPUWaitStatus_UnsupportedTimeout:
                case WGPUWaitStatus_UnsupportedCount:
                    use_timed_wait = false;
                    continue;
                case WGPUWaitStatus_UnsupportedMixedSources:
                default:
                    log_error(
                        "WaitAny failed: status=%u\n", (uint32_t)wait_status
                    );
                    return RETURN_FAILURE;
            }
        }

        wgpuInstanceProcessEvents(instance);
        if (any_callback_completed(completion_flags, completion_count)) {
            return RETURN_SUCCESS;
        }
        if (use_timed_wait) continue;
        if (monotonic_time_ns(&now_ns) != RETURN_SUCCESS) return RETURN_FAILURE;
        if (now_ns >= deadline_ns) return RETURN_TIMEOUT;

        const uint64_t fallback_remaining_ns = deadline_ns - now_ns;
        const uint64_t sleep_ns =
            fallback_remaining_ns < WGPU_WAIT_POLL_INTERVAL_NS
                ? fallback_remaining_ns
                : WGPU_WAIT_POLL_INTERVAL_NS;

        struct timespec sleep_time = {
            .tv_sec = (time_t)(sleep_ns / UINT64_C(1000000000)),
            .tv_nsec = (long)(sleep_ns % UINT64_C(1000000000)),
        };

        if (nanosleep(&sleep_time, NULL) != 0 && errno != EINTR) {
            log_error("Failed to sleep while polling WGPU future\n");
            return RETURN_FAILURE;
        }
    }
}
