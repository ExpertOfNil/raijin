#define CIMPL_IMPLEMENTATION

#include "../include/raijin.h"

#include <stdlib.h>

#include "raijin/raijin.h"

struct RaijinContext {
    Raijin engine;
    uint32_t width;
    uint32_t height;
    RaijinRenderMode render_mode;
};

RaijinResult raijin_context_create(
    const RaijinContextDesc* desc, RaijinContext** out
) {
    RaijinContext* ctx;
    if (!out) return RAIJIN_ERROR_INVALID_ARGUMENT;
    *out = NULL;
    if (!desc || desc->width == 0 || desc->height == 0)
        return RAIJIN_ERROR_INVALID_ARGUMENT;

    ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return RAIJIN_ERROR_OUT_OF_MEMORY;

    RenderMode render_mode = RENDER_MODE_HEADLESS;
    switch (desc->render_mode) {
        case RAIJIN_RENDER_MODE_HEADLESS: {
            render_mode = RENDER_MODE_HEADLESS;
        } break;
        case RAIJIN_RENDER_MODE_WINDOWED: {
            render_mode = RENDER_MODE_WINDOWED;
        } break;
        default:
            free(ctx);
            return RAIJIN_ERROR_INVALID_ARGUMENT;
    }
    const char* title = "Raijin";
    if (desc->title) {
        title = desc->title;
    }

    ReturnStatus result = Raijin_init(
        &ctx->engine, title, desc->width, desc->height, render_mode
    );
    if (result != RETURN_SUCCESS) {
        free(ctx);
        return RAIJIN_ERROR_INITIALIZATION;
    }
    ctx->height = desc->height;
    ctx->width = desc->width;
    ctx->render_mode = desc->render_mode;

    *out = ctx;
    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_render(RaijinContext* ctx) {
    if (!ctx) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    if (Raijin_render(&ctx->engine) != RETURN_SUCCESS) {
        return RAIJIN_ERROR_RENDER;
    }

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_readback_size(
    const RaijinContext* ctx, uint64_t* out_size_bytes
) {
    if (!ctx || !out_size_bytes) return RAIJIN_ERROR_INVALID_ARGUMENT;
    if (ctx->render_mode != RAIJIN_RENDER_MODE_HEADLESS) {
        return RAIJIN_ERROR_INVALID_STATE;
    }

    *out_size_bytes =
        (uint64_t)ctx->width * (uint64_t)ctx->height * UINT64_C(4);

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_read_rgba8(
    RaijinContext* ctx, uint8_t* pixel_buf, uint64_t pixel_capacity
) {
    uint64_t required_capacity;

    if (!ctx || !pixel_buf) return RAIJIN_ERROR_INVALID_ARGUMENT;

    RaijinResult result = raijin_context_readback_size(ctx, &required_capacity);
    if (result != RAIJIN_SUCCESS) return result;

    if (pixel_capacity < required_capacity)
        return RAIJIN_ERROR_BUFFER_TOO_SMALL;

    if (Raijin_copy_frame_to_buffer(
            &ctx->engine, ctx->width, ctx->height, pixel_buf, pixel_capacity
        ) != RETURN_SUCCESS) {
        return RAIJIN_ERROR_READBACK;
    }

    return RAIJIN_SUCCESS;
}

void raijin_context_destroy(RaijinContext* ctx) {
    if (ctx == NULL) return;
    Raijin_destroy(&ctx->engine);
    free(ctx);
}
