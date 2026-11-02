#ifndef RAIJIN_H_EXTERNAL
#define RAIJIN_H_EXTERNAL

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t RaijinResult;
#define RAIJIN_SUCCESS ((RaijinResult)0u)
#define RAIJIN_ERROR_INVALID_ARGUMENT ((RaijinResult)1u)
#define RAIJIN_ERROR_OUT_OF_MEMORY ((RaijinResult)2u)
#define RAIJIN_ERROR_INITIALIZATION ((RaijinResult)3u)
#define RAIJIN_ERROR_RENDER ((RaijinResult)4u)
#define RAIJIN_ERROR_INVALID_STATE ((RaijinResult)5u)
#define RAIJIN_ERROR_BUFFER_TOO_SMALL ((RaijinResult)6u)
#define RAIJIN_ERROR_READBACK ((RaijinResult)7u)
#define RAIJIN_ERROR_INVALID_HANDLE ((RaijinResult)8u)
#define RAIJIN_ERROR_TIMEOUT ((RaijinResult)9u)
#define RAIJIN_ERROR_DEVICE_LOST ((RaijinResult)10u)
#define RAIJIN_ERROR_MESH_CREATION ((RaijinResult)11u)
#define RAIJIN_ERROR_IO ((RaijinResult)12u)
#define RAIJIN_ERROR_NOT_FOUND ((RaijinResult)13u)
#define RAIJIN_ERROR_INVALID_FORMAT ((RaijinResult)14u)
#define RAIJIN_ERROR_UNSUPPORTED_VERSION ((RaijinResult)15u)

typedef struct RaijinContext RaijinContext;

typedef uint32_t RaijinRenderMode;
#define RAIJIN_RENDER_MODE_HEADLESS ((RaijinRenderMode)0u)
#define RAIJIN_RENDER_MODE_WINDOWED ((RaijinRenderMode)1u)

typedef uint64_t RaijinMesh;
#define RAIJIN_MESH_INVALID ((RaijinMesh)0u)

typedef uint32_t RaijinBuiltinMesh;
#define RAIJIN_BUILTIN_MESH_CUBE ((RaijinBuiltinMesh)1u)
#define RAIJIN_BUILTIN_MESH_PLANE ((RaijinBuiltinMesh)2u)
#define RAIJIN_BUILTIN_MESH_DISC ((RaijinBuiltinMesh)3u)
#define RAIJIN_BUILTIN_MESH_CYLINDER ((RaijinBuiltinMesh)4u)
#define RAIJIN_BUILTIN_MESH_CONE ((RaijinBuiltinMesh)5u)
#define RAIJIN_BUILTIN_MESH_SPHERE_UV ((RaijinBuiltinMesh)6u)

typedef uint64_t RaijinAssembly;
#define RAIJIN_ASSEMBLY_INVALID ((RaijinAssembly)0u)

typedef uint32_t RaijinKey;
#define RAIJIN_KEY_UNKNOWN ((RaijinKey)0u)
#define RAIJIN_KEY_ESC ((RaijinKey)1u)
#define RAIJIN_KEY_TAB ((RaijinKey)2u)
#define RAIJIN_KEY_S ((RaijinKey)3u)
#define RAIJIN_KEY_L ((RaijinKey)4u)

typedef uint32_t RaijinKeyModifiers;
#define RAIJIN_KEY_MOD_NONE ((RaijinKeyModifiers)0u)
#define RAIJIN_KEY_MOD_SHIFT ((RaijinKeyModifiers)1u)
#define RAIJIN_KEY_MOD_CTRL ((RaijinKeyModifiers)(1u << 1))
#define RAIJIN_KEY_MOD_ALT ((RaijinKeyModifiers)(1u << 2))
#define RAIJIN_KEY_MOD_SUPER ((RaijinKeyModifiers)(1u << 3))

typedef struct RaijinContextDesc {
    const char* title;
    uint32_t width;
    uint32_t height;
    RaijinRenderMode render_mode;
} RaijinContextDesc;

typedef struct RaijinInstance {
    /* Column major 4x4 homogeneous transformation matrix */
    float transform[16];
    /* Linear RGBA color */
    float color[4];
} RaijinInstance;

typedef struct RaijinVertex {
    float position[3];
    float color[3];
    float normal[3];
} RaijinVertex;

/*
 * All arrays are borrowed for the duration of raijin_create_mesh().
 * Raijin does not retain these pointers or assume ownership of their storage.
 *
 * triangle_index_count must be divisible by three.
 *
 * Edge indices are optional. When edge_index_count is nonzero, edge_indices
 * must be nonnull and edge_index_count must be divisible by two.
 */
typedef struct RaijinMeshDesc {
    const RaijinVertex* vertices;
    uint32_t vertex_count;

    const uint32_t* triangle_indices;
    uint32_t triangle_index_count;

    const uint32_t* edge_indices;
    uint32_t edge_index_count;
} RaijinMeshDesc;

/*
 * Camera pose and projection, independent of GPU resources.
 *
 * All components must be finite.
 */
typedef struct RaijinCameraState {
    /* World-space orbit pivot. */
    float target[3];
    /*
     * Unit quaternion in x, y, z, w order.
     * Identity orientation faces +Y with +Z as up.
     * Applying state permits small normalization drift.
     */
    float orientation[4];
    /* Positive distance from the eye to the orbit pivot. */
    float distance;
    /*
     * Column-major, aspect-adjustable projection matrix.
     * Has the same semantics as raijin_camera_set_projection_matrix().
     */
    float projection[16];
} RaijinCameraState;

/*
 * Supported persistent viewer state.
 *
 * Currently contains only camera state, not complete scene contents.
 * This in-memory representation is not an on-disk file format.
 */
typedef struct RaijinState {
    RaijinCameraState camera;
} RaijinState;

typedef struct RaijinKeyEvent {
    RaijinKey key;
    RaijinKeyModifiers modifiers;
    // 1 = down, 0 = up
    uint32_t pressed;
    // 1 = auto-repeat key-down, otherwise 0
    uint32_t repeat;
} RaijinKeyEvent;

/*
 * event is borrowed and valid only during this call.
 *
 * Do not call Raijin context operations from the callback, including
 * polling, rendering, state application, callback registration, or
 * destruction. Record application requests and handle them after
 * raijin_poll_events() returns.
 */
typedef void (*RaijinKeyCallback)(const RaijinKeyEvent* event, void* user_data);

RaijinResult raijin_create(const RaijinContextDesc* desc, RaijinContext** out);
RaijinResult raijin_poll_events(RaijinContext* ctx);
RaijinResult raijin_get_viewport_size(
    const RaijinContext* ctx, uint32_t* out_width, uint32_t* out_height
);
RaijinResult raijin_is_minimized(
    const RaijinContext* ctx, uint32_t* out_is_minimized
);
RaijinResult raijin_camera_look_to(
    RaijinContext* ctx,
    const float eye[3],
    const float direction[3],
    const float up[3],
    float orbit_distance
);
RaijinResult raijin_camera_look_at(
    RaijinContext* ctx,
    const float eye[3],
    const float target[3],
    const float up[3]
);
RaijinResult raijin_camera_set_view_matrix(
    RaijinContext* ctx, const float view_matrix[16]
);
/*
 * Sets a column-major, aspect-adjustable projection matrix.
 *
 * Raijin preserves the matrix's vertical projection scale and updates its
 * horizontal scale for the current viewport aspect ratio immediately and
 * whenever the viewport is resized.
 */
RaijinResult raijin_camera_set_projection_matrix(
    RaijinContext* ctx, const float proj_matrix[16]
);
RaijinResult raijin_set_edge_visibility(
    RaijinContext* ctx, uint32_t is_visible
);
RaijinResult raijin_get_edge_visibility(
    const RaijinContext* ctx, uint32_t* out_is_visible
);
RaijinResult raijin_set_clear_color(
    RaijinContext* ctx, const float linear_rgba[4]
);
RaijinResult raijin_get_clear_color(
    const RaijinContext* ctx, float out_linear_rgba[4]
);
RaijinResult raijin_should_close(
    const RaijinContext* ctx, uint32_t* out_should_close
);
RaijinResult raijin_render(RaijinContext* ctx);
RaijinResult raijin_readback_size(
    const RaijinContext* ctx, uint64_t* out_size_bytes
);
RaijinResult raijin_read_rgba8(
    RaijinContext* ctx, uint8_t* pixel_buf, uint64_t pixel_capacity
);
RaijinResult raijin_draw_mesh(
    RaijinContext* ctx,
    RaijinMesh mesh,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_draw_cube(
    RaijinContext* ctx,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_draw_plane(
    RaijinContext* ctx,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_draw_disc(
    RaijinContext* ctx,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_draw_cylinder(
    RaijinContext* ctx,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_draw_cone(
    RaijinContext* ctx,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_draw_sphere_uv(
    RaijinContext* ctx,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_draw_line(
    RaijinContext* ctx,
    const float start[3],
    const float end[3],
    float radius,
    const float color[4]
);
/*
 * Expands an assembly into transient mesh submissions.
 *
 * rotation is a column-major 3x3 matrix.
 * Component transforms are multiplied on the left by the parent TRS.
 * Component RGBA colors are multiplied by the parent color.
 *
 * Failure leaves previously queued submissions unchanged and queues
 * none of this assembly's components. Internal capacity may grow.
 *
 * Submit again before each render attempt in which the assembly is visible.
 */
RaijinResult raijin_draw_assembly(
    RaijinContext* ctx,
    RaijinAssembly assembly,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);
RaijinResult raijin_mesh_get_builtin(
    const RaijinContext* ctx, RaijinBuiltinMesh builtin, RaijinMesh* mesh
);
RaijinResult raijin_mesh_create(
    RaijinContext* ctx, const RaijinMeshDesc* desc, RaijinMesh* out_mesh
);
RaijinResult raijin_submit_instances(
    RaijinContext* ctx,
    RaijinMesh mesh,
    const RaijinInstance* instances,
    uint32_t instance_count
);
/*
 * Creates an empty assembly owned by ctx.
 * On failure, *out_assembly is RAIJIN_ASSEMBLY_INVALID.
 *
 * Assemblies remain valid until context destruction.
 */
RaijinResult raijin_assembly_create(
    RaijinContext* ctx, RaijinAssembly* out_assembly
);
/*
 * Copies a mesh reference and its local instance into the assembly.
 * Both handles must belong to ctx. The assembly does not own the mesh.
 *
 * Failure leaves the assembly's logical contents unchanged.
 */
RaijinResult raijin_assembly_add_mesh(
    RaijinContext* ctx,
    RaijinAssembly assembly,
    RaijinMesh mesh,
    const RaijinInstance* local_instance
);

void raijin_instance_init(RaijinInstance* instance);
RaijinResult raijin_instance_init_trs(
    RaijinInstance* instance,
    const float position[3],
    const float rotation[9],
    float scale,
    const float color[4]
);

/*
 * Captures the supported persistent viewer state.
 *
 * The caller owns out_state. No pointers into ctx are retained.
 * On failure, out_state is unchanged.
 *
 * Call on the same thread used for other context operations.
 */
RaijinResult raijin_get_state(const RaijinContext* ctx, RaijinState* out_state);

/*
 * Applies a previously captured or loaded state.
 *
 * The camera view is rebuilt from target, orientation, and distance.
 * Projection horizontal scale is adjusted for the current viewport,
 * matching raijin_camera_set_projection_matrix().
 *
 * Controller tuning, distance limits, and viewport size are unchanged.
 * Distance must be positive and within the destination camera's limits.
 * Invalid state is rejected without modifying ctx.
 *
 * Call on the same thread used for other context operations.
 */
RaijinResult raijin_set_state(RaijinContext* ctx, const RaijinState* state);

/*
 * Writes state using Raijin's versioned binary format.
 *
 * Does not access or modify a context.
 * An existing file is overwritten.
 * On failure, the destination file may be incomplete.
 *
 * Returns INVALID_ARGUMENT for a null state, null path, or empty path;
 * IO for filesystem failures.
 */
RaijinResult raijin_state_save(const RaijinState* state, const char* path);

/*
 * Reads a complete state file without applying it to a context.
 * On failure, out_state is unchanged.
 *
 * This function validates the file structure, not whether the decoded
 * state can be applied to a particular context. Use raijin_set_state()
 * to validate and apply it.
 *
 * Returns INVALID_ARGUMENT for a null path, empty path, or null output;
 * NOT_FOUND if the file does not exist;
 * INVALID_FORMAT for malformed or truncated data;
 * UNSUPPORTED_VERSION for an unrecognized format version;
 * IO for other filesystem failures.
 */
RaijinResult raijin_state_load(const char* path, RaijinState* out_state);

/*
 * Registers a keyboard notification callback for a windowed context.
 * Passing NULL unregisters the callback; user_data is then ignored.
 *
 * Raijin borrows user_data and does not free it. The application must
 * keep it valid while registered and during callback execution.
 *
 * The callback runs synchronously during raijin_poll_events(), on
 * the polling thread. It receives notifications, not control over
 * Raijin's built-in input handling.
 *
 * Returns INVALID_ARGUMENT for a null context, or INVALID_STATE
 * for a headless context.
 */
RaijinResult raijin_set_key_callback(
    RaijinContext* ctx, RaijinKeyCallback callback, void* user_data
);

void raijin_destroy(RaijinContext* ctx);

#ifdef __cplusplus
}
#endif
#endif /* RAIJIN_H_EXTERNAL */
