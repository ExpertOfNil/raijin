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

typedef struct RaijinContext RaijinContext;

typedef uint32_t RaijinRenderMode;
#define RAIJIN_RENDER_MODE_HEADLESS ((RaijinRenderMode)0u)
#define RAIJIN_RENDER_MODE_WINDOWED ((RaijinRenderMode)1u)

typedef struct RaijinContextDesc {
    const char* title;
    uint32_t width;
    uint32_t height;
    RaijinRenderMode render_mode;
} RaijinContextDesc;

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
void raijin_context_destroy(RaijinContext* ctx);

#ifdef __cplusplus
}
#endif
#endif /* RAIJIN_H_EXTERNAL */
