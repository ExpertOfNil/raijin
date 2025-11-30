#define CIMPL_IMPLEMENTATION
#include "cimpl_glm.h"
#include "mesh.h"
#include "raijin.h"
#include "cglm/mat4.h"
#include "renderer.h"

int main(void) {
    Raijin engine = {0};
    const u32 width = 1280;
    const u32 height = 720;
    Raijin_init(&engine, "Raijin", width, height, RENDER_MODE_HEADLESS);
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

    Instance sphere_instance = {
        .color = {1.0, 0.0, 1.0, 1.0},
    };
    glm_mat4_identity(sphere_instance.model_matrix);
    glm_translate(sphere_instance.model_matrix, (vec3){0.0f, 0.0f, 0.0f});
    glm_scale_uni(sphere_instance.model_matrix, 0.2f);

    Instance disc_instance = {
        .color = {1.0, 1.0, 1.0, 1.0},
    };
    glm_mat4_identity(disc_instance.model_matrix);
    glm_translate(disc_instance.model_matrix, (vec3){0.0f, 0.0f, 0.0f});
    glm_rotate_x(
        disc_instance.model_matrix, PI * 0.5f, disc_instance.model_matrix
    );
    glm_scale_uni(disc_instance.model_matrix, 0.5f);

    f32 line_r = 0.05f;
    f32 cone_r = line_r * 1.5f;
    Instance x_axis;
    Instance_from_line(
        &x_axis,
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){4.0f, 0.0f, 0.0f},
        line_r,
        (vec4){1.0f, 0.0f, 0.0f, 1.0f}
    );
    Instance x_axis_tip;
    Instance_from_line(
        &x_axis_tip,
        (vec3){4.0f, 0.0f, 0.0f},
        (vec3){4.5f, 0.0f, 0.0f},
        cone_r,
        (vec4){1.0f, 0.0f, 0.0f, 1.0f}
    );
    Instance y_axis;
    Instance_from_line(
        &y_axis,
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, 4.0f, 0.0f},
        line_r,
        (vec4){0.0f, 1.0f, 0.0f, 1.0f}
    );
    Instance y_axis_tip;
    Instance_from_line(
        &y_axis_tip,
        (vec3){0.0f, 4.0f, 0.0f},
        (vec3){0.0f, 4.5f, 0.0f},
        cone_r,
        (vec4){0.0f, 1.0f, 0.0f, 1.0f}
    );
    Instance z_axis;
    Instance_from_line(
        &z_axis,
        (vec3){0.0f, 0.0f, 0.0f},
        (vec3){0.0f, 0.0f, 4.0f},
        line_r,
        (vec4){0.0f, 0.0f, 1.0f, 1.0f}
    );
    Instance z_axis_tip;
    Instance_from_line(
        &z_axis_tip,
        (vec3){0.0f, 0.0f, 4.0f},
        (vec3){0.0f, 0.0f, 4.5f},
        cone_r,
        (vec4){0.0f, 0.0f, 1.0f, 1.0f}
    );

    u32 buffer_size = width * height * 4;
    u8* output_buffer = malloc(buffer_size);

    char output_fname[256] = {0};

    f32 theta = 314.159f / 2.0f;
    PanOrbitCamera_orbit(&engine.camera, (vec2){-314.159f * 1.5f, theta});
    Renderer_update_uniforms(
        &engine.renderer, engine.camera.proj_matrix, engine.camera.view_matrix
    );
    f32 theta_delta = 314.159f / 20.0f;

    u32 frame_number = 0;
    while (!engine.window.should_close && frame_number < 10) {
        Raijin_draw_disc_instance(&engine, disc_instance);
        Raijin_draw_sphere_uv_instance(&engine, sphere_instance);
        Raijin_draw_cylinder_instance(&engine, x_axis);
        Raijin_draw_cone_instance(&engine, x_axis_tip);
        Raijin_draw_cylinder_instance(&engine, y_axis);
        Raijin_draw_cone_instance(&engine, y_axis_tip);
        Raijin_draw_cylinder_instance(&engine, z_axis);
        Raijin_draw_cone_instance(&engine, z_axis_tip);
        // for (u32 i = 0; i < 3; ++i) {
        //     Raijin_draw_cube_instance(&engine, cube_instances[i]);
        // }
        Raijin_render(&engine);
        Raijin_copy_frame_to_buffer(
            &engine, width, height, output_buffer, buffer_size
        );
        snprintf(
            output_fname, sizeof(output_fname), "Frame_%d.ppm", frame_number
        );
        FILE* fp = fopen(output_fname, "w");
        fprintf(fp, "P6\n%d %d\n255\n", width, height);
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                u32 index = (y * width + x) * 4;
                u8 r = output_buffer[index + 0];
                u8 g = output_buffer[index + 1];
                u8 b = output_buffer[index + 2];
                fputc(r, fp);
                fputc(g, fp);
                fputc(b, fp);
            }
        }
        // u32 bytes_written = fwrite(output_buffer, 1, buffer_size, fp);
        // if (bytes_written <= 0) {
        //     log_error("Failed to write PPM file bytes");
        //     return RETURN_FAILURE;
        // }
        fclose(fp);
        frame_number++;
        theta += theta_delta;
        PanOrbitCamera_orbit(&engine.camera, (vec2){theta_delta, 0.0f});
        Renderer_update_uniforms(
            &engine.renderer, engine.camera.proj_matrix, engine.camera.view_matrix
        );
    }
    Raijin_destroy(&engine);
}
