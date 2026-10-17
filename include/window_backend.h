#ifndef RAIJIN_WINDOW_BACKEND_H
#define RAIJIN_WINDOW_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#include "webgpu.h"

typedef struct {
    void* handle;
    uint32_t width;
    uint32_t height;
    bool should_close;
} RaijinWindow;

typedef enum {
    RAIJIN_WINDOW_EVENT_NONE,
    RAIJIN_WINDOW_EVENT_QUIT,
    RAIJIN_WINDOW_EVENT_RESIZED,
    RAIJIN_WINDOW_EVENT_KEY_DOWN,
    RAIJIN_WINDOW_EVENT_MOUSE_WHEEL,
    RAIJIN_WINDOW_EVENT_MOUSE_BUTTON_DOWN,
    RAIJIN_WINDOW_EVENT_MOUSE_BUTTON_UP,
    RAIJIN_WINDOW_EVENT_MOUSE_MOTION,
} RaijinWindowEventType;

typedef enum {
    RAIJIN_WINDOW_KEY_UNKNOWN,
    RAIJIN_WINDOW_KEY_ESCAPE,
    RAIJIN_WINDOW_KEY_TAB,
} RaijinWindowKey;

typedef enum {
    RAIJIN_WINDOW_MOUSE_BUTTON_UNKNOWN,
    RAIJIN_WINDOW_MOUSE_BUTTON_LEFT,
    RAIJIN_WINDOW_MOUSE_BUTTON_MIDDLE,
    RAIJIN_WINDOW_MOUSE_BUTTON_RIGHT,
} RaijinWindowMouseButton;

typedef struct {
    RaijinWindowEventType type;

    union {
        struct {
            int32_t width;
            int32_t height;
        } resized;

        struct {
            RaijinWindowKey key;
        } key;

        struct {
            float delta;
        } wheel;

        struct {
            RaijinWindowMouseButton button;
            float x;
            float y;
        } mouse_button;

        struct {
            float x;
            float y;
        } mouse_motion;
    } data;
} RaijinWindowEvent;

bool RaijinWindow_init(
    RaijinWindow* window, const char* title, uint32_t width, uint32_t height
);

bool RaijinWindow_poll_event(
    RaijinWindow* window, RaijinWindowEvent* out_event
);

WGPUSurface RaijinWindow_create_surface(
    RaijinWindow* window, WGPUInstance instance
);

void RaijinWindow_destroy(RaijinWindow* window);

#endif /* RAIJIN_WINDOW_BACKEND_H */
