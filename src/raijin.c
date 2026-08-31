#include "../include/raijin.h"

#include <stdlib.h>
#include <string.h>

#include "raijin/raijin.h"

_Static_assert(
    sizeof(((RaijinInstance*)0)->transform) == sizeof(mat4),
    "Public and internal matrix sizes differ"
);

struct RaijinContext {
    Raijin engine;
    uint32_t width;
    uint32_t height;
    RaijinRenderMode render_mode;
};

static RaijinMesh raijin_mesh_from_internal(MeshHandle mesh) {
    return (RaijinMesh)mesh + UINT64_C(1);
}

static int raijin_mesh_to_internal(
    const RaijinContext* ctx, RaijinMesh mesh, MeshHandle* out
) {
    if (!ctx || !out || mesh == RAIJIN_MESH_INVALID) return -1;

    uint64_t index = mesh - UINT64_C(1);
    if (index >= ctx->engine.renderer.meshes.count) return -1;

    *out = (MeshHandle)index;
    return 0;
}

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
    if (!out_size_bytes) return RAIJIN_ERROR_INVALID_ARGUMENT;
    *out_size_bytes = 0;
    if (!ctx) return RAIJIN_ERROR_INVALID_ARGUMENT;
    if (ctx->render_mode != RAIJIN_RENDER_MODE_HEADLESS) {
        return RAIJIN_ERROR_INVALID_STATE;
    }

    uint64_t pixels = (uint64_t)ctx->width * (uint64_t)ctx->height;
    if (pixels > UINT64_MAX / UINT64_C(4)) return RAIJIN_ERROR_OUT_OF_MEMORY;
    *out_size_bytes = pixels * UINT64_C(4);

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

RaijinResult raijin_context_draw_cube(
    RaijinContext* ctx, const RaijinInstance* instance
) {
    if (!ctx || !instance) return RAIJIN_ERROR_INVALID_ARGUMENT;

    RaijinMesh cube;
    RaijinResult result =
        raijin_context_get_builtin_mesh(ctx, RAIJIN_BUILTIN_MESH_CUBE, &cube);
    if (result != RAIJIN_SUCCESS) {
        return result;
    }

    return raijin_context_submit_instances(ctx, cube, instance, 1);
}

void raijin_context_destroy(RaijinContext* ctx) {
    if (ctx == NULL) return;
    Raijin_destroy(&ctx->engine);
    free(ctx);
}

void raijin_instance_init(RaijinInstance* instance) {
    if (!instance) return;

    *instance = (RaijinInstance){0};
    instance->transform[0] = 1.0f;
    instance->transform[5] = 1.0f;
    instance->transform[10] = 1.0f;
    instance->transform[15] = 1.0f;

    instance->color[0] = 1.0f;
    instance->color[1] = 1.0f;
    instance->color[2] = 1.0f;
    instance->color[3] = 1.0f;
}

RaijinResult raijin_context_get_builtin_mesh(
    const RaijinContext* ctx, RaijinBuiltinMesh builtin, RaijinMesh* out
) {
    // As long as `out` is valid, it will be cleared
    if (!out) return RAIJIN_ERROR_INVALID_ARGUMENT;
    *out = RAIJIN_MESH_INVALID;

    if (!ctx) return RAIJIN_ERROR_INVALID_ARGUMENT;

    switch (builtin) {
        case RAIJIN_BUILTIN_MESH_CUBE:
            *out = raijin_mesh_from_internal(ctx->engine.renderer.builtin.cube);
            return RAIJIN_SUCCESS;
        default:
            return RAIJIN_ERROR_INVALID_ARGUMENT;
    }
}

RaijinResult raijin_context_submit_instances(
    RaijinContext* ctx,
    RaijinMesh mesh,
    const RaijinInstance* instances,
    uint32_t instance_count
) {
    MeshHandle internal_mesh;

    if (!ctx) return RAIJIN_ERROR_INVALID_ARGUMENT;
    if (raijin_mesh_to_internal(ctx, mesh, &internal_mesh) != 0) {
        return RAIJIN_ERROR_INVALID_HANDLE;
    }

    if (instance_count == 0) return RAIJIN_SUCCESS;
    if (!instances) return RAIJIN_ERROR_INVALID_ARGUMENT;

    DrawCommandArray* commands = &ctx->engine.renderer.draw_commands;

    if (instance_count > UINT32_MAX - commands->count) {
        return RAIJIN_ERROR_OUT_OF_MEMORY;
    }

    uint32_t required_count = commands->count + instance_count;
    if (DrawCommandArray_reserve(commands, required_count) != RETURN_OK) {
        return RAIJIN_ERROR_OUT_OF_MEMORY;
    }

    for (uint32_t i = 0; i < instance_count; ++i) {
        Instance internal_instance = {0};

        memcpy(
            internal_instance.model_matrix,
            instances[i].transform,
            sizeof(internal_instance.model_matrix)
        );

        memcpy(
            internal_instance.color,
            instances[i].color,
            sizeof(internal_instance.color)
        );

        commands->items[commands->count++] = (DrawCommand){
            .mesh_handle = internal_mesh,
            .instance = internal_instance,
        };
    }

    return RAIJIN_SUCCESS;
}
