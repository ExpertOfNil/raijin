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

    CHECK(raijin_context_create(NULL, &ctx) == RAIJIN_ERROR_INVALID_ARGUMENT);

    CHECK(raijin_context_create(&desc, NULL) == RAIJIN_ERROR_INVALID_ARGUMENT);

    CHECK(raijin_context_render(NULL) == RAIJIN_ERROR_INVALID_ARGUMENT);

    CHECK(
        raijin_context_draw_cube(NULL, NULL) == RAIJIN_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        raijin_context_readback_size(NULL, NULL) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );

    raijin_instance_init(NULL);
    raijin_context_destroy(NULL);

    RaijinInstance instance;
    raijin_instance_init(&instance);

    RaijinMesh mesh = UINT64_MAX;

    CHECK(
        raijin_context_get_builtin_mesh(
            NULL, RAIJIN_BUILTIN_MESH_CUBE, &mesh
        ) == RAIJIN_ERROR_INVALID_ARGUMENT
    );

    CHECK(mesh == RAIJIN_MESH_INVALID);

    CHECK(
        raijin_context_get_builtin_mesh(NULL, RAIJIN_BUILTIN_MESH_CUBE, NULL) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        raijin_context_submit_instances(
            NULL, RAIJIN_MESH_INVALID, &instance, 1
        ) == RAIJIN_ERROR_INVALID_ARGUMENT
    );

    CHECK(
        raijin_context_draw_cube(NULL, &instance) ==
        RAIJIN_ERROR_INVALID_ARGUMENT
    );

    return EXIT_SUCCESS;
}
