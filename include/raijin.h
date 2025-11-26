#ifndef RAIJIN_H
#define RAIJIN_H

#include <stdbool.h>

#include "camera.h"
#include "cglm/mat4.h"
#include "cimpl_core.h"
#include "core.h"
#include "mesh.h"
#include "renderer.h"
#include "webgpu.h"

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE

// #ifdef RAIJIN_SDL3_IMPL
#include "raijin_sdl3.h"

/* Types */

typedef struct Raijin {
    SdlWindow window;
    Renderer renderer;
    PanOrbitCamera camera;
    MouseState mouse;
} Raijin;

/* Function prototypes */

ReturnStatus Raijin_init(Raijin*, const char* title, u32 width, u32 height);
void Raijin_handle_events(Raijin* engine);
void Raijin_draw_cube(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Raijin_draw_sphere_uv_instance(Raijin* engine, Instance instance);
void Raijin_draw_sphere_uv(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Raijin_draw_disc_instance(Raijin* engine, Instance instance);
void Raijin_draw_disc(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Raijin_draw_cylinder_instance(Raijin* engine, Instance instance);
void Raijin_draw_cylinder(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Raijin_draw_cone_instance(Raijin* engine, Instance instance);
void Raijin_draw_cone(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Raijin_destroy(Raijin* engine);

/* Function */

ReturnStatus Raijin_init(
    Raijin* engine, const char* title, u32 width, u32 height
) {
    engine->mouse = (MouseState){
        .button_left = false,
        .button_middle = false,
        .button_right = false,
        .position = {0.0f, 0.0f},
    };
    if (!SdlWindow_init(&engine->window, title, width, height)) {
        return RETURN_FAILURE;
    }
    WGPUInstanceDescriptor instance_desc = {0};
    WGPUInstance instance = wgpuCreateInstance(&instance_desc);
    if (instance == NULL) {
        log_error("Failed to create WGPU instance");
        return false;
    }

    // Create platform-specific surface
    engine->renderer.render_target.windowed.surface =
        create_surface_sdl3(instance, engine->window.handle);
    if (engine->renderer.render_target.windowed.surface == NULL) {
        log_error("Failed to create surface");
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

    PanOrbitCamera_init(&engine->camera);
    Renderer_update_uniforms(
        &engine->renderer,
        engine->camera.proj_matrix,
        engine->camera.view_matrix
    );
    return RETURN_SUCCESS;
}

void Raijin_handle_events(Raijin* engine) {
    SdlWindow_handle_events(
        &engine->window, &engine->renderer, &engine->mouse, &engine->camera
    );
}

void Raijin_destroy(Raijin* engine) { SdlWindow_destroy(&engine->window); }
// #endif

ReturnStatus Raijin_render(Raijin* engine) {
    return Renderer_render(&engine->renderer);
}

void Raijin_draw_cube_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_type = MESH_TYPE_CUBE,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
}

void Raijin_draw_cube(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_scale_uni(instance.model_matrix, scale);
    Raijin_draw_cube_instance(engine, instance);
}

void Raijin_draw_sphere_uv_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_type = MESH_TYPE_SPHERE,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
}

void Raijin_draw_sphere_uv(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_scale_uni(instance.model_matrix, scale);
    Raijin_draw_sphere_uv_instance(engine, instance);
}

void Raijin_draw_disc_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_type = MESH_TYPE_DISC,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
}

void Raijin_draw_disc(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_scale_uni(instance.model_matrix, scale);
    Raijin_draw_disc_instance(engine, instance);
}

void Raijin_draw_cylinder_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_type = MESH_TYPE_CYLINDER,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
}

void Raijin_draw_cylinder(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_scale_uni(instance.model_matrix, scale);
    Raijin_draw_cylinder_instance(engine, instance);
}

void Raijin_draw_cone_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_type = MESH_TYPE_CONE,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
}

void Raijin_draw_cone(
    Raijin* engine, vec3 position, mat3 rotation, f32 scale, vec4 color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_scale_uni(instance.model_matrix, scale);
    Raijin_draw_cone_instance(engine, instance);
}

#endif /* RAIJIN_H */
