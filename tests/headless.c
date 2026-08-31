#include <inttypes.h>
#include <raijin/raijin.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    RaijinContextDesc desc;
    desc.render_mode = RAIJIN_RENDER_MODE_HEADLESS;
    desc.title = "Raijin";
    desc.width = 65;
    desc.height = 33;

    int exit_status = EXIT_SUCCESS;
    RaijinContext* ctx = NULL;
    uint8_t* pixel_buf = NULL;

    // Create context
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

    // Verify empty frame rendering and command clearing
    result = raijin_context_render(ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr, "--> ERROR: Failed to render empty frame: %u\n", (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Create cube mesh
    RaijinMesh cube_mesh = RAIJIN_MESH_INVALID;
    result = raijin_context_get_builtin_mesh(
        ctx, RAIJIN_BUILTIN_MESH_CUBE, &cube_mesh
    );
    if (result != RAIJIN_SUCCESS || cube_mesh == RAIJIN_MESH_INVALID) {
        fprintf(
            stderr,
            "--> ERROR: Failed to initialize cube mesh: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Test invalid builtin arguments
    RaijinMesh invalid_output = UINT64_MAX;
    result = raijin_context_get_builtin_mesh(ctx, UINT32_MAX, &invalid_output);
    if (result != RAIJIN_ERROR_INVALID_ARGUMENT ||
        invalid_output != RAIJIN_MESH_INVALID) {
        fprintf(
            stderr,
            "--> ERROR: Failed invalid builtin test: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Test invalid submission arguments
    result = raijin_context_submit_instances(ctx, RAIJIN_MESH_INVALID, NULL, 0);
    if (result != RAIJIN_ERROR_INVALID_HANDLE) {
        fprintf(
            stderr,
            "--> ERROR: Failed invalid submission arguments test: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_context_submit_instances(ctx, cube_mesh, NULL, 1);
    if (result != RAIJIN_ERROR_INVALID_ARGUMENT) {
        fprintf(
            stderr,
            "--> ERROR: Failed invalid submission arguments test: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_context_submit_instances(ctx, cube_mesh, NULL, 0);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed invalid submission arguments test: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    RaijinInstance instance;
    raijin_instance_init(&instance);
    result = raijin_context_submit_instances(ctx, UINT64_MAX, &instance, 1);
    if (result != RAIJIN_ERROR_INVALID_HANDLE) {
        fprintf(
            stderr,
            "--> ERROR: Failed invalid submission arguments test: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Create cube instance
    RaijinInstance cube;
    raijin_instance_init(&cube);

    cube.color[0] = 0.0f;
    cube.color[1] = 1.0f;
    cube.color[2] = 0.0f;

    result = raijin_context_draw_cube(ctx, &cube);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr, "--> ERROR: Failed to draw cube: %u\n", (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Create multiple cube instances
    RaijinInstance cubes[2];
    raijin_instance_init(&cubes[0]);
    raijin_instance_init(&cubes[1]);

    cubes[0].transform[12] = -1.5f;
    cubes[0].color[0] = 1.0f;
    cubes[0].color[1] = 0.0f;
    cubes[0].color[2] = 0.0f;

    cubes[1].transform[12] = 1.5f;
    cubes[1].color[0] = 0.0f;
    cubes[1].color[1] = 1.0f;
    cubes[1].color[2] = 0.0f;

    result = raijin_context_submit_instances(ctx, cube_mesh, cubes, 2);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed to submit multiple cube instances: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    enum { GROWTH_INSTANCE_COUNT = 257 };
    RaijinInstance growth_instances[GROWTH_INSTANCE_COUNT];

    for (uint32_t i = 0; i < GROWTH_INSTANCE_COUNT; ++i) {
        raijin_instance_init(&growth_instances[i]);
    }

    result = raijin_context_submit_instances(
        ctx, cube_mesh, growth_instances, GROWTH_INSTANCE_COUNT
    );

    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed instance buffer growth submission: %u",
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

    // Read frame to buffer
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

    // Check for geometry in buffer
    // TODO (mmckenna): for test compare against frame with no geometry
    bool found_geometry = false;
    for (uint64_t offset = 4; offset < pixel_capacity; offset += 4) {
        if (memcmp(pixel_buf, pixel_buf + offset, 3) != 0) {
            found_geometry = true;
            break;
        }
    }

    if (!found_geometry) {
        fprintf(stderr, "--> ERROR: Rendered frame has no visible geometry\n");
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

cleanup:
    if (pixel_buf) free(pixel_buf);
    raijin_context_destroy(ctx);
    return exit_status;
}
