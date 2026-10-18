#include "../include/raijin.h"

#include <math.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"
#include "mesh.h"
#include "raijin/raijin.h"
#include "renderer.h"

_Static_assert(
    sizeof(((RaijinInstance*)0)->transform) == sizeof(mat4),
    "Public and internal matrix sizes differ"
);
_Static_assert(
    sizeof(RaijinVertex) == sizeof(Vertex),
    "Public and internal vertex sizes differ"
);
_Static_assert(
    _Alignof(RaijinVertex) == _Alignof(Vertex),
    "Public and internal vertex alignments differ"
);
_Static_assert(
    offsetof(RaijinVertex, position) == offsetof(Vertex, position),
    "Public and internal position offsets differ"
);
_Static_assert(
    offsetof(RaijinVertex, color) == offsetof(Vertex, color),
    "Public and internal vertex color offsets differ"
);
_Static_assert(
    offsetof(RaijinVertex, normal) == offsetof(Vertex, normal),
    "Public and internal vertex normal offsets differ"
);

struct RaijinContext {
    Raijin engine;
    uint32_t width;
    uint32_t height;
    RaijinRenderMode render_mode;
    uint32_t id;
};

static atomic_uint_fast64_t raijin_next_context_id =
    ATOMIC_VAR_INIT(UINT64_C(1));

static bool raijin_acquire_context_id(uint32_t* out_id) {
    if (out_id == NULL) return false;

    uint_fast64_t current =
        atomic_load_explicit(&raijin_next_context_id, memory_order_relaxed);

    for (;;) {
        if (current == 0 || current > UINT32_MAX) return false;

        if (atomic_compare_exchange_weak_explicit(
                &raijin_next_context_id,
                &current,
                current + UINT64_C(1),
                memory_order_relaxed,
                memory_order_relaxed
            )) {
            *out_id = (uint32_t)current;
            return true;
        }
    }
}

static RaijinMesh raijin_mesh_from_internal(
    const RaijinContext* ctx, MeshHandle mesh
) {
    if (ctx == NULL || ctx->id == 0 || mesh == INVALID_MESH_HANDLE) {
        return RAIJIN_MESH_INVALID;
    }

    uint64_t encoded_index = (uint64_t)mesh + UINT64_C(1);
    return ((uint64_t)ctx->id << 32) | encoded_index;
}

static int raijin_mesh_to_internal(
    const RaijinContext* ctx, RaijinMesh mesh, MeshHandle* out
) {
    if (ctx == NULL || out == NULL || mesh == RAIJIN_MESH_INVALID) return -1;

    uint32_t context_id = (uint32_t)(mesh >> 32);
    uint32_t encoded_index = (uint32_t)mesh;

    if (context_id == 0 || context_id != ctx->id || encoded_index == 0) {
        return -1;
    }

    uint32_t index = encoded_index - UINT32_C(1);
    if (index >= ctx->engine.renderer.meshes.count) return -1;

    *out = (MeshHandle)index;
    return 0;
}

static RaijinResult raijin_result_from_status(
    ReturnStatus status, RaijinResult fallback
) {
    switch (status) {
        case RETURN_SUCCESS:
            return RAIJIN_SUCCESS;
        case RETURN_TIMEOUT:
            return RAIJIN_ERROR_TIMEOUT;
        case RETURN_DEVICE_LOST:
            return RAIJIN_ERROR_DEVICE_LOST;
        case RETURN_FAILURE:
        default:
            return fallback;
    }
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

    if (!raijin_acquire_context_id(&ctx->id)) {
        free(ctx);
        return RAIJIN_ERROR_OUT_OF_MEMORY;
    }

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
        return raijin_result_from_status(result, RAIJIN_ERROR_INITIALIZATION);
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

    ReturnStatus status = Raijin_render(&ctx->engine);
    return raijin_result_from_status(status, RAIJIN_ERROR_RENDER);
}

RaijinResult raijin_context_poll_events(RaijinContext* ctx) {
    if (ctx == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;
    if (ctx->render_mode != RAIJIN_RENDER_MODE_WINDOWED) {
        return RAIJIN_ERROR_INVALID_STATE;
    }

    Raijin_handle_events(&ctx->engine);

    ctx->width = ctx->engine.window.width;
    ctx->height = ctx->engine.window.height;

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_get_viewport_size(
    const RaijinContext* ctx, uint32_t* out_width, uint32_t* out_height
) {
    // Headless contexts return fixed render dimensions
    if (out_width != NULL) *out_width = 0;
    if (out_height != NULL) *out_height = 0;

    if (ctx == NULL || out_width == NULL || out_height == NULL) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    *out_width = ctx->width;
    *out_height = ctx->height;
    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_is_minimized(
    const RaijinContext* ctx, uint32_t* out_is_minimized
) {
    if (out_is_minimized == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;
    *out_is_minimized = 0;

    if (ctx == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;
    if (ctx->render_mode != RAIJIN_RENDER_MODE_WINDOWED) {
        return RAIJIN_ERROR_INVALID_STATE;
    }

    *out_is_minimized =
        ctx->engine.window.minimized ? UINT32_C(1) : UINT32_C(0);
    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_camera_look_to(
    RaijinContext* ctx,
    const float* eye,
    const float* direction,
    const float* up,
    float orbit_distance
) {
    if (ctx == NULL || eye == NULL || direction == NULL || up == NULL) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    if (!PanOrbitCamera_look_to(
            &ctx->engine.camera, eye, direction, up, orbit_distance
        )) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    Renderer_update_uniforms(
        &ctx->engine.renderer,
        ctx->engine.camera.proj_matrix,
        ctx->engine.camera.view_matrix
    );

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_camera_set_view_matrix(
    RaijinContext* ctx, const float view_matrix[16]
) {
    if (ctx == NULL || view_matrix == NULL) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    if (!PanOrbitCamera_set_view_matrix(&ctx->engine.camera, view_matrix)) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    Renderer_update_uniforms(
        &ctx->engine.renderer,
        ctx->engine.camera.proj_matrix,
        ctx->engine.camera.view_matrix
    );

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_camera_look_at(
    RaijinContext* ctx, const float* eye, const float* target, const float* up
) {
    if (ctx == NULL || eye == NULL || target == NULL || up == NULL) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    if (!PanOrbitCamera_look_at(&ctx->engine.camera, eye, target, up)) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    Renderer_update_uniforms(
        &ctx->engine.renderer,
        ctx->engine.camera.proj_matrix,
        ctx->engine.camera.view_matrix
    );

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_camera_set_projection_matrix(
    RaijinContext* ctx, const float* proj_matrix
) {
    if (ctx == NULL || proj_matrix == NULL) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < 16; ++i) {
        if (!isfinite(proj_matrix[i])) return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    memcpy(
        ctx->engine.camera.proj_matrix,
        proj_matrix,
        sizeof(ctx->engine.camera.proj_matrix)
    );

    Renderer_update_uniforms(
        &ctx->engine.renderer,
        ctx->engine.camera.proj_matrix,
        ctx->engine.camera.view_matrix
    );

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_set_edge_visibility(
    RaijinContext* ctx, uint32_t is_visible
) {
    if (ctx == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;

    ctx->engine.renderer.enable_edges = is_visible != 0;
    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_get_edge_visibility(
    const RaijinContext* ctx, uint32_t* out_is_visible
) {
    if (out_is_visible == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;
    *out_is_visible = 0;

    if (ctx == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;

    *out_is_visible =
        ctx->engine.renderer.enable_edges ? UINT32_C(1) : UINT32_C(0);
    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_set_clear_color(
    RaijinContext* ctx, const float linear_rgba[4]
) {
    if (ctx == NULL || linear_rgba == NULL) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < 4; ++i) {
        if (!isfinite(linear_rgba[i]) || linear_rgba[i] < 0.0f ||
            linear_rgba[i] > 1.0f) {
            return RAIJIN_ERROR_INVALID_ARGUMENT;
        }
    }

    memcpy(
        ctx->engine.renderer.clear_color,
        linear_rgba,
        sizeof(ctx->engine.renderer.clear_color)
    );
    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_get_clear_color(
    const RaijinContext* ctx, float out_linear_rgba[4]
) {
    if (out_linear_rgba == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;
    memset(out_linear_rgba, 0, sizeof(float) * 4);

    if (ctx == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;

    memcpy(
        out_linear_rgba,
        ctx->engine.renderer.clear_color,
        sizeof(ctx->engine.renderer.clear_color)
    );
    return RAIJIN_SUCCESS;
}

RaijinResult raijin_context_should_close(
    const RaijinContext* ctx, uint32_t* out_should_close
) {
    if (out_should_close == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;
    *out_should_close = 0;

    if (ctx == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;
    if (ctx->render_mode != RAIJIN_RENDER_MODE_WINDOWED) {
        return RAIJIN_ERROR_INVALID_STATE;
    }

    *out_should_close =
        ctx->engine.window.should_close ? UINT32_C(1) : UINT32_C(0);
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

    ReturnStatus readback_status = Raijin_copy_frame_to_buffer(
        &ctx->engine, ctx->width, ctx->height, pixel_buf, pixel_capacity
    );
    if (readback_status != RETURN_SUCCESS) {
        return raijin_result_from_status(
            readback_status, RAIJIN_ERROR_READBACK
        );
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
            *out = raijin_mesh_from_internal(
                ctx, ctx->engine.renderer.builtin.cube
            );
            return RAIJIN_SUCCESS;
        default:
            return RAIJIN_ERROR_INVALID_ARGUMENT;
    }
}

RaijinResult raijin_context_create_mesh(
    RaijinContext* ctx, const RaijinMeshDesc* desc, RaijinMesh* out_mesh
) {
    if (out_mesh == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;

    *out_mesh = RAIJIN_MESH_INVALID;

    if (ctx == NULL || desc == NULL) return RAIJIN_ERROR_INVALID_ARGUMENT;

    MeshGeometryView geometry = {
        .vertices = (const Vertex*)desc->vertices,
        .vertex_count = desc->vertex_count,
        .indices = desc->triangle_indices,
        .index_count = desc->triangle_index_count,
        .edge_indices = desc->edge_indices,
        .edge_index_count = desc->edge_index_count,
    };

    if (MeshGeometryView_validate(&geometry) != RETURN_SUCCESS) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    MeshHandle internal_mesh =
        Renderer_register_mesh(&ctx->engine.renderer, &geometry);
    if (internal_mesh == INVALID_MESH_HANDLE) {
        return RAIJIN_ERROR_MESH_CREATION;
    }

    RaijinMesh public_mesh = raijin_mesh_from_internal(ctx, internal_mesh);
    if (public_mesh == RAIJIN_MESH_INVALID) {
        /*
         * This should be unreachable because the context ID and internal
         * handle were already validated.
         */
        return RAIJIN_ERROR_MESH_CREATION;
    }

    *out_mesh = public_mesh;
    return RAIJIN_SUCCESS;
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

    Renderer* renderer = &ctx->engine.renderer;
    Mesh* internal = &renderer->meshes.items[internal_mesh];
    InstanceArray* dest = &internal->submitted_instances;
    bool activate = dest->count == 0;

    if (instance_count > UINT32_MAX - dest->count) {
        return RAIJIN_ERROR_OUT_OF_MEMORY;
    }
    uint32_t required_count = dest->count + instance_count;

    if (activate) {
        if (renderer->active_meshes.count == UINT32_MAX ||
            MeshHandleArray_reserve(
                &renderer->active_meshes,
                (size_t)renderer->active_meshes.count + 1
            ) != RETURN_OK) {
            return RAIJIN_ERROR_OUT_OF_MEMORY;
        }
    }

    if (InstanceArray_reserve(dest, required_count) != RETURN_OK) {
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

        dest->items[dest->count++] = internal_instance;
    }

    if (activate) {
        renderer->active_meshes.items[renderer->active_meshes.count++] =
            internal_mesh;
    }
    return RAIJIN_SUCCESS;
}
