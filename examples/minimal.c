#include <inttypes.h>
#include <raijin/raijin.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    RaijinContextDesc desc;
    desc.render_mode = RAIJIN_RENDER_MODE_HEADLESS;
    desc.title = "Raijin";
    desc.width = 65;
    desc.height = 33;

    int exit_status = EXIT_SUCCESS;
    RaijinContext* ctx = NULL;
    RaijinContext* windowed_ctx = NULL;
    uint8_t* pixel_buf = NULL;

    RaijinResult result = raijin_context_create(&desc, &ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed to initialize Raijin context: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_context_render(ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr, "--> ERROR: Failed to render frame: %u\n", (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    uint64_t pixel_capacity = 0;
    result = raijin_context_readback_size(ctx, &pixel_capacity);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed to get buffer readback size: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    if (pixel_capacity !=
        (uint64_t)desc.width * (uint64_t)desc.height * UINT64_C(4)) {
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    printf("--> INFO: pixel_capacity = %" PRIu64 "\n", pixel_capacity);

    pixel_buf = (uint8_t*)malloc((size_t)pixel_capacity);
    if (pixel_buf == NULL) {
        fprintf(stderr, "--> ERROR: Failed to create pixel buffer\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_context_read_rgba8(ctx, pixel_buf, pixel_capacity);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed to read pixels into buffer: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Expected error
    result = raijin_context_read_rgba8(ctx, pixel_buf, pixel_capacity - 1);
    uint32_t expected_result = RAIJIN_ERROR_BUFFER_TOO_SMALL;
    if (result != expected_result) {
        fprintf(
            stderr,
            "--> ERROR: Unexpected result: expected=%u, got=%u\n",
            expected_result,
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Expected error
    RaijinContextDesc windowed_desc = desc;
    windowed_desc.render_mode = RAIJIN_RENDER_MODE_WINDOWED;
    result = raijin_context_create(&windowed_desc, &windowed_ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed to initialize Raijin context: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    result = raijin_context_readback_size(windowed_ctx, &pixel_capacity);
    expected_result = RAIJIN_ERROR_INVALID_STATE;
    if (result != expected_result) {
        fprintf(
            stderr,
            "--> ERROR: Unexpected result: expected=%u, got=%u\n",
            expected_result,
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

cleanup:
    if (pixel_buf) free(pixel_buf);
    raijin_context_destroy(ctx);
    raijin_context_destroy(windowed_ctx);
    return exit_status;
}
