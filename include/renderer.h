#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>

#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "cimpl_core.h"
#include "core.h"
#include "mesh.h"
#include "webgpu.h"
#include "wgpu.h"

/* Types */

enum {
    BUILTIN_PLANE,
    BUILTIN_DISC,
    BUILTIN_CUBE,
    BUILTIN_CYLINDER,
    BUILTIN_CONE,
    BUILTIN_SPHERE_UV,
    BUILTIN_COUNT,
};

typedef enum {
    RENDER_MODE_HEADLESS,
    RENDER_MODE_WINDOWED,
} RenderMode;

typedef enum {
    RENDERER_SURFACE_RECOVERY_IDLE,
    RENDERER_SURFACE_RECOVERY_RECONFIGURED,
    RENDERER_SURFACE_RECOVERY_REPLACED,
} RendererSurfaceRecoveryState;

typedef struct WgpuAdapterRequestContext {
    bool completed;
    WGPURequestAdapterStatus status;
    WGPUAdapter adapter;
} WgpuAdapterRequestContext;

typedef struct WgpuDeviceRequestContext {
    bool completed;
    WGPURequestDeviceStatus status;
    WGPUDevice device;
} WgpuDeviceRequestContext;

typedef struct WgpuDeviceLostContext {
    bool completed;
    WGPUDeviceLostReason reason;
} WgpuDeviceLostContext;

typedef struct WgpuBufferMapContext {
    bool active;
    bool completed;
    bool unmap_requested;
    WGPUMapAsyncStatus status;
    WGPUBuffer staging_buffer;
} WgpuBufferMapContext;

typedef struct Renderer {
    bool enable_edges;
    float clear_color[4];
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    RenderMode render_mode;
    WGPUInstance instance;
    union {
        struct {
            WGPUTexture texture;
        } headless;
        struct {
            WGPUSurface surface;
            WGPUSurfaceConfiguration surface_config;
            RendererSurfaceRecoveryState recovery_state;
        } windowed;
    } render_target;
    WGPURenderPipeline solid_pipeline;
    WGPURenderPipeline edges_pipeline;
    WGPUBuffer uniform_buffer;
    WGPUBindGroup uniform_bind_group;
    WGPUTexture depth_texture;
    WGPUTextureView depth_texture_view;
    WgpuWaitConfig wait_config;
    WgpuAdapterRequestContext adapter_request;
    WgpuDeviceRequestContext device_request;
    WgpuDeviceLostContext device_lost;
    WGPUFuture device_lost_future;
    bool device_lost_future_available;
    WgpuBufferMapContext buffer_map;
    MeshHandleArray active_meshes;
    MeshArray meshes;
    struct {
        // MeshHandle triangle;
        // MeshHandle tetrahedron;
        MeshHandle plane;
        MeshHandle disc;
        MeshHandle cube;
        MeshHandle cylinder;
        MeshHandle cone;
        MeshHandle sphere_uv;
    } builtin;
} Renderer;

/* Function Prototypes */

MeshHandle Renderer_register_mesh(
    Renderer* renderer, const MeshGeometryView* geometry
);
ReturnStatus Renderer_init_windowed(
    Renderer* renderer,
    const WGPUInstance instance,
    const WGPUSurface surface,
    const u32 width,
    const u32 height
);
ReturnStatus Renderer_init_headless(
    Renderer* renderer, const WGPUInstance instance, u32 width, u32 height
);
ReturnStatus Renderer_render(Renderer* renderer);
ReturnStatus Renderer_handle_resize(Renderer* renderer, u32 width, u32 height);
void Renderer_update_uniforms(
    Renderer* renderer, mat4 proj_matrix, mat4 view_matrix
);
ReturnStatus Renderer_copy_frame_to_buffer(
    Renderer* renderer, u32 width, u32 height, u8* buffer, u64 buffer_capacity
);
ReturnStatus Renderer_submit_instances(
    Renderer* renderer,
    MeshHandle mesh_handle,
    const Instance* instances,
    u32 instance_count
);
void Renderer_clear_submissions(Renderer* renderer);
void Renderer_release_surface(Renderer* renderer);
ReturnStatus Renderer_adopt_surface(
    Renderer* renderer, WGPUSurface new_surface, uint32_t width, uint32_t height
);
ReturnStatus Renderer_reconfigure_surface(
    Renderer* renderer, uint32_t width, uint32_t height
);
void Renderer_destroy(Renderer* renderer);

#endif /* RENDERER_H */
