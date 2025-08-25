#ifndef RAIJIN_H
#define RAIJIN_H

#include <stdbool.h>

#include "cglm/cam.h"
#include "cglm/common.h"
#include "cglm/io.h"
#include "cglm/mat4.h"
#include "core.h"
#include "mesh.h"
#include "renderer.h"

// #ifdef RAIJIN_SDL3_IMPL
#include "raijin_sdl3.h"
#include "webgpu.h"

#define CGLM_CONFIG_CLIP_CONTROL CGLM_CLIP_CONTROL_RH_ZO

/* Types */

typedef struct Raijin {
    SdlWindow window;
    Renderer renderer;
} Raijin;

/* Function prototypes */

ReturnStatus Raijin_init(Raijin*, const char* title, u32 width, u32 height);
void Raijin_handle_events(Raijin* engine);
 void Raijin_draw_cube(
     Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Raijin_destroy(Raijin* engine);

/* Function */

ReturnStatus Raijin_init(
    Raijin* engine, const char* title, u32 width, u32 height
) {
    if (!SdlWindow_init(&engine->window, title, width, height)) {
        return RETURN_FAILURE;
    }
    WGPUInstanceDescriptor instance_desc = {0};
    WGPUInstance instance = wgpuCreateInstance(&instance_desc);
    if (instance == NULL) {
        LOG_ERROR("Failed to create WGPU instance");
        return false;
    }

    // Create platform-specific surface
    engine->renderer.render_target.windowed.surface =
        create_surface_sdl3(instance, engine->window.handle);
    if (engine->renderer.render_target.windowed.surface == NULL) {
        LOG_ERROR("Failed to create surface");
        return false;
    }

    ReturnStatus status =
        Renderer_init_windowed(&engine->renderer, instance, width, height);
    if (status != RETURN_SUCCESS) {
        Renderer_destroy(&engine->renderer);
        wgpuInstanceRelease(instance);
        Raijin_destroy(engine);
        return RETURN_FAILURE;
    }

    f32 aspect = (f32)width / (f32)height;
    mat4 proj_matrix = {0};
    glm_perspective(glm_rad(60.0f), aspect, 0.1, 1000.0, proj_matrix);
    mat4 view_matrix = {0};
    glm_lookat(
        (vec3){10.0f, 10.0f, 10.0f},
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, 0.0f, 1.0f},
        view_matrix
    );
    Renderer_update_uniforms(&engine->renderer, proj_matrix, view_matrix);
    return RETURN_SUCCESS;
}

void Raijin_handle_events(Raijin* engine) {
    SdlWindow_handle_events(&engine->window, &engine->renderer);
}

void Raijin_destroy(Raijin* engine) { SdlWindow_destroy(&engine->window); }
// #endif

ReturnStatus Raijin_render(Raijin* engine) {
    // Renderer_update_uniforms(&engine->renderer, proj_matrix, view_matrix);
    return Renderer_render(&engine->renderer);
}

void Raijin_draw_cube_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_type = MESH_TYPE_CUBE,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
    LOG_DEBUG(
        "Array Push command count: %ld", engine->renderer.draw_commands.count
    );
}

void Raijin_draw_cube(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_mat4_scale(instance.model_matrix, scale);
    Raijin_draw_cube_instance(engine, instance);
}

#endif /* RAIJIN_H */
