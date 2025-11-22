#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>

#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "cglm/util.h"
#include "cglm/vec3.h"
#include "cimpl_core.h"

typedef struct PanOrbitCamera {
    mat4 view_matrix;
    mat4 proj_matrix;
    vec3 target;
    vec4 orientation;
    f32 distance;
    f32 distance_min;
    f32 distance_max;
    f32 mouse_speed;
    f32 zoom_speed;
    f32 pan_speed;
    f32 z_near;
    f32 z_far;
    f32 aspect;
    f32 fovy;
} PanOrbitCamera;

/* Function Prototypes */

void PanOrbitCamera_init(PanOrbitCamera* cam);
void PanOrbitCamera_update(PanOrbitCamera* cam);
void PanOrbitCamera_orbit(PanOrbitCamera* cam, vec2 mouse_delta);
void PanOrbitCamera_pan(PanOrbitCamera* cam, vec2 mouse_delta);
f32 PanOrbitCamera_get_focal_distance(const PanOrbitCamera* cam);
void PanOrbitCamera_set_focal_distance(PanOrbitCamera* cam, f32 dist);

/* Functions */

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
    float yaw = -mouse_delta[0] * cam->mouse_speed;
    float pitch = -mouse_delta[1] * cam->mouse_speed;
    vec4 yaw_q;
    glm_quatv(yaw_q, yaw, GLM_ZUP);
    vec3 rt;
    glm_quat_rotatev(cam->orientation, GLM_XUP, rt);
    vec4 pitch_q;
    glm_quatv(pitch_q, pitch, rt);
    vec4 q;
    glm_quat_mul(yaw_q, pitch_q, q);
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

#endif /* CAMERA_H */
