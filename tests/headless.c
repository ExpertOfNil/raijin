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
    RaijinContext* other_ctx = NULL;
    uint8_t* pixel_buf = NULL;

    // Create context
    RaijinResult result = raijin_create(&desc, &ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed to initialize Raijin context: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    float clear_color[4];
    result = raijin_get_clear_color(ctx, clear_color);
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: Failed to get clear color\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    if (clear_color[0] != 0.03f || clear_color[1] != 0.03f ||
        clear_color[2] != 0.03f || clear_color[3] != 1.0f) {
        fprintf(stderr, "--> ERROR: Invalid initial clear color\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    clear_color[0] = 1.0f;
    clear_color[1] = 1.0f;
    clear_color[2] = 1.0f;
    clear_color[3] = 1.0f;
    result = raijin_set_clear_color(ctx, clear_color);
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: Failed to set clear color\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    result = raijin_get_clear_color(ctx, clear_color);
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: failed to get clear color\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }
    if (clear_color[0] != 1.0f || clear_color[1] != 1.0f ||
        clear_color[2] != 1.0f || clear_color[3] != 1.0f) {
        fprintf(
            stderr,
            "--> ERROR: Invalid clear color received. Got: [%.2f, %.2f, %.2f, "
            "%.2f]\n",
            clear_color[0],
            clear_color[1],
            clear_color[2],
            clear_color[3]
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Verify public edge-visibility controls
    uint32_t edges_visible = UINT32_MAX;

    result = raijin_get_edge_visibility(ctx, &edges_visible);
    if (result != RAIJIN_SUCCESS || edges_visible != 0) {
        fprintf(stderr, "--> ERROR: Invalid initial edge visibility\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_set_edge_visibility(ctx, 2);
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: Failed to enable edge visibility\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_get_edge_visibility(ctx, &edges_visible);
    if (result != RAIJIN_SUCCESS || edges_visible != 1) {
        fprintf(stderr, "--> ERROR: Edge visibility was not enabled\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_set_edge_visibility(ctx, 0);
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: Failed to disable edge visibility\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_get_edge_visibility(ctx, &edges_visible);
    if (result != RAIJIN_SUCCESS || edges_visible != 0) {
        fprintf(stderr, "--> ERROR: Edge visibility was not disabled\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Verify empty frame rendering and command clearing
    result = raijin_render(ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr,
            "--> ERROR: Failed to render empty frame: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Create cube mesh
    RaijinMesh cube_mesh = RAIJIN_MESH_INVALID;
    result = raijin_mesh_get_builtin(ctx, RAIJIN_BUILTIN_MESH_CUBE, &cube_mesh);
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
    result = raijin_mesh_get_builtin(ctx, UINT32_MAX, &invalid_output);
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
    result = raijin_submit_instances(ctx, RAIJIN_MESH_INVALID, NULL, 0);
    if (result != RAIJIN_ERROR_INVALID_HANDLE) {
        fprintf(
            stderr,
            "--> ERROR: Failed invalid submission arguments test: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_submit_instances(ctx, cube_mesh, NULL, 1);
    if (result != RAIJIN_ERROR_INVALID_ARGUMENT) {
        fprintf(
            stderr,
            "--> ERROR: Failed invalid submission arguments test: %u\n",
            (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_submit_instances(ctx, cube_mesh, NULL, 0);
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
    /*
     * Public mesh handles must be rejected by contexts other than their
     * original owner.
     */
    result = raijin_create(&desc, &other_ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: Failed to create second context\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_submit_instances(other_ctx, cube_mesh, &instance, 1);
    if (result != RAIJIN_ERROR_INVALID_HANDLE) {
        fprintf(stderr, "--> ERROR: Cross-context mesh was accepted\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    /*
     * A handle from a destroyed context must not become valid in a newly
     * created context.
     */
    RaijinMesh stale_mesh = RAIJIN_MESH_INVALID;
    result = raijin_mesh_get_builtin(
        other_ctx, RAIJIN_BUILTIN_MESH_CUBE, &stale_mesh
    );
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: Failed to get second-context mesh\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    raijin_destroy(other_ctx);
    other_ctx = NULL;

    result = raijin_create(&desc, &other_ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(stderr, "--> ERROR: Failed to create replacement context\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    result = raijin_submit_instances(other_ctx, stale_mesh, &instance, 1);
    if (result != RAIJIN_ERROR_INVALID_HANDLE) {
        fprintf(stderr, "--> ERROR: Stale mesh handle was accepted\n");
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    raijin_destroy(other_ctx);
    other_ctx = NULL;

    /* Invalid arguments test */
    result = raijin_submit_instances(ctx, UINT64_MAX, &instance, 1);
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
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    float scale = 1.0f;
    float color[4] = {0.0f, 1.0f, 0.0f, 1.0f};

    result = raijin_draw_cube(ctx, position, rotation, scale, color);
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

    result = raijin_submit_instances(ctx, cube_mesh, cubes, 2);
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

    result = raijin_submit_instances(
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

    result = raijin_render(ctx);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr, "--> ERROR: Failed to render frame: %u\n", (uint32_t)result
        );
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    // Read frame to buffer
    uint64_t pixel_capacity = 0;
    result = raijin_readback_size(ctx, &pixel_capacity);
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

    result = raijin_read_rgba8(ctx, pixel_buf, pixel_capacity);
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
    result = raijin_read_rgba8(ctx, pixel_buf, pixel_capacity - 1);
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
    raijin_destroy(ctx);
    raijin_destroy(other_ctx);
    return exit_status;
}
