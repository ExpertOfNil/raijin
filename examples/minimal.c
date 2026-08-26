#include <raijin/raijin.h>
#include <stdio.h>
#include <stdlib.h>

static int write_pam_file(
    const uint8_t* pixel_buf,
    uint64_t pixel_capacity,
    uint32_t width,
    uint32_t height,
    const char* filename
) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "--> ERROR: Failed to open file: %s\n", filename);
        return -1;
    }

    // Write header
    int result = fprintf(
        fp,
        "P7\n"
        "WIDTH %u\n"
        "HEIGHT %u\n"
        "DEPTH 4\n"
        "MAXVAL 255\n"
        "TUPLTYPE RGB_ALPHA\n"
        "ENDHDR\n",
        width,
        height
    );
    if (result <= 0) {
        fprintf(
            stderr, "--> ERROR: Failed to write PAM header: %s\n", filename
        );
        fclose(fp);
        return -1;
    }

    size_t written = fwrite(pixel_buf, 1, pixel_capacity, fp);
    if ((uint64_t)written != pixel_capacity) {
        fprintf(stderr, "--> ERROR: Failed to write PAM body: %s\n", filename);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

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

    // Create cube instance
    RaijinInstance cube;
    raijin_instance_init(&cube);

    cube.color[0] = 0.0f;
    cube.color[1] = 1.0f;
    cube.color[2] = 0.0f;
    cube.color[3] = 1.0f;

    result = raijin_context_draw_cube(ctx, &cube);
    if (result != RAIJIN_SUCCESS) {
        fprintf(
            stderr, "--> ERROR: Failed to draw cube: %u\n", (uint32_t)result
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

    const char* fname = "frame.pam";
    if (write_pam_file(pixel_buf, pixel_capacity, desc.width, desc.height, fname) != 0) {
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

cleanup:
    if (pixel_buf) free(pixel_buf);
    raijin_context_destroy(ctx);
    return exit_status;
}
