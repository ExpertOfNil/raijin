#define CIMPL_IMPLEMENTATION
#include "raijin.h"

int main(void) {
    Raijin engine = {0};
    Raijin_init(&engine, "Raijin", 1280, 720);
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

    while (!engine.window.should_close) {
        Raijin_handle_events(&engine);
        for (u32 i = 0; i < 3; ++i) {
            Raijin_draw_cube_instance(&engine, cube_instances[i]);
        }
        Raijin_render(&engine);
    }
}
