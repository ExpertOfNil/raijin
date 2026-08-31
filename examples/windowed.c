#include "cglm/cglm.h"
#include "raijin.h"

static const vec2 BIT_POSITIONS[] = {
    {0, 22},   {0, 26},   {4, 22},  {4, 26},  {11, 26},  {15, 26},
    {15, 23},  {22, 0},   {26, 0},  {22, 4},  {26, 4},   {26, 11},
    {26, 15},  {23, 15},  {23, 11}, {11, 23}, {22, 7.5}, {7.5, 22},
    {26, 7.5}, {7.5, 26}, {15, 30}, {11, 30}, {4, 30},   {0, 30},
};

static void normalize_position(vec3 pos, float scale) {
    pos[0] = (0.5f - pos[0] / 30.0f) * scale;
    pos[2] = (0.5f - pos[2] / 30.0f) * scale;
}

AssemblyHandle create_code(Raijin* engine, uint32_t code_id) {
    AssemblyHandle code = Raijin_create_assembly(engine);
    vec3 targets[] = {
        // clang-format off
        {00.0f, 0.001f, 00.0f}, // PT_C
        {26.0f, 0.001f, 26.0f}, // PT_A
        {11.5f, 0.001f, 11.5f}, // PT_E
        {00.0f, 0.001f, 11.0f}, // PT_D
        {11.0f, 0.001f, 00.0f}, // PT_B
        {00.0f, 0.001f, 00.0f}, // bit
        {00.0f, 0.001f, 00.0f}, // bit
        {00.0f, 0.001f, 00.0f}, // bit
        // clang-format on
    };

    u32 selected[3];
    u32 count = 0;
    for (u32 bit = 1; bit <= 24; ++bit) {
        if ((code_id & (UINT32_C(1) << bit)) == 0) continue;
        selected[count++] = bit - 1;
    }

    for (u32 i = 0; i < 3; ++i) {
        const u32 index = selected[i];
        const u32 target_index = 5 + i;
        targets[target_index][0] = BIT_POSITIONS[index][0];
        targets[target_index][2] = BIT_POSITIONS[index][1];
    }

    Instance plane = {
        .color = {0.0f, 0.0f, 0.0f, 1.0f},
    };
    glm_mat4_identity(plane.model_matrix);
    Instance_set_position(&plane, (vec3){0.0f, 0.0f, 0.0f});
    Raijin_assembly_add_mesh(
        engine, code, engine->renderer.builtin.plane, plane
    );

    for (u32 i = 0; i < ARRAY_COUNT(targets); ++i) {
        Instance disc = {
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
        };
        glm_mat4_identity(disc.model_matrix);
        glm_scale_uni(disc.model_matrix, 0.05f);
        normalize_position(targets[i], 0.8);
        printf("Target: [%.3f, %.3f]\n", targets[i][0], targets[i][2]);
        Instance_set_position(&disc, targets[i]);
        Raijin_assembly_add_mesh(
            engine, code, engine->renderer.builtin.disc, disc
        );
    }

    return code;
}

MeshHandle register_triangle(Raijin* engine) {
    // Create a custom triangle
    Mesh triangle = {0};
    // Add 3 vertices
    Vertex va = {
        .position = {1.0f, 0.0f, 0.0f},
        .color = {1.0f, 0.0f, 0.0f},
        .normal = {1.0f, 0.0f, 0.0f}
    };
    Vertex vb = {
        .position = {0.0f, 1.0f, 0.0f},
        .color = {0.0f, 1.0f, 0.0f},
        .normal = {1.0f, 0.0f, 0.0f}
    };
    Vertex vc = {
        .position = {0.0f, 0.0f, 1.0f},
        .color = {0.0f, 0.0f, 1.0f},
        .normal = {1.0f, 0.0f, 0.0f}
    };
    vec3 vba;
    glm_vec3_sub(vb.position, va.position, vba);
    vec3 vca;
    glm_vec3_sub(vc.position, va.position, vca);
    vec3 vn;
    glm_vec3_cross(vba, vca, vn);
    glm_vec3_copy(vn, va.normal);
    glm_vec3_copy(vn, vb.normal);
    glm_vec3_copy(vn, vc.normal);
    VertexArray_push(&triangle.vertices, va);
    VertexArray_push(&triangle.vertices, vb);
    VertexArray_push(&triangle.vertices, vc);
    // ... add 2 more vertices
    // Add indices
    IndexArray_push(&triangle.indices, 0);
    IndexArray_push(&triangle.indices, 1);
    IndexArray_push(&triangle.indices, 2);
    // Optional: Add edge indices
    IndexArray_push(&triangle.edge_indices, 0);
    IndexArray_push(&triangle.edge_indices, 1);
    IndexArray_push(&triangle.edge_indices, 1);
    IndexArray_push(&triangle.edge_indices, 2);
    IndexArray_push(&triangle.edge_indices, 2);
    IndexArray_push(&triangle.edge_indices, 0);
    // Register with engine
    MeshHandle handle = Raijin_register_mesh(engine, &triangle);
    if (handle == INVALID_MESH_HANDLE) {
        Mesh_release_cpu_arrays(&triangle);
    } else {
        triangle.vertices = (VertexArray){0};
        triangle.indices = (IndexArray){0};
        triangle.edge_indices = (IndexArray){0};
    }
    return handle;
}

int main(void) {
    Raijin engine = {0};
    Raijin_init(&engine, "Raijin", 1280, 720, RENDER_MODE_WINDOWED);
    Instance cube_instances[] = {
        {.color = {1.0, 0.0, 0.0, 1.0}},
        {.color = {0.0, 1.0, 0.0, 1.0}},
        {.color = {0.0, 0.0, 1.0, 1.0}},
    };
    glm_mat4_identity(cube_instances[0].model_matrix);
    glm_translate(cube_instances[0].model_matrix, (vec3){4.0f, 0.0f, 0.0f});
    glm_scale_uni(cube_instances[0].model_matrix, 0.2f);

    glm_mat4_identity(cube_instances[1].model_matrix);
    glm_translate(cube_instances[1].model_matrix, (vec3){0.0f, 4.0f, 0.0f});
    glm_scale_uni(cube_instances[1].model_matrix, 0.2f);

    glm_mat4_identity(cube_instances[2].model_matrix);
    glm_translate(cube_instances[2].model_matrix, (vec3){0.0f, 0.0f, 4.0f});
    glm_scale_uni(cube_instances[2].model_matrix, 0.2f);

    Instance disc_instance = {
        .color = {1.0, 1.0, 1.0, 1.0},
    };
    glm_mat4_identity(disc_instance.model_matrix);
    glm_translate(disc_instance.model_matrix, (vec3){0.0f, 0.0f, 0.0f});
    glm_rotate_x(
        disc_instance.model_matrix, GLM_PI * 0.5f, disc_instance.model_matrix
    );
    glm_scale_uni(disc_instance.model_matrix, 0.5f);

    AssemblyHandle axis_assy = Raijin_create_axis(&engine, 4.0f, 0.05f, true);

    Instance global_axis = {
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
    };
    glm_mat4_identity(global_axis.model_matrix);

    vec4 rot_x, rot_y, rot_z;
    glm_quatv(rot_x, GLM_PI_4, GLM_XUP);
    glm_quatv(rot_y, GLM_PI_4, GLM_YUP);
    glm_quatv(rot_z, GLM_PI_4, GLM_ZUP);

    vec4 camera_rot;
    glm_quat_mul(rot_z, rot_y, camera_rot);
    glm_quat_mul(camera_rot, rot_x, camera_rot);

    vec3 camera_pos = {4.0f, 4.0f, 4.0f};
    f32 camera_scale = 0.5f;

    Instance camera_axis = {
        .color = {0.1f, 0.1f, 0.1f, 1.0f},
    };
    glm_mat4_identity(camera_axis.model_matrix);
    glm_translate(camera_axis.model_matrix, camera_pos);
    glm_quat_rotate(
        camera_axis.model_matrix, camera_rot, camera_axis.model_matrix
    );
    glm_scale_uni(camera_axis.model_matrix, camera_scale);

    AssemblyHandle code_assy = create_code(&engine, 4227074);
    Instance code = {
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
    };
    glm_mat4_identity(code.model_matrix);
    glm_translate(code.model_matrix, (vec3){1.0f, 0.0f, 1.0f});

    AssemblyHandle frustum = Raijin_create_frustum(
        &engine,
        engine.camera.proj_matrix,
        0.05f,
        (vec4){1.0f, 1.0f, 1.0f, 1.0f}
    );
    Instance camera_frustum = {
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
    };
    glm_mat4_identity(camera_frustum.model_matrix);
    glm_translate(camera_frustum.model_matrix, camera_pos);
    glm_quat_rotate(
        camera_frustum.model_matrix, camera_rot, camera_frustum.model_matrix
    );
    glm_scale_uni(camera_frustum.model_matrix, 0.5f);

    Instance ref_camera = {
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
    };
    glm_mat4_copy(global_axis.model_matrix, ref_camera.model_matrix);

    MeshHandle tri = register_triangle(&engine);
    Instance tri_instance = {.color = {1, 0, 0, 1}};
    glm_mat4_identity(tri_instance.model_matrix);

    PanOrbitCamera_orbit(
        &engine.camera, (vec2){-314.159f * 1.5f, 314.159f / 2.0f}
    );
    Renderer_update_uniforms(
        &engine.renderer, engine.camera.proj_matrix, engine.camera.view_matrix
    );

    while (!engine.window.should_close) {
        Raijin_handle_events(&engine);
        // Raijin_draw_mesh_instance(&engine, tri, tri_instance);
        // Raijin_draw_disc_instance(&engine, disc_instance);
        Raijin_draw_assembly_instance(&engine, axis_assy, global_axis);
        // Raijin_draw_assembly_instance(&engine, axis_assy, camera_axis);
        // Raijin_draw_assembly_instance(&engine, frustum, ref_camera);
        // Raijin_draw_assembly_instance(&engine, frustum, camera_frustum);
        Raijin_draw_assembly_instance(&engine, code_assy, code);
        Raijin_render(&engine);
    }

    Raijin_destroy(&engine);
}
