#include "camera.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "cglm/util.h"
#include "cglm/vec3.h"

static inline bool vec3_is_finite(const float value[3]) {
    return isfinite(value[0]) && isfinite(value[1]) && isfinite(value[2]);
}

static inline bool quat_is_finite(const float value[4]) {
    return isfinite(value[0]) && isfinite(value[1]) && isfinite(value[2]) &&
           isfinite(value[3]);
}

static bool mat4_is_finite(const mat4 matrix) {
    for (size_t col = 0; col < 4; ++col) {
        for (size_t row = 0; row < 4; ++row) {
            if (!isfinite(matrix[col][row])) return false;
        }
    }
    return true;
}

static bool mat4_near(const mat4 lhs, const mat4 rhs, float tol) {
    for (size_t col = 0; col < 4; ++col) {
        for (size_t row = 0; row < 4; ++row) {
            float a = lhs[col][row];
            float b = rhs[col][row];
            float scale = fmaxf(1.0f, fmaxf(fabsf(a), fabsf(b)));

            if (fabsf(a - b) > tol * scale) return false;
        }
    }
    return true;
}

void PanOrbitCamera_init(PanOrbitCamera* cam) {
    cam->z_near = 0.1f;
    cam->z_far = 1000.0f;
    cam->aspect = 16.0 / 9.0f;
    cam->fovy = glm_rad(60.0f);
    glm_vec3_copy(GLM_VEC3_ZERO, cam->target);
    cam->distance = 10.0f;
    cam->distance_min = 0.1f;
    cam->distance_max = 1000.0f;
    cam->mouse_speed = 0.005f;
    cam->zoom_speed = 0.5f;
    cam->pan_speed = 0.001f;

    glm_quat_identity(cam->orientation);

    glm_perspective(
        cam->fovy, cam->aspect, cam->z_near, cam->z_far, cam->proj_matrix
    );
    PanOrbitCamera_update(cam);
}

void PanOrbitCamera_update(PanOrbitCamera* cam) {
    glm_clamp(cam->distance, cam->distance_min, cam->distance_max);

    vec3 offset;
    glm_quat_rotatev(
        cam->orientation, (vec3){0.0f, -cam->distance, 0.0}, offset
    );

    vec3 position;
    glm_vec3_add(cam->target, offset, position);
    vec3 up;
    glm_quat_rotatev(cam->orientation, GLM_ZUP, up);
    glm_lookat(position, cam->target, up, cam->view_matrix);
}

// TODO (mmckenna): add function for adding aspect ratio for resize

void PanOrbitCamera_orbit(PanOrbitCamera* cam, vec2 mouse_delta) {
    float angle = glm_vec2_norm(mouse_delta) * cam->mouse_speed;
    if (angle < 1e-6f) return;

    vec3 screen_axis = {-mouse_delta[1], 0.0f, -mouse_delta[0]};
    glm_vec3_normalize(screen_axis);

    vec3 world_axis;
    glm_quat_rotatev(cam->orientation, screen_axis, world_axis);

    vec4 q;
    glm_quatv(q, angle, world_axis);
    glm_quat_mul(q, cam->orientation, cam->orientation);
    glm_quat_normalize(cam->orientation);
    PanOrbitCamera_update(cam);
}

void PanOrbitCamera_pan(PanOrbitCamera* cam, vec2 mouse_delta) {
    vec3 right, up;
    glm_quat_rotatev(cam->orientation, GLM_XUP, right);
    glm_quat_rotatev(cam->orientation, GLM_ZUP, up);

    f32 scale = cam->pan_speed * cam->distance;

    vec3 offset;
    glm_vec3_scale(right, -mouse_delta[0] * scale, offset);
    glm_vec3_muladds(up, mouse_delta[1] * scale, offset);

    glm_vec3_add(cam->target, offset, cam->target);
    PanOrbitCamera_update(cam);
}

void PanOrbitCamera_zoom(PanOrbitCamera* cam, f32 scroll) {
    cam->distance -= scroll * cam->zoom_speed;
    PanOrbitCamera_update(cam);
}

f32 PanOrbitCamera_get_focal_distance(const PanOrbitCamera* cam) {
    return cam->proj_matrix[1][1];
}

void PanOrbitCamera_set_focal_distance(PanOrbitCamera* cam, f32 dist) {
    cam->proj_matrix[0][0] =
        (cam->proj_matrix[0][0] / cam->proj_matrix[1][1]) * dist;
    cam->proj_matrix[1][1] = dist;
}

bool PanOrbitCamera_look_to(
    PanOrbitCamera* cam,
    const float eye[3],
    const float direction[3],
    const float up[3],
    float orbit_distance
) {
    const float eps = 1e-6f;
    if (cam == NULL || eye == NULL || direction == NULL || up == NULL) {
        return false;
    }

    if (!vec3_is_finite(eye)) return false;
    if (!vec3_is_finite(direction)) return false;
    if (!vec3_is_finite(up)) return false;
    if (!isfinite(orbit_distance)) return false;

    if (orbit_distance < cam->distance_min ||
        orbit_distance > cam->distance_max) {
        return false;
    }

    vec3 fwd_norm = {direction[0], direction[1], direction[2]};
    float fwd_len = glm_vec3_norm(fwd_norm);
    if (!isfinite(fwd_len) || fwd_len <= eps) return false;
    glm_vec3_scale(fwd_norm, 1.0f / fwd_len, fwd_norm);

    vec3 up_norm = {up[0], up[1], up[2]};
    float up_len = glm_vec3_norm(up_norm);
    if (!isfinite(up_len) || up_len <= eps) return false;
    glm_vec3_scale(up_norm, 1.0f / up_len, up_norm);

    vec3 right_norm;
    glm_vec3_cross(fwd_norm, up_norm, right_norm);
    float right_len = glm_vec3_norm(right_norm);
    if (!isfinite(right_len) || right_len <= eps) return false;
    glm_vec3_scale(right_norm, 1.0f / right_len, right_norm);

    vec3 corrected_up;
    glm_vec3_cross(right_norm, fwd_norm, corrected_up);
    glm_vec3_normalize(corrected_up);

    mat3 orientation_basis = {0};
    glm_vec3_copy(right_norm, orientation_basis[0]);
    glm_vec3_copy(fwd_norm, orientation_basis[1]);
    glm_vec3_copy(corrected_up, orientation_basis[2]);

    vec4 orientation;
    glm_mat3_quat(orientation_basis, orientation);
    glm_quat_normalize(orientation);

    if (!quat_is_finite(orientation)) return false;

    vec3 target_offset;
    glm_vec3_scale(fwd_norm, orbit_distance, target_offset);

    vec3 target = {eye[0], eye[1], eye[2]};
    glm_vec3_add(target, target_offset, target);
    if (!vec3_is_finite(target)) return false;

    glm_vec3_copy(target, cam->target);
    glm_quat_copy(orientation, cam->orientation);
    cam->distance = orbit_distance;

    PanOrbitCamera_update(cam);
    return true;
}

bool PanOrbitCamera_look_at(
    PanOrbitCamera* cam,
    const float eye[3],
    const float target[3],
    const float up[3]
) {
    if (cam == NULL || eye == NULL || target == NULL || up == NULL) {
        return false;
    }

    if (!vec3_is_finite(eye) || !vec3_is_finite(target) ||
        !vec3_is_finite(up)) {
        return false;
    }

    vec3 direction = {
        target[0] - eye[0],
        target[1] - eye[1],
        target[2] - eye[2],
    };

    float orbit_distance = glm_vec3_norm(direction);
    if (!isfinite(orbit_distance)) return false;

    return PanOrbitCamera_look_to(cam, eye, direction, up, orbit_distance);
}

bool PanOrbitCamera_set_view_matrix(
    PanOrbitCamera* cam, const float view_matrix[16]
) {
    const float eps = 1e-5f;

    if (cam == NULL || view_matrix == NULL) return false;

    mat4 requested;
    memcpy(requested, view_matrix, sizeof(requested));
    if (!mat4_is_finite(requested)) return false;

    float det = glm_mat4_det(requested);
    if (!isfinite(det) || fabsf(det) <= FLT_EPSILON) return false;

    mat4 camera_world;
    glm_mat4_inv(requested, camera_world);
    if (!mat4_is_finite(camera_world)) return false;

    vec3 eye = {
        camera_world[3][0],
        camera_world[3][1],
        camera_world[3][2],
    };

    vec3 fwd = {
        -camera_world[2][0],
        -camera_world[2][1],
        -camera_world[2][2],
    };

    vec3 up = {
        camera_world[1][0],
        camera_world[1][1],
        camera_world[1][2],
    };

    PanOrbitCamera candidate = *cam;
    if (!PanOrbitCamera_look_to(&candidate, eye, fwd, up, cam->distance)) {
        return false;
    }

    if (!mat4_near(candidate.view_matrix, requested, eps)) return false;

    memcpy(candidate.view_matrix, requested, sizeof(candidate.view_matrix));
    *cam = candidate;
    return true;
}
