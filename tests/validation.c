#include <raijin/raijin.h>
#include <stdlib.h>

#define CHECK(expression)        \
    do {                         \
        if (!(expression)) {     \
            return EXIT_FAILURE; \
        }                        \
    } while (0)

int main(void) {
    RaijinContext* ctx = NULL;
    RaijinContextDesc desc = {
        .title = "Raijin Test",
        .width = 65,
        .height = 33,
        .render_mode = RAIJIN_RENDER_MODE_HEADLESS,
    };
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    float scale = 1.0f;
    float color[4] = {0.0f, 1.0f, 0.0f, 1.0f};

    CHECK(raijin_create(NULL, &ctx) == RAIJIN_ERROR_INVALID_ARGUMENT);

    CHECK(raijin_create(&desc, NULL) == RAIJIN_ERROR_INVALID_ARGUMENT);

    CHECK(raijin_render(NULL) == RAIJIN_ERROR_INVALID_ARGUMENT);

    CHECK(
        raijin_draw_cube(NULL, position, rotation, scale, color) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );

    CHECK(raijin_readback_size(NULL, NULL) == RAIJIN_ERROR_INVALID_ARGUMENT);

    raijin_instance_init(NULL);
    raijin_destroy(NULL);

    RaijinInstance instance;
    CHECK(
        raijin_instance_init_trs(NULL, position, rotation, scale, color) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );
    CHECK(
        raijin_instance_init_trs(&instance, NULL, rotation, scale, color) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );
    CHECK(
        raijin_instance_init_trs(&instance, position, NULL, scale, color) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );
    CHECK(
        raijin_instance_init_trs(&instance, position, rotation, scale, NULL) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );
    raijin_instance_init(&instance);

    RaijinMesh mesh = UINT64_MAX;

    CHECK(
        raijin_mesh_get_builtin(NULL, RAIJIN_BUILTIN_MESH_CUBE, &mesh) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );

    CHECK(mesh == RAIJIN_MESH_INVALID);

    CHECK(
        raijin_mesh_get_builtin(NULL, RAIJIN_BUILTIN_MESH_CUBE, NULL) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        raijin_submit_instances(NULL, RAIJIN_MESH_INVALID, &instance, 1) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );

    return EXIT_SUCCESS;
}
