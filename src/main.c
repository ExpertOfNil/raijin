#define CIMPL_IMPLEMENTATION
#include "cimpl_glm.h"
#include "raijin.h"

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
    // Clean up temp data (since registration copies it)
    VertexArray_free(&triangle.vertices);
    IndexArray_free(&triangle.indices);
    IndexArray_free(&triangle.edge_indices);
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
        disc_instance.model_matrix, PI * 0.5f, disc_instance.model_matrix
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
        Raijin_draw_mesh_instance(&engine, tri, tri_instance);
        Raijin_draw_disc_instance(&engine, disc_instance);
        Raijin_draw_assembly_instance(&engine, axis_assy, global_axis);
        Raijin_draw_assembly_instance(&engine, axis_assy, camera_axis);
        Raijin_draw_assembly_instance(&engine, frustum, ref_camera);
        Raijin_draw_assembly_instance(&engine, frustum, camera_frustum);
        Raijin_render(&engine);
    }

    Raijin_destroy(&engine);
}
