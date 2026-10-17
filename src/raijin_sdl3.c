#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <limits.h>

#include "logging.h"
#include "window_backend.h"

#ifdef __linux__
#include <X11/Xlib.h>
#include <wayland-client-core.h>
#endif

static RaijinWindowKey translate_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_ESCAPE:
            return RAIJIN_WINDOW_KEY_ESCAPE;
        case SDLK_TAB:
            return RAIJIN_WINDOW_KEY_TAB;
        default:
            return RAIJIN_WINDOW_KEY_UNKNOWN;
    }
}

static RaijinWindowMouseButton translate_mouse_button(uint8_t button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return RAIJIN_WINDOW_MOUSE_BUTTON_LEFT;
        case SDL_BUTTON_MIDDLE:
            return RAIJIN_WINDOW_MOUSE_BUTTON_MIDDLE;
        case SDL_BUTTON_RIGHT:
            return RAIJIN_WINDOW_MOUSE_BUTTON_RIGHT;
        default:
            return RAIJIN_WINDOW_MOUSE_BUTTON_UNKNOWN;
    }
}

bool RaijinWindow_init(
    RaijinWindow* window, const char* title, uint32_t width, uint32_t height
) {
    if (window == NULL || width == 0 || height == 0) return false;
    if (width > (uint32_t)INT_MAX || height > (uint32_t)INT_MAX) return false;

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        log_error("SDL_InitSubSystem: %s\n", SDL_GetError());
        return false;
    }

    SDL_Window* handle =
        SDL_CreateWindow(title, (int)width, (int)height, SDL_WINDOW_RESIZABLE);
    if (handle == NULL) {
        log_error("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    window->handle = handle;
    window->width = width;
    window->height = height;
    window->should_close = false;
    return true;
}

bool RaijinWindow_poll_event(
    RaijinWindow* window, RaijinWindowEvent* out_event
) {
    if (window == NULL || window->handle == NULL || out_event == NULL) {
        return false;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        RaijinWindowEvent translated = {0};

        switch (event.type) {
            case SDL_EVENT_QUIT:
                translated.type = RAIJIN_WINDOW_EVENT_QUIT;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                translated.type = RAIJIN_WINDOW_EVENT_RESIZED;
                translated.data.resized.width = event.window.data1;
                translated.data.resized.height = event.window.data2;
                break;
            case SDL_EVENT_KEY_DOWN:
                translated.type = RAIJIN_WINDOW_EVENT_KEY_DOWN;
                translated.data.key.key = translate_key(event.key.key);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                translated.type = RAIJIN_WINDOW_EVENT_MOUSE_WHEEL;
                translated.data.wheel.delta = event.wheel.y;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                translated.type = RAIJIN_WINDOW_EVENT_MOUSE_BUTTON_DOWN;
                translated.data.mouse_button.button =
                    translate_mouse_button(event.button.button);
                translated.data.mouse_button.x = event.button.x;
                translated.data.mouse_button.y = event.button.y;
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                translated.type = RAIJIN_WINDOW_EVENT_MOUSE_BUTTON_UP;
                translated.data.mouse_button.button =
                    translate_mouse_button(event.button.button);
                translated.data.mouse_button.x = event.button.x;
                translated.data.mouse_button.y = event.button.y;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                translated.type = RAIJIN_WINDOW_EVENT_MOUSE_MOTION;
                translated.data.mouse_motion.x = event.motion.x;
                translated.data.mouse_motion.y = event.motion.y;
                break;
            default:
                continue;
        }

        *out_event = translated;
        return true;
    }
    return false;
}

WGPUSurface RaijinWindow_create_surface(
    RaijinWindow* window, WGPUInstance instance
) {
    if (window == NULL || window->handle == NULL || instance == NULL) {
        return NULL;
    }

#if defined(__linux__)
    SDL_Window* sdl_window = (SDL_Window*)window->handle;
    const char* driver = SDL_GetCurrentVideoDriver();
    if (driver == NULL) {
        log_error("SDL has no active video driver\n");
        return NULL;
    }
    SDL_PropertiesID properties = SDL_GetWindowProperties(sdl_window);
    SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "SDL video driver: %s\n", driver);

    /* Get WGPU surface */
    if (SDL_strcmp(driver, "x11") == 0) {
        Display* x11_display = SDL_GetPointerProperty(
            properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL
        );
        if (!x11_display) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to get X11 display");
            return NULL;
        }
        Window x11_window = SDL_GetNumberProperty(
            properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0
        );
        if (!x11_window) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to get X11 window");
            return NULL;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Using X11 backend");
        WGPUSurfaceSourceXlibWindow x11_surface_src = {
            .chain =
                {
                    .sType = WGPUSType_SurfaceSourceXlibWindow,
                    .next = NULL,
                },
            .display = x11_display,
            .window = x11_window,
        };
        WGPUSurfaceDescriptor surface_desc = {
            .nextInChain = (const WGPUChainedStruct*)&x11_surface_src,
            .label = {"X11 Surface", WGPU_STRLEN},
        };
        WGPUSurface surface =
            wgpuInstanceCreateSurface(instance, &surface_desc);
        if (surface == NULL) {
            SDL_LogError(
                SDL_LOG_CATEGORY_VIDEO, "Failed to create WGPU surface"
            );
            return NULL;
        }
        SDL_LogInfo(
            SDL_LOG_CATEGORY_VIDEO, "Successfully created WGPU surface"
        );
        return surface;
    }

    if (SDL_strcmp(driver, "wayland") == 0) {
        struct wl_display* wl_display = SDL_GetPointerProperty(
            properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL
        );
        if (!wl_display) {
            SDL_LogError(
                SDL_LOG_CATEGORY_VIDEO, "Failed to get Wayland display"
            );
            return NULL;
        }
        struct wl_surface* wl_surface = SDL_GetPointerProperty(
            properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, 0
        );
        if (!wl_surface) {
            SDL_LogError(
                SDL_LOG_CATEGORY_VIDEO, "Failed to get Wayland surface"
            );
            return NULL;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Using Wayland backend");
        WGPUSurfaceSourceWaylandSurface wl_surface_src = {
            .chain =
                {
                    .sType = WGPUSType_SurfaceSourceWaylandSurface,
                    .next = NULL,
                },
            .display = wl_display,
            .surface = wl_surface,
        };
        WGPUSurfaceDescriptor surface_desc = {
            .nextInChain = (const WGPUChainedStruct*)&wl_surface_src,
            .label = {"Wayland Surface", WGPU_STRLEN},
        };
        WGPUSurface surface =
            wgpuInstanceCreateSurface(instance, &surface_desc);
        if (surface == NULL) {
            SDL_LogError(
                SDL_LOG_CATEGORY_VIDEO, "Failed to create WGPU surface"
            );
            return NULL;
        }
        SDL_LogInfo(
            SDL_LOG_CATEGORY_VIDEO, "Successfully created WGPU surface"
        );
        return surface;
    }

    SDL_LogError(
        SDL_LOG_CATEGORY_VIDEO, "Failed to get linux native window handle"
    );
    return NULL;
#else
#error "Raijin SDL backend currently supports Linux only"
#endif
}

void RaijinWindow_destroy(RaijinWindow* window) {
    if (window == NULL || window->handle == NULL) return;

    SDL_DestroyWindow((SDL_Window*)window->handle);
    window->handle = NULL;
    window->width = 0;
    window->height = 0;
    window->should_close = false;

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
