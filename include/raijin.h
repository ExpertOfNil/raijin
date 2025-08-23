#ifndef RAIJIN_H
#define RAIJIN_H

#include <stdbool.h>

#include "cglm/mat4.h"
#include "core.h"
#include "mesh.h"
#include "renderer.h"

// #ifdef RAIJIN_SDL3_IMPL
#include "raijin_sdl3.h"

typedef struct Raijin {
    SdlWindow window;
    Renderer renderer;
} Raijin;

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
    return Renderer_init_windowed(&engine->renderer, instance, width, height);
}

void Raijin_handle_events(Raijin* engine) {
    SdlWindow_handle_events(&engine->window, &engine->renderer);
}

void Raijin_destroy(Raijin* engine) { SdlWindow_destroy(&engine->window); }
// #endif

ReturnStatus Raijin_render(Raijin* engine) {
    return Renderer_render(&engine->renderer);
}

void Raijin_draw_cube(Raijin* engine) {
    DrawCommand cmd = {
        .mesh_type = MESH_TYPE_CUBE,
        .instance = (Instance){
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
        },
    };
    glm_mat4_identity(cmd.instance.model_matrix);
    glm_mat4_scale(cmd.instance.model_matrix, 10.0f);
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
    LOG_DEBUG(
        "Array Push command count: %ld", engine->renderer.draw_commands.count
    );
}

#endif /* RAIJIN_H */
