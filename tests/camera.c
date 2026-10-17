#include "camera.h"

#include <cglm/cam.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expression)        \
    do {                         \
        if (!(expression)) {     \
            return EXIT_FAILURE; \
        }                        \
    } while (0)

static int matrix_near(const mat4 lhs, const mat4 rhs, float tolerance) {
    for (size_t column = 0; column < 4; ++column) {
        for (size_t row = 0; row < 4; ++row) {
            if (fabsf(lhs[column][row] - rhs[column][row]) > tolerance) {
                return 0;
            }
        }
    }
    return 1;
}

int main(void) {
    const float tolerance = 1e-5f;
    PanOrbitCamera camera = {0};
    PanOrbitCamera_init(&camera);

    vec3 eye = {3.0f, -4.0f, 2.0f};
    vec3 target = {0.5f, 1.0f, -0.25f};
    vec3 up = {0.1f, 0.2f, 1.0f};

    CHECK(PanOrbitCamera_look_at(&camera, eye, target, up));

    mat4 expected;
    glm_lookat(eye, target, up, expected);
    CHECK(matrix_near(camera.view_matrix, expected, tolerance));

    /*
     * Rebuilding from controller state must not change the initialized view.
     */
    mat4 initialized;
    glm_mat4_copy(camera.view_matrix, initialized);
    PanOrbitCamera_update(&camera);
    CHECK(matrix_near(camera.view_matrix, initialized, tolerance));

    vec3 direction = {-0.3f, 0.8f, 0.4f};
    float orbit_distance = 7.0f;

    CHECK(PanOrbitCamera_look_to(&camera, eye, direction, up, orbit_distance));

    vec3 normalized_direction;
    glm_vec3_normalize_to(direction, normalized_direction);

    vec3 look_to_target;
    glm_vec3_scale(normalized_direction, orbit_distance, look_to_target);
    glm_vec3_add(eye, look_to_target, look_to_target);

    glm_lookat(eye, look_to_target, up, expected);
    CHECK(matrix_near(camera.view_matrix, expected, tolerance));

    /*
     * Invalid input must not partially modify the camera.
     */
    unsigned char snapshot[sizeof(camera)];
    memcpy(snapshot, &camera, sizeof(camera));

    vec3 zero_direction = {0.0f, 0.0f, 0.0f};
    CHECK(!PanOrbitCamera_look_to(
        &camera, eye, zero_direction, up, orbit_distance
    ));
    CHECK(memcmp(snapshot, &camera, sizeof(camera)) == 0);

    /*
     * Non-default matrix test
     */
    float preserved_distance = camera.distance;

    vec3 matrix_eye = {-2.0f, 5.0f, 3.0f};
    vec3 matrix_target = {1.0f, -0.5f, 0.75f};
    vec3 matrix_up = {0.0f, 0.0f, 1.0f};

    mat4 requested_view;
    glm_lookat(matrix_eye, matrix_target, matrix_up, requested_view);

    CHECK(
        PanOrbitCamera_set_view_matrix(&camera, (const float*)requested_view)
    );
    CHECK(matrix_near(camera.view_matrix, requested_view, tolerance));
    CHECK(fabsf(camera.distance - preserved_distance) <= tolerance);

    PanOrbitCamera_update(&camera);
    CHECK(matrix_near(camera.view_matrix, requested_view, tolerance));

    return EXIT_SUCCESS;
}
