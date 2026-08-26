#ifndef RAIJIN_H
#define RAIJIN_H

#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

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
    AssemblyArray assemblies;
} Raijin;

/* Function prototypes */

ReturnStatus Raijin_init(
    Raijin*, const char* title, u32 width, u32 height, RenderMode mode
);
void Raijin_handle_events(Raijin* engine);
MeshHandle Raijin_register_mesh(Raijin* engine, Mesh* mesh);
AssemblyHandle Raijin_create_assembly(Raijin* engine);
AssemblyHandle Raijin_create_axis(
    Raijin* engine, f32 axis_length, f32 axis_radius, bool include_origin
);
void Raijin_assembly_add_mesh(
    Raijin* engine,
    AssemblyHandle assy_handle,
    MeshHandle mesh_handle,
    Instance instance
);
void Raijin_draw_mesh_instance(
    Raijin* engine, MeshHandle mesh_handle, Instance instance
);
void Raijin_draw_assembly_instance(
    Raijin* engine, AssemblyHandle assy_handle, Instance parent_instance
);
void Raijin_draw_mesh(
    Raijin* engine,
    MeshHandle mesh_handle,
    vec3 position,
    mat3 rotation,
    f32 scale,
    vec4 color
);
void Raijin_draw_assembly(
    Raijin* engine,
    AssemblyHandle assy_handle,
    float* position,
    vec3* rotation,
    f32 scale,
    float* color
);
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

ReturnStatus Raijin_copy_frame_to_buffer(
    Raijin* engine, u32 width, u32 height, u8* buffer, u64 buffer_capacity
);

ReturnStatus Raijin_render(Raijin* engine);

void Raijin_destroy(Raijin* engine);

/* Function */

ReturnStatus Raijin_init(
    Raijin* engine, const char* title, u32 width, u32 height, RenderMode mode
) {
    WGPUInstanceDescriptor instance_desc = {0};
    WGPUInstance instance = wgpuCreateInstance(&instance_desc);
    if (instance == NULL) {
        log_error("Failed to create WGPU instance");
        return RETURN_FAILURE;
    }

    if (mode == RENDER_MODE_WINDOWED) {
        engine->mouse = (MouseState){
            .button_left = false,
            .button_middle = false,
            .button_right = false,
            .position = {0.0f, 0.0f},
        };
        if (!SdlWindow_init(&engine->window, title, width, height)) {
            wgpuInstanceRelease(instance);
            return RETURN_FAILURE;
        }

        // Create platform-specific surface
        WGPUSurface surface =
            create_surface_sdl3(instance, engine->window.handle);
        if (surface == NULL) {
            log_error("Failed to create surface");
            SdlWindow_destroy(&engine->window);
            wgpuInstanceRelease(instance);
            return RETURN_FAILURE;
        }

        ReturnStatus status = Renderer_init_windowed(
            &engine->renderer, instance, surface, width, height
        );
        if (status != RETURN_SUCCESS) {
            Raijin_destroy(engine);
            return RETURN_FAILURE;
        }
    } else {
        ReturnStatus status =
            Renderer_init_headless(&engine->renderer, instance, width, height);
        if (status != RETURN_SUCCESS) {
            Raijin_destroy(engine);
            return RETURN_FAILURE;
        }
    }

    PanOrbitCamera_init(&engine->camera);

    if (width > 0 && height > 0) {
        engine->camera.aspect = (f32)width / (f32)height;
        glm_perspective_resize(
            engine->camera.aspect, engine->camera.proj_matrix
        );
    }

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

void Raijin_destroy(Raijin* engine) {
    for (u32 i = 0; i < engine->assemblies.count; ++i) {
        Assembly* assembly = &engine->assemblies.items[i];
        AssemblyComponentArray_free(&assembly->components);
        log_debug("Assy component %d free.", i);
    }

    AssemblyArray_free(&engine->assemblies);
    log_debug("Assy array free.");
    Renderer_destroy(&engine->renderer);
    log_debug("Renderer free.");

    if (engine->window.handle != NULL) {
        SdlWindow_destroy(&engine->window);
    }
}
// #endif

MeshHandle Raijin_register_mesh(Raijin* engine, Mesh* mesh) {
    return Renderer_register_mesh(&engine->renderer, mesh);
}

AssemblyHandle Raijin_create_assembly(Raijin* engine) {
    Assembly assembly = {0};
    AssemblyComponentArray_init(&assembly.components);
    AssemblyArray_push(&engine->assemblies, assembly);
    return engine->assemblies.count - 1;
}

AssemblyHandle Raijin_create_axis(
    Raijin* engine, f32 axis_length, f32 axis_radius, bool include_origin
) {
    AssemblyHandle axis_assy = Raijin_create_assembly(engine);
    f32 cone_r = axis_radius * 1.5f;
    f32 cone_end = axis_length + axis_length * 0.1f;
    f32 origin_r = axis_radius * 4.0f;
    if (include_origin) {
        Instance sphere_instance = {
            .color = {1.0, 0.0, 1.0, 1.0},
        };
        glm_mat4_identity(sphere_instance.model_matrix);
        glm_translate(sphere_instance.model_matrix, (vec3){0.0f, 0.0f, 0.0f});
        glm_scale_uni(sphere_instance.model_matrix, origin_r);
        Raijin_assembly_add_mesh(
            engine,
            axis_assy,
            engine->renderer.builtin.sphere_uv,
            sphere_instance
        );
    }

    Instance x_axis;
    Instance_from_line(
        &x_axis,
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){axis_length, 0.0f, 0.0f},
        axis_radius,
        (vec4){1.0f, 0.0f, 0.0f, 1.0f}
    );
    Raijin_assembly_add_mesh(
        engine, axis_assy, engine->renderer.builtin.cylinder, x_axis
    );
    Instance x_axis_tip;
    Instance_from_line(
        &x_axis_tip,
        (vec3){axis_length, 0.0f, 0.0f},
        (vec3){cone_end, 0.0f, 0.0f},
        cone_r,
        (vec4){1.0f, 0.0f, 0.0f, 1.0f}
    );
    Raijin_assembly_add_mesh(
        engine, axis_assy, engine->renderer.builtin.cone, x_axis_tip
    );
    Instance y_axis;
    Instance_from_line(
        &y_axis,
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, axis_length, 0.0f},
        axis_radius,
        (vec4){0.0f, 1.0f, 0.0f, 1.0f}
    );
    Raijin_assembly_add_mesh(
        engine, axis_assy, engine->renderer.builtin.cylinder, y_axis
    );
    Instance y_axis_tip;
    Instance_from_line(
        &y_axis_tip,
        (vec3){0.0f, axis_length, 0.0f},
        (vec3){0.0f, cone_end, 0.0f},
        cone_r,
        (vec4){0.0f, 1.0f, 0.0f, 1.0f}
    );
    Raijin_assembly_add_mesh(
        engine, axis_assy, engine->renderer.builtin.cone, y_axis_tip
    );
    Instance z_axis;
    Instance_from_line(
        &z_axis,
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, 0.0f, axis_length},
        axis_radius,
        (vec4){0.001f, 0.001f, 1.0f, 1.0f}
    );
    Raijin_assembly_add_mesh(
        engine, axis_assy, engine->renderer.builtin.cylinder, z_axis
    );
    Instance z_axis_tip;
    Instance_from_line(
        &z_axis_tip,
        (vec3){0.0f, 0.0f, axis_length},
        (vec3){0.0f, 0.0f, cone_end},
        cone_r,
        (vec4){0.001f, 0.001f, 1.0f, 1.0f}
    );
    Raijin_assembly_add_mesh(
        engine, axis_assy, engine->renderer.builtin.cone, z_axis_tip
    );

    return axis_assy;
}

AssemblyHandle Raijin_create_frustum(
    Raijin* engine, mat4 proj_matrix, float line_radius, vec4 color
) {
    AssemblyHandle frustum = Raijin_create_assembly(engine);
    mat4 inv_proj;
    glm_mat4_inv(proj_matrix, inv_proj);
    vec3 corners[4] = {
        // Far plane, ccw order starting with top-right
        // clang-format off
        {1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f},
        // clang-format on
    };
    for (u32 i = 0; i < 4; ++i) {
        vec4 clip_pos = {corners[i][0], corners[i][1], corners[i][2], 1.0f};
        vec4 world_pos;
        glm_mat4_mulv(inv_proj, clip_pos, world_pos);
        glm_vec4_scale(world_pos, 1.0f / world_pos[3], world_pos);
        glm_vec4_normalize(world_pos);
        glm_vec4_scale(world_pos, 4.0f, world_pos);
        glm_vec4_copy3(world_pos, corners[i]);
    }

    for (u32 i = 0; i < 4; ++i) {
        Instance line;
        Instance_from_line(
            &line, (vec3){0.0f, 0.0f, 0.0f}, corners[i], line_radius, color
        );
        Raijin_assembly_add_mesh(
            engine, frustum, engine->renderer.builtin.cylinder, line
        );
    }

    for (u32 i = 0; i < 4; ++i) {
        Instance line;
        u32 next_corner = (i + 1) % 4;
        Instance_from_line(
            &line, corners[i], corners[next_corner], line_radius, color
        );
        Raijin_assembly_add_mesh(
            engine, frustum, engine->renderer.builtin.cylinder, line
        );
    }

    return frustum;
}

void Raijin_assembly_add_mesh(
    Raijin* engine,
    AssemblyHandle assy_handle,
    MeshHandle mesh_handle,
    Instance instance
) {
    Assembly* assembly = &engine->assemblies.items[assy_handle];
    AssemblyComponent component = {
        .mesh_handle = mesh_handle,
        .instance = instance,
    };
    AssemblyComponentArray_push(&assembly->components, component);
}

ReturnStatus Raijin_render(Raijin* engine) {
    return Renderer_render(&engine->renderer);
}

void Raijin_draw_mesh_instance(
    Raijin* engine, MeshHandle mesh_handle, Instance instance
) {
    DrawCommand cmd = {
        .mesh_handle = mesh_handle,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
}

void Raijin_draw_mesh(
    Raijin* engine,
    MeshHandle mesh_handle,
    float* position,
    vec3* rotation,
    f32 scale,
    float* color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_scale_uni(instance.model_matrix, scale);
    Raijin_draw_mesh_instance(engine, mesh_handle, instance);
}

void Raijin_draw_assembly_instance(
    Raijin* engine, AssemblyHandle assy_handle, Instance parent_instance
) {
    Assembly* assembly = &engine->assemblies.items[assy_handle];
    for (u32 i = 0; i < assembly->components.count; ++i) {
        AssemblyComponent* comp = &assembly->components.items[i];
        Instance final_instance = {0};
        glm_mat4_mul(
            parent_instance.model_matrix,
            comp->instance.model_matrix,
            final_instance.model_matrix
        );
        glm_vec4_mul(
            comp->instance.color, parent_instance.color, final_instance.color
        );
        Raijin_draw_mesh_instance(engine, comp->mesh_handle, final_instance);
    }
}

void Raijin_draw_assembly(
    Raijin* engine,
    AssemblyHandle assy_handle,
    float* position,
    vec3* rotation,
    f32 scale,
    float* color
) {
    Instance instance = {.color = {color[0], color[1], color[2], color[3]}};
    glm_mat4_identity(instance.model_matrix);
    glm_mat4_ins3(rotation, instance.model_matrix);
    glm_translate(instance.model_matrix, position);
    glm_scale_uni(instance.model_matrix, scale);
    Raijin_draw_assembly_instance(engine, assy_handle, instance);
}

void Raijin_draw_cube_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_handle = engine->renderer.builtin.cube,
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
        .mesh_handle = engine->renderer.builtin.sphere_uv,
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

void Raijin_draw_plane_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_handle = engine->renderer.builtin.plane,
        .instance = instance,
    };
    DrawCommandArray_push(&engine->renderer.draw_commands, cmd);
}

void Raijin_draw_disc_instance(Raijin* engine, Instance instance) {
    DrawCommand cmd = {
        .mesh_handle = engine->renderer.builtin.disc,
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
        .mesh_handle = engine->renderer.builtin.cylinder,
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
        .mesh_handle = engine->renderer.builtin.cone,
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

ReturnStatus Raijin_copy_frame_to_buffer(
    Raijin* engine, u32 width, u32 height, u8* buffer, u64 buffer_capacity
) {
    return Renderer_copy_frame_to_buffer(
        &engine->renderer, width, height, buffer, buffer_capacity
    );
}

#endif /* RAIJIN_H */
