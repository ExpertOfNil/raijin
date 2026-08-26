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

typedef struct RaijinContext RaijinContext;

typedef uint32_t RaijinRenderMode;
#define RAIJIN_RENDER_MODE_HEADLESS ((RaijinRenderMode)0u)
#define RAIJIN_RENDER_MODE_WINDOWED ((RaijinRenderMode)1u)

typedef uint64_t RaijinMesh;
#define RAIJIN_MESH_INVALID ((RaijinMesh)0u)

typedef uint32_t RaijinBuiltinMesh;
#define RAIJIN_BUILTIN_MESH_CUBE ((RaijinBuiltinMesh)1u)

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

RaijinResult raijin_context_create(
    const RaijinContextDesc* desc, RaijinContext** out
);
RaijinResult raijin_context_render(RaijinContext* ctx);
RaijinResult raijin_context_readback_size(
    const RaijinContext* ctx, uint64_t* out_size_bytes
);
RaijinResult raijin_context_read_rgba8(
    RaijinContext* ctx, uint8_t* pixel_buf, uint64_t pixel_capacity
);
RaijinResult raijin_context_draw_cube(
    RaijinContext* ctx, const RaijinInstance* instance
);
RaijinResult raijin_context_get_builtin_mesh(
    const RaijinContext* ctx, RaijinBuiltinMesh builtin, RaijinMesh* mesh
);
RaijinResult raijin_context_submit_instances(
    RaijinContext* ctx,
    RaijinMesh mesh,
    const RaijinInstance* instances,
    uint32_t instance_count
);
void raijin_context_destroy(RaijinContext* ctx);

void raijin_instance_init(RaijinInstance* instance);

#ifdef __cplusplus
}
#endif
#endif /* RAIJIN_H_EXTERNAL */
