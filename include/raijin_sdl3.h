#ifndef RAIJIN_SDL3_H
#define RAIJIN_SDL3_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "mesh.h"
#include "renderer.h"
#include "webgpu.h"

// Core structures
typedef struct {
    SDL_Window* handle;
    int width;
    int height;
    bool should_close;
} SdlWindow;

ReturnStatus SdlWindow_init(
    SdlWindow* window, const char* title, int width, int height
) {
    int log_category = SDL_LOG_CATEGORY_VIDEO;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR("SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    window->handle =
        SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!window->handle) {
        LOG_ERROR("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    window->width = width;
    window->height = height;
    window->should_close = false;

    LOG_INFO("SDL window initialized successfully");
    return true;
}

void SdlWindow_destroy(SdlWindow* window) {
    if (window->handle) {
        SDL_DestroyWindow(window->handle);
        window->handle = NULL;
    }
    SDL_Quit();
    LOG_INFO("Window destroyed");
}

void SdlWindow_handle_events(SdlWindow* window, Renderer* renderer) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                window->should_close = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                window->width = event.window.data1;
                window->height = event.window.data2;
                Renderer_handle_resize(renderer, window->width, window->height);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    window->should_close = true;
                }
                break;
        }
    }
}

// Platform-specific surface creation
#ifdef _WIN32
#include <SDL3/SDL_syswm.h>
static WGPUSurface create_surface_sdl3_win32(
    WGPUInstance instance, SDL_Window* window
) {
    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    if (!SDL_GetWindowWMInfo(window, &wm_info)) {
        return NULL;
    }

    WGPUSurfaceDescriptorFromWindowsHWND native_desc = {
        .chain = {.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND},
        .hinstance = GetModuleHandle(NULL),
        .hwnd = wm_info.info.win.window
    };

    WGPUSurfaceDescriptor surface_desc = {.nextInChain = &native_desc.chain};

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <wayland-client-core.h>
static WGPUSurface create_surface_sdl3_linux(
    WGPUInstance instance, SDL_Window* window
) {
    const char* driver = SDL_GetCurrentVideoDriver();
    SDL_PropertiesID win_props = SDL_GetWindowProperties(window);
    SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "SDL video driver: %s\n", driver);

    /* Get WGPU surface */
    if (SDL_strcmp(driver, "x11") == 0) {
        Display* x11_display = SDL_GetPointerProperty(
            win_props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL
        );
        if (!x11_display) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to get X11 display");
            return NULL;
        }
        Window x11_window = SDL_GetNumberProperty(
            win_props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0
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
        SDL_LogInfo(
            SDL_LOG_CATEGORY_VIDEO, "Successfully created WGPU surface"
        );
        return wgpuInstanceCreateSurface(instance, &surface_desc);
    }

    if (SDL_strcmp(driver, "wayland") == 0) {
        struct wl_display* wl_display = SDL_GetPointerProperty(
            win_props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL
        );
        if (!wl_display) {
            SDL_LogError(
                SDL_LOG_CATEGORY_VIDEO, "Failed to get Wayland display"
            );
            return NULL;
        }
        struct wl_surface* wl_surface = SDL_GetPointerProperty(
            win_props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, 0
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
        SDL_LogInfo(
            SDL_LOG_CATEGORY_VIDEO, "Successfully created WGPU surface"
        );
        return wgpuInstanceCreateSurface(instance, &surface_desc);
    }

    SDL_LogError(
        SDL_LOG_CATEGORY_VIDEO, "Failed to get linux native window handle"
    );
    return NULL;
}
#elif defined(__APPLE__)
#include <SDL3/SDL_syswm.h>
static WGPUSurface create_surface_sdl3_macos(
    WGPUInstance instance, SDL_Window* window
) {
    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    if (!SDL_GetWindowWMInfo(window, &wm_info)) {
        return NULL;
    }

    // Create metal layer and attach to NSView
    void* metal_layer = NULL;  // You'll need to create CAMetalLayer here

    WGPUSurfaceDescriptorFromMetalLayer native_desc = {
        .chain = {.sType = WGPUSType_SurfaceDescriptorFromMetalLayer},
        .layer = metal_layer
    };

    WGPUSurfaceDescriptor surface_desc = {.nextInChain = &native_desc.chain};

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#endif

static WGPUSurface create_surface_sdl3(
    WGPUInstance instance, SDL_Window* window
) {
#ifdef _WIN32
    return create_surface_sdl3_windows(instance, window);
#elif defined(__linux__)
    return create_surface_sdl3_linux(instance, window);
#elif defined(__APPLE__)
    return create_surface_sdl3_macos(instance, window);
#else
#error "Unsupported platform"
#endif
}

#endif /* RAIJIN_SDL3_H */
