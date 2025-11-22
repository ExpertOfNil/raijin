#ifndef CIMPL_GLM_H
#define CIMPL_GLM_H

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

#include "cimpl_core.h"

#define FLOAT_EPS 0.000001f
#define PI 3.14159265358979323846

#define POSE_PRINT_FORMAT \
    "[%010d] %9.3f, %9.3f, %9.3f,%9.3f, %9.3f, %9.3f, %9.3f\n"

typedef struct Vec3 {
    f32 x, y, z;
} Vec3;
#define VEC3_ZERO {0.0f, 0.0f, 0.0f}

DEFINE_DYNAMIC_ARRAY(Vec3, Vec3Array)

typedef struct Ray {
    Vec3 position;
    Vec3 direction;
} Ray;

typedef struct Vec4 {
    f32 x, y, z, w;
} Vec4;

typedef struct Vec4 Quat;
#define QUAT_IDENTITY {0.0f, 0.0f, 0.0f, 1.0f}

typedef struct Mat3 {
    f32 xi, xj, xk;
    f32 yi, yj, yk;
    f32 zi, zj, zk;
} Mat3;
// clang-format off
#define MAT3_IDENTITY { \
    1.0f, 0.0f, 0.0f, \
    0.0f, 1.0f, 0.0f, \
    0.0f, 0.0f, 1.0f, \
}
// clang-format on

/** A column-vector, column-major 4x4 matrix
 *
 */
typedef struct Mat4 {
    f32 xi, xj, xk, xw;
    f32 yi, yj, yk, yw;
    f32 zi, zj, zk, zw;
    f32 ti, tj, tk, tw;
} Mat4;
// clang-format off
#define MAT4_IDENTITY { \
    1.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 1.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 1.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f, \
}
// clang-format on

/*** FUNCTION DECLARATIONS ***/

Quat Quat_slerp(Quat qa, Quat qb, float t);
Quat Quat_from_euler(Vec3 euler);
Vec3 Quat_to_euler(Quat q);
Quat Quat_from_mat3(Mat3 m);
Mat3 Quat_to_mat3(Quat q);

Quat Mat3_to_quat(Mat3 m);

void Mat4_print_with_id(Mat4 m, const char* id);
Mat4 Mat4_inverse_rigid(Mat4 src);
Mat4 Mat4_orthonormalize(Mat4 m);
Mat4 Mat4_perspective_NO(f32 fovy, f32 aspect, f32 near, f32 far);
Mat4 Mat4_perspective_ZO(f32 fovy, f32 aspect, f32 near, f32 far);
Mat4 Mat4_perspective_from_intrinsic_NO(
    Mat3 intrinsic, f32 w, f32 h, f32 near, f32 far
);
Mat4 Mat4_perspective_from_intrinsic_ZO(
    Mat3 intrinsic, f32 w, f32 h, f32 near, f32 far
);
#ifndef FORCE_DEPTH_ZERO_TO_ONE
#define Mat4_perspective Mat4_perspective_NO
#define Mat4_perspective_from_intrinsic Mat4_perspective_from_intrinsic_NO
#else
#define Mat4_perspective Mat4_perspective_ZO
#define Mat4_perspective_from_intrinsic Mat4_perspective_from_intrinsic_ZO
#endif
Mat4 Mat4_look_dir(Vec3 eye, Vec3 direction, Vec3 up);
Mat4 Mat4_look_at(Vec3 eye, Vec3 target, Vec3 up);

/*** STATIC FUNCTION DEFINITIONS ***/

/* General */
static inline f32 radians(f32 deg) {
    return deg * PI / 180.0f;
}

static inline f32 degrees(f32 rad) {
    return rad * 180.0f / PI;
}

static inline f32 clamp(f32 value, f32 min, f32 max) {
    f32 result = value < min ? min : value;
    result = result > max ? max : result;
    return result;
}

static inline f32 lerp(f32 start, f32 end, f32 amount) {
    f32 result = start + amount * (end - start);
    return result;
}

static inline f32 normalize(f32 value, f32 start, f32 end) {
    f32 result = (value - start) / (end - start);
    return result;
}

static inline f32 remap(
    f32 value, f32 in_start, f32 in_end, f32 out_start, f32 out_end
) {
    f32 t = normalize(value, in_start, in_end);
    return lerp(out_start, out_end, t);
}

static inline bool equals_f32(f32 a, f32 b) {
    bool result =
        fabsf(a - b) <= FLOAT_EPS * fmaxf(1.0f, fmaxf(fabsf(a), fabsf(b)));
    return result;
}

/* Vec3 */
static inline Vec3 Vec3_init(f32 x, f32 y, f32 z) {
    Vec3 v = {x, y, z};
    return v;
}

static inline Vec3 Vec3_add(Vec3 va, Vec3 vb) {
    Vec3 result = {va.x + vb.x, va.y + vb.y, va.z + vb.z};
    return result;
}

static inline Vec3 Vec3_sub(Vec3 va, Vec3 vb) {
    Vec3 result = {va.x - vb.x, va.y - vb.y, va.z - vb.z};
    return result;
}

static inline Vec3 Vec3_scale(Vec3 va, f32 s) {
    Vec3 result = {va.x * s, va.y * s, va.z * s};
    return result;
}

static inline f32 Vec3_dot(Vec3 va, Vec3 vb) {
    return va.x * vb.x + va.y * vb.y + va.z * vb.z;
}

static inline f32 Vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline Vec3 Vec3_project(Vec3 va, Vec3 vb) {
    f32 va_vb = Vec3_dot(va, vb);
    f32 va_va = Vec3_dot(va, va);
    f32 mag = va_vb / va_va;
    Vec3 result = Vec3_scale(vb, mag);
    return result;
}

static inline Vec3 Vec3_reject(Vec3 va, Vec3 vb) {
    Vec3 proj = Vec3_project(va, vb);
    Vec3 result = Vec3_sub(va, proj);
    return result;
}

static inline Vec3 Vec3_normalize(Vec3 v) {
    f32 mag = Vec3_length(v);
    v.x = v.x / mag;
    v.y = v.y / mag;
    v.z = v.z / mag;
    return v;
}

static inline Vec3 Vec3_cross(Vec3 a, Vec3 b) {
    Vec3 c = VEC3_ZERO;
    c.x = a.y * b.z - a.z * b.y;
    c.y = a.z * b.x - a.x * b.z;
    c.z = a.x * b.y - a.y * b.x;
    return c;
}

static inline Vec3 Vec3_translation_from_mat4(Mat4 m) {
    Vec3 translation = {m.ti, m.tj, m.tk};
    return translation;
}

/* Ray */
static inline Ray Ray_init(Vec3 position, Vec3 direction) {
    Ray r = {
        .position = position,
        .direction = direction,
    };
    return r;
}

/* Quat */
static inline Quat Quat_init(f32 x, f32 y, f32 z, f32 w) {
    Quat q = {x, y, z, w};
    return q;
}

static inline f32 Quat_dot(Quat qa, Quat qb) {
    return qa.x * qb.x + qa.y * qb.y + qa.z * qb.z + qa.w * qb.w;
}

static inline Quat Quat_mul(Quat qa, Quat qb) {
    Quat result = QUAT_IDENTITY;

    result.x = qa.x * qb.w + qa.w * qb.x + qa.y * qb.z - qa.z * qb.y;
    result.y = qa.y * qb.w + qa.w * qb.y + qa.z * qb.x - qa.x * qb.z;
    result.z = qa.z * qb.w + qa.w * qb.z + qa.x * qb.y - qa.y * qb.x;
    result.w = qa.w * qb.w - qa.x * qb.x - qa.y * qb.y - qa.z * qb.z;

    return result;
}

static inline f32 Quat_length(Quat q) {
    return sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

static inline Quat Quat_normalize(Quat q) {
    f32 mag = Quat_length(q);
    if (mag == 0.0f) {
        mag = 1.0f;
    }
    f32 imag = 1.0f / mag;
    q.x = q.x * imag;
    q.y = q.y * imag;
    q.z = q.z * imag;
    q.w = q.w * imag;
    return q;
}

static inline Quat Quat_inverse(Quat q) {
    Quat result = q;
    float sq_mag = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;

    if (sq_mag != 0.0f) {
        float isq_mag = 1.0f / sq_mag;
        result.x *= -isq_mag;
        result.y *= -isq_mag;
        result.z *= -isq_mag;
        result.w *= isq_mag;
    }

    return result;
}

static inline Quat Quat_norm_lerp(Quat qa, Quat qb, float t) {
    Quat result = QUAT_IDENTITY;

    // Quaternion lerp
    result.x = qa.x + t * (qb.x - qa.x);
    result.y = qa.y + t * (qb.y - qa.y);
    result.z = qa.z + t * (qb.z - qa.z);
    result.w = qa.w + t * (qb.w - qa.w);

    return Quat_normalize(result);
}

/* Mat3 */
static inline Mat3 Mat3_from_quat(Quat q) {
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    Mat3 dst;

    dst.xi = 1.0f - 2.0f * (yy + zz);
    dst.xj = 2.0f * (xy + wz);
    dst.xk = 2.0f * (xz - wy);

    dst.yi = 2.0f * (xy - wz);
    dst.yj = 1.0f - 2.0f * (xx + zz);
    dst.yk = 2.0f * (yz + wx);

    dst.zi = 2.0f * (xz + wy);
    dst.zj = 2.0f * (yz - wx);
    dst.zk = 1.0f - 2.0f * (xx + yy);

    return dst;
}

static inline Mat3 Mat3_rotation_from_mat4(Mat4 m) {
    Mat3 rotation = {
        // clang-format off
        m.xi, m.xj, m.xk,
        m.yi, m.yj, m.yk,
        m.zi, m.zj, m.zk,
        // clang-format on
    };
    return rotation;
}

/* Mat4 */
static inline Mat4 Mat4_with_rotation(Mat4 dst, Mat3 src) {
    dst.xi = src.xi;
    dst.xj = src.xj;
    dst.xk = src.xk;
    dst.yi = src.yi;
    dst.yj = src.yj;
    dst.yk = src.yk;
    dst.zi = src.zi;
    dst.zj = src.zj;
    dst.zk = src.zk;
    return dst;
}

static inline Mat4 Mat4_from_translation_quat(Vec3 t, Quat q) {
    Mat4 m = MAT4_IDENTITY;
    m.ti = t.x;
    m.tj = t.y;
    m.tk = t.z;
    Mat3 R = Mat3_from_quat(q);
    return Mat4_with_rotation(m, R);
}

static inline Mat4 Mat4_mul(Mat4 a, Mat4 b) {
    Mat4 dst = MAT4_IDENTITY;
    dst.xi = a.xi * b.xi + a.yi * b.xj + a.zi * b.xk + a.ti * b.xw;
    dst.xj = a.xj * b.xi + a.yj * b.xj + a.zj * b.xk + a.tj * b.xw;
    dst.xk = a.xk * b.xi + a.yk * b.xj + a.zk * b.xk + a.tk * b.xw;
    dst.xw = a.xw * b.xi + a.yw * b.xj + a.zw * b.xk + a.tw * b.xw;

    dst.yi = a.xi * b.yi + a.yi * b.yj + a.zi * b.yk + a.ti * b.yw;
    dst.yj = a.xj * b.yi + a.yj * b.yj + a.zj * b.yk + a.tj * b.yw;
    dst.yk = a.xk * b.yi + a.yk * b.yj + a.zk * b.yk + a.tk * b.yw;
    dst.yw = a.xw * b.yi + a.yw * b.yj + a.zw * b.yk + a.tw * b.yw;

    dst.zi = a.xi * b.zi + a.yi * b.zj + a.zi * b.zk + a.ti * b.zw;
    dst.zj = a.xj * b.zi + a.yj * b.zj + a.zj * b.zk + a.tj * b.zw;
    dst.zk = a.xk * b.zi + a.yk * b.zj + a.zk * b.zk + a.tk * b.zw;
    dst.zw = a.xw * b.zi + a.yw * b.zj + a.zw * b.zk + a.tw * b.zw;

    dst.ti = a.xi * b.ti + a.yi * b.tj + a.zi * b.tk + a.ti * b.tw;
    dst.tj = a.xj * b.ti + a.yj * b.tj + a.zj * b.tk + a.tj * b.tw;
    dst.tk = a.xk * b.ti + a.yk * b.tj + a.zk * b.tk + a.tk * b.tw;
    dst.tw = a.xw * b.ti + a.yw * b.tj + a.zw * b.tk + a.tw * b.tw;

    return dst;
}

static inline Mat3 Mat4_rotation(Mat4 m) {
    Mat3 rotation = {
        // clang-format off
        m.xi, m.xj, m.xk,
        m.yi, m.yj, m.yk,
        m.zi, m.zj, m.zk,
        // clang-format on
    };
    return rotation;
}

static inline Vec3 Mat4_translation(Mat4 m) {
    Vec3 translation = {m.ti, m.tj, m.tk};
    return translation;
}

/*** NON-STATIC FUNCTION DEFINITIONS ***/
#ifdef CIMPL_IMPLEMENTATION

/* Quat */
Quat Quat_from_euler(Vec3 euler) {
    Quat result = QUAT_IDENTITY;

    float xa = cosf(euler.x * 0.5f);
    float xb = sinf(euler.x * 0.5f);
    float ya = cosf(euler.y * 0.5f);
    float yb = sinf(euler.y * 0.5f);
    float za = cosf(euler.z * 0.5f);
    float zb = sinf(euler.z * 0.5f);

    result.x = xb * ya * za - xa * yb * zb;
    result.y = xa * yb * za + xb * ya * zb;
    result.z = xa * ya * zb - xb * yb * za;
    result.w = xa * ya * za + xb * yb * zb;

    return result;
}

Vec3 Quat_to_euler(Quat q) {
    Vec3 result = VEC3_ZERO;

    // x-axis rotation
    float xa = 2.0f * (q.w * q.x + q.y * q.z);
    float xb = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    result.x = atan2f(xa, xb);

    // y-axis rotation
    float ya = 2.0f * (q.w * q.y - q.z * q.x);
    ya = ya > 1.0f ? 1.0f : ya;
    ya = ya < -1.0f ? -1.0f : ya;
    result.y = asinf(ya);

    // z-axis rotation
    float za = 2.0f * (q.w * q.z + q.x * q.y);
    float zb = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    result.z = atan2f(za, zb);

    return result;
}

/* Convert rotation matrix to quaternion
 *
 * Use Shepperd's method.  No normalization is performed.
 */
Quat Quat_from_mat3(Mat3 m) {
    Quat result = QUAT_IDENTITY;

    float four_w_sq_m1 = m.xi + m.yj + m.zk;
    float four_x_sq_m1 = m.xi - m.yj - m.zk;
    float four_y_sq_m1 = m.yj - m.xi - m.zk;
    float four_z_sq_m1 = m.zk - m.xi - m.yj;

    u32 biggest_index = 0;
    float four_biggest_sq_m1 = four_w_sq_m1;
    if (four_x_sq_m1 > four_biggest_sq_m1) {
        four_biggest_sq_m1 = four_x_sq_m1;
        biggest_index = 1;
    }
    if (four_y_sq_m1 > four_biggest_sq_m1) {
        four_biggest_sq_m1 = four_y_sq_m1;
        biggest_index = 2;
    }
    if (four_z_sq_m1 > four_biggest_sq_m1) {
        four_biggest_sq_m1 = four_z_sq_m1;
        biggest_index = 3;
    }

    float biggest_val = sqrtf(1.0f + four_biggest_sq_m1) * 0.5f;
    float s = 0.25f / biggest_val;

    switch (biggest_index) {
        case 0:
            result.x = (m.yk - m.zj) * s;
            result.y = (m.zi - m.xk) * s;
            result.z = (m.xj - m.yi) * s;
            result.w = biggest_val;
            break;
        case 1:
            result.x = biggest_val;
            result.y = (m.xj + m.yi) * s;
            result.z = (m.zi + m.xk) * s;
            result.w = (m.yk - m.zj) * s;
            break;
        case 2:
            result.x = (m.xj + m.yi) * s;
            result.y = biggest_val;
            result.z = (m.yk + m.zj) * s;
            result.w = (m.zi - m.xk) * s;
            break;
        case 3:
            result.x = (m.zi + m.xk) * s;
            result.y = (m.yk + m.zj) * s;
            result.z = biggest_val;
            result.w = (m.xj - m.yi) * s;
            break;
    }

    return result;
}

Mat3 Quat_to_mat3(Quat q) { return Mat3_from_quat(q); }

Quat Quat_slerp(Quat qa, Quat qb, float t) {
    float dotq = Quat_dot(Quat_normalize(qa), Quat_normalize(qb));

    // Quaternions are in different hemispheres
    if (dotq < 0.0f) {
        qb.x = -qb.x;
        qb.y = -qb.y;
        qb.z = -qb.z;
        qb.w = -qb.w;
        dotq = -dotq;
    }

    if (fabsf(dotq) >= 1.0f) {
        return qa;
    } else if (dotq > 0.95f) {
        return Quat_norm_lerp(qa, qb, t);
    }

    float theta = acosf(dotq);
    float sin_theta = sqrtf(1.0f - dotq * dotq);

    Quat result = QUAT_IDENTITY;
    if (fabsf(sin_theta) < FLOAT_EPS) {
        result.x = qa.x * 0.5f + qb.x * 0.5f;
        result.y = qa.y * 0.5f + qb.y * 0.5f;
        result.z = qa.z * 0.5f + qb.z * 0.5f;
        result.w = qa.w * 0.5f + qb.w * 0.5f;

    } else {
        float ca = sinf((1.0f - t) * theta) / sin_theta;
        float cb = sinf(t * theta) / sin_theta;

        result.x = qa.x * ca + qb.x * cb;
        result.y = qa.y * ca + qb.y * cb;
        result.z = qa.z * ca + qb.z * cb;
        result.w = qa.w * ca + qb.w * cb;
    }
    return result;
}

/* Mat3 */
Quat Mat3_to_quat(Mat3 m) { return Quat_from_mat3(m); }

/* Mat4 */
void Mat4_print_with_id(Mat4 m, const char* id) {
    printf(
        "%s: [\n"
        "  %9.3f, %9.3f, %9.3f, %9.3f\n"
        "  %9.3f, %9.3f, %9.3f, %9.3f\n"
        "  %9.3f, %9.3f, %9.3f, %9.3f\n"
        "  %9.3f, %9.3f, %9.3f, %9.3f\n"
        "]\n",
        id,
        // clang-format off
        (double)m.xi, (double)m.yi, (double)m.zi, (double)m.ti,
        (double)m.xj, (double)m.yj, (double)m.zj, (double)m.tj,
        (double)m.xk, (double)m.yk, (double)m.zk, (double)m.tk,
        (double)m.xw, (double)m.yw, (double)m.zw, (double)m.tw
        // clang-format on
    );
}

Mat4 Mat4_orthonormalize(Mat4 m) {
    Mat4 dst = MAT4_IDENTITY;
    // Ensure x-axis is orthogonal to the yz-plane
    Vec3 x_axis =
        Vec3_cross(Vec3_init(m.yi, m.yj, m.yk), Vec3_init(m.zi, m.zj, m.zk));
    x_axis = Vec3_normalize(x_axis);
    dst.xi = x_axis.x;
    dst.xj = x_axis.y;
    dst.xk = x_axis.z;
    // Ensure y-axis is orthogonal to the xz-plane
    Vec3 y_axis = Vec3_cross(
        Vec3_init(m.zi, m.zj, m.zk), Vec3_init(dst.xi, dst.xj, dst.xk)
    );
    y_axis = Vec3_normalize(y_axis);
    dst.yi = y_axis.x;
    dst.yj = y_axis.y;
    dst.yk = y_axis.z;
    // Ensure z-axis is normalized
    Vec3 z_axis = Vec3_normalize(Vec3_init(m.zi, m.zj, m.zk));
    dst.zi = z_axis.x;
    dst.zj = z_axis.y;
    dst.zk = z_axis.z;

    dst.ti = m.ti;
    dst.tj = m.tj;
    dst.tk = m.tk;
    return dst;
}

Mat4 Mat4_inverse_rigid(Mat4 src) {
    Mat4 dst = MAT4_IDENTITY;
    Vec3 x = Vec3_normalize(Vec3_init(src.xi, src.xj, src.xk));
    Vec3 y = Vec3_normalize(Vec3_init(src.yi, src.yj, src.yk));
    Vec3 z = Vec3_normalize(Vec3_init(src.zi, src.zj, src.zk));
    dst.xi = x.x;
    dst.xj = y.x;
    dst.xk = z.x;

    dst.yi = x.y;
    dst.yj = y.y;
    dst.yk = z.y;

    dst.zi = x.z;
    dst.zj = y.z;
    dst.zk = z.z;

    dst.ti = -(x.x * src.ti + x.y * src.tj + x.z * src.tk);
    dst.tj = -(y.x * src.ti + y.y * src.tj + y.z * src.tk);
    dst.tk = -(z.x * src.ti + z.y * src.tj + z.z * src.tk);

    return dst;
}

Mat4 Mat4_perspective_NO(f32 fovy, f32 aspect, f32 near, f32 far) {
    Mat4 result = {0};
    f32 tan_half_fovy = tanf(fovy * 0.5f);

    result.xi = 1.0f / (aspect * tan_half_fovy);
    result.yj = 1.0f / tan_half_fovy;
    result.zk = -(far + near) / (far - near);
    result.zw = -1.0f;
    result.tk = -(2.0f * far * near) / (far - near);

    return result;
}

Mat4 Mat4_perspective_ZO(f32 fovy, f32 aspect, f32 near, f32 far) {
    Mat4 result = {0};
    f32 tan_half_fovy = tanf(fovy * 0.5f);

    result.xi = 1.0f / (aspect * tan_half_fovy);
    result.yj = 1.0f / tan_half_fovy;
    result.zk = far / (near - far);
    result.zw = -1.0f;
    result.tk = -(far * near) / (far - near);

    return result;
}

Mat4 Mat4_perspective_from_intrinsic_NO(
    Mat3 intrinsic, f32 w, f32 h, f32 near, f32 far
) {
    f32 fx = intrinsic.xi;
    f32 fy = intrinsic.yj;
    f32 cx = intrinsic.zi;
    f32 cy = intrinsic.zj;

    f32 fovy = 2.0f * atanf(h / (2.0f * fy));
    f32 aspect = (w * fy) / (h * fx);

    Mat4 result = Mat4_perspective_NO(fovy, aspect, near, far);
    result.zi = 1.0f - 2.0f * cx / w;
    result.zj = 2.0f * cy / h - 1.0f;

    return result;
}

Mat4 Mat4_perspective_from_intrinsic_ZO(
    Mat3 intrinsic, f32 w, f32 h, f32 near, f32 far
) {
    f32 fx = intrinsic.xi;
    f32 fy = intrinsic.yj;
    f32 cx = intrinsic.zi;
    f32 cy = intrinsic.zj;

    f32 fovy = 2.0f * atanf(h / (2.0f * fy));
    f32 aspect = (w * fy) / (h * fx);

    Mat4 result = Mat4_perspective_ZO(fovy, aspect, near, far);
    result.zi = 1.0f - 2.0f * cx / w;
    result.zj = 2.0f * cy / h - 1.0f;

    return result;
}

Mat4 Mat4_look_dir(Vec3 eye, Vec3 direction, Vec3 up) {
    Vec3 f = Vec3_normalize(direction);
    Vec3 r = Vec3_normalize(Vec3_cross(f, up));
    Vec3 u = Vec3_cross(r, f);

    Mat4 result = MAT4_IDENTITY;
    result.xi = r.x;
    result.yi = r.y;
    result.zi = r.z;

    result.xj = u.x;
    result.yj = u.y;
    result.zj = u.z;

    result.xk = -f.x;
    result.yk = -f.y;
    result.zk = -f.z;

    result.ti = -Vec3_dot(r, eye);
    result.tj = -Vec3_dot(u, eye);
    result.tk = Vec3_dot(f, eye);

    return result;
}

Mat4 Mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 direction = Vec3_sub(target, eye);
    return Mat4_look_dir(eye, direction, up);
}

#endif /* CIMPL_IMPLEMENTATION */

#endif /* CIMPL_GLM_H */
