#include <stddef.h>

#include "window_backend.h"

bool RaijinWindow_init(
    RaijinWindow* window, const char* title, uint32_t width, uint32_t height
) {
    (void)title;
    (void)width;
    (void)height;
    if (window != NULL) {
        *window = (RaijinWindow){0};
    }
    return false;
}

bool RaijinWindow_poll_event(
    RaijinWindow* window, RaijinWindowEvent* out_event
) {
    (void)window;

    if (out_event != NULL) {
        *out_event = (RaijinWindowEvent){
            .type = RAIJIN_WINDOW_EVENT_NONE,
        };
    }

    return false;
}

WGPUSurface RaijinWindow_create_surface(
    RaijinWindow* window, WGPUInstance instance
) {
    (void)window;
    (void)instance;
    return NULL;
}

bool RaijinWindow_get_pixel_size(const RaijinWindow *window, uint32_t *out_width, uint32_t *out_height) {
    (void)window;

    if (out_width != NULL) *out_width = 0;
    if (out_height != NULL) *out_height = 0;

    return false;
}

void RaijinWindow_destroy(RaijinWindow* window) {
    if (window != NULL) {
        *window = (RaijinWindow){0};
    }
}
