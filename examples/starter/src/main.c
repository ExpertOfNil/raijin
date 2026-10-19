#include <raijin/raijin.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define POINT_COUNT 5u

static int report_error(const char* operation, RaijinResult result)
{
    fprintf(
        stderr,
        "Raijin operation failed: %s (result %u)\n",
        operation,
        (unsigned int)result
    );

    return EXIT_FAILURE;
}

int main(void)
{
    /*
     * Four alternating cube corners form a tetrahedral pattern. The fifth
     * point is at the center of the cube.
     */
    static const float positions[POINT_COUNT][3] = {
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 0.0f,  0.0f,  0.0f},
    };

    static const float colors[POINT_COUNT][4] = {
        {1.0f, 0.15f, 0.15f, 1.0f},
        {0.15f, 1.0f, 0.15f, 1.0f},
        {0.15f, 0.35f, 1.0f, 1.0f},
        {1.0f, 0.75f, 0.15f, 1.0f},
        {1.0f, 0.15f, 1.0f, 1.0f},
    };

    static const float identity_rotation[9] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
    };

    static const float line_color[4] = {
        0.65f, 0.70f, 0.80f, 1.0f
    };

    static const float camera_eye[3] = {
        4.0f, 3.0f, 5.0f
    };

    static const float camera_target[3] = {
        0.0f, 0.0f, 0.0f
    };

    static const float camera_up[3] = {
        0.0f, 1.0f, 0.0f
    };

    const RaijinContextDesc context_desc = {
        .title = "Raijin Starter",
        .width = 1280,
        .height = 720,
        .render_mode = RAIJIN_RENDER_MODE_WINDOWED,
    };

    RaijinContext* ctx = NULL;
    RaijinMesh sphere_mesh = RAIJIN_MESH_INVALID;
    RaijinInstance spheres[POINT_COUNT];

    RaijinResult result;
    int exit_status = EXIT_SUCCESS;

    result = raijin_create(&context_desc, &ctx);
    if (result != RAIJIN_SUCCESS) {
        return report_error("create context", result);
    }

    result = raijin_camera_look_at(
        ctx,
        camera_eye,
        camera_target,
        camera_up
    );

    if (result != RAIJIN_SUCCESS) {
        exit_status = report_error("position camera", result);
        goto cleanup;
    }

    result = raijin_mesh_get_builtin(
        ctx,
        RAIJIN_BUILTIN_MESH_SPHERE_UV,
        &sphere_mesh
    );

    if (result != RAIJIN_SUCCESS) {
        exit_status = report_error("get builtin sphere", result);
        goto cleanup;
    }

    /*
     * Construct the sphere instances once. Camera movement does not change
     * their world-space transforms.
     */
    for (uint32_t i = 0; i < POINT_COUNT; ++i) {
        result = raijin_instance_init_trs(
            &spheres[i],
            positions[i],
            identity_rotation,
            0.20f,
            colors[i]
        );

        if (result != RAIJIN_SUCCESS) {
            exit_status = report_error("construct sphere instance", result);
            goto cleanup;
        }
    }

    puts(
        "Controls: left-drag orbit, right-drag pan, "
        "mouse wheel zoom, Escape quit"
    );

    uint32_t should_close = 0;

    while (!should_close) {
        result = raijin_poll_events(ctx);
        if (result != RAIJIN_SUCCESS) {
            exit_status = report_error("poll events", result);
            break;
        }

        result = raijin_should_close(ctx, &should_close);
        if (result != RAIJIN_SUCCESS) {
            exit_status = report_error("query close state", result);
            break;
        }

        if (should_close) {
            continue;
        }

        /*
         * High-level immediate-mode path: draw one cylinder between every
         * unique pair of points.
         */
        for (uint32_t i = 0;
             i < POINT_COUNT && result == RAIJIN_SUCCESS;
             ++i) {
            for (uint32_t j = i + 1; j < POINT_COUNT; ++j) {
                result = raijin_draw_line(
                    ctx,
                    positions[i],
                    positions[j],
                    0.025f,
                    line_color
                );

                if (result != RAIJIN_SUCCESS) {
                    break;
                }
            }
        }

        if (result != RAIJIN_SUCCESS) {
            exit_status = report_error("draw lines", result);
            break;
        }

        /*
         * Lower-level bulk path: submit every sphere with one call.
         *
         * Submissions are transient, so the unchanged instances must still
         * be submitted before every frame.
         */
        result = raijin_submit_instances(
            ctx,
            sphere_mesh,
            spheres,
            POINT_COUNT
        );

        if (result != RAIJIN_SUCCESS) {
            exit_status = report_error("submit spheres", result);
            break;
        }

        result = raijin_render(ctx);
        if (result != RAIJIN_SUCCESS) {
            exit_status = report_error("render", result);
            break;
        }
    }

cleanup:
    raijin_destroy(ctx);
    return exit_status;
}
