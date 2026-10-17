#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>
#include <stdbool.h>

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
bool PanOrbitCamera_look_at(
    PanOrbitCamera* cam,
    const float eye[3],
    const float target[3],
    const float up[3]
);
bool PanOrbitCamera_look_to(
    PanOrbitCamera* cam,
    const float eye[3],
    const float direction[3],
    const float up[3],
    float orbit_distance
);
bool PanOrbitCamera_set_view_matrix(
    PanOrbitCamera* cam, const float view_matrix[16]
);
void PanOrbitCamera_orbit(PanOrbitCamera* cam, vec2 mouse_delta);
void PanOrbitCamera_pan(PanOrbitCamera* cam, vec2 mouse_delta);
void PanOrbitCamera_zoom(PanOrbitCamera* cam, f32 scroll);
f32 PanOrbitCamera_get_focal_distance(const PanOrbitCamera* cam);
void PanOrbitCamera_set_focal_distance(PanOrbitCamera* cam, f32 dist);

#endif /* CAMERA_H */
