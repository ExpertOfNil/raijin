#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "webgpu.h"
#include "renderer.h"
#include "mesh.h"

// Forward declarations
typedef struct GraphicsEngine GraphicsEngine;
typedef struct Renderer Renderer;

// Core structures
typedef struct {
    SDL_Window* window;
    int width;
    int height;
    bool should_quit;
} AppWindow;

typedef struct {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUTextureFormat surface_format;
    WGPUSurfaceConfiguration surface_config;
} WGPUContext;

typedef struct {
    WGPURenderPipeline pipeline;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;
    uint32_t index_count;
    WGPUBindGroup bind_group;
} RenderPipeline;

struct GraphicsEngine {
    AppWindow window;
    WGPUContext wgpu;
    RenderPipeline pipeline;
    bool initialized;
};

// Error handling
static void log_error(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
}

static void log_info(const char* message) { printf("Info: %s\n", message); }

// Window management
static bool window_init(
    AppWindow* window, const char* title, int width, int height
) {
    int log_category = SDL_LOG_CATEGORY_VIDEO;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(log_category, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    window->window =
        SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!window->window) {
        log_error("Failed to create SDL window");
        SDL_LogError(log_category, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    window->width = width;
    window->height = height;
    window->should_quit = false;

    SDL_LogInfo(log_category, "Window initialized successfully");
    return true;
}

static void window_destroy(AppWindow* window) {
    if (window->window) {
        SDL_DestroyWindow(window->window);
        window->window = NULL;
    }
    SDL_Quit();
    log_info("Window destroyed");
}

static void window_handle_events(AppWindow* window) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                window->should_quit = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                window->width = event.window.data1;
                window->height = event.window.data2;
                // TODO: Handle resize - reconfigure surface
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    window->should_quit = true;
                }
                break;
        }
    }
}

// Platform-specific surface creation
#ifdef _WIN32
#include <SDL3/SDL_syswm.h>
static WGPUSurface create_surface_windows(
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
static WGPUSurface create_surface_linux(
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
static WGPUSurface create_surface_macos(
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

static void create_surface(WGPUContext* ctx, SDL_Window* window) {
#ifdef _WIN32
    ctx->surface = create_surface_windows(ctx->instance, window);
#elif defined(__linux__)
    ctx->surface = create_surface_linux(ctx->instance, window);
#elif defined(__APPLE__)
    ctx->surface = create_surface_macos(ctx->instance, window);
#else
#error "Unsupported platform"
#endif
}


// WGPU initialization
static bool wgpu_init(WGPUContext* ctx, SDL_Window* window) {
    // Create WGPU instance
    WGPUInstanceDescriptor instance_desc = {0};
    ctx->instance = wgpuCreateInstance(&instance_desc);
    if (!ctx->instance) {
        log_error("Failed to create WGPU instance");
        return false;
    }

    // Create platform-specific surface
    create_surface(ctx, window);
    if (!ctx->surface) {
        log_error("Failed to create surface");
        return false;
    }

    // Request adapter
    WGPURequestAdapterOptions adapter_options = {
        .compatibleSurface = ctx->surface,
        .powerPreference = WGPUPowerPreference_HighPerformance
    };

    ctx->adapter = NULL;
    WGPURequestAdapterCallbackInfo adapter_cb_info = {
        .callback = adapter_request_callback,
        .userdata1 = &ctx->adapter,
        .userdata2 = NULL,
    };
    wgpuInstanceRequestAdapter(
        ctx->instance, &adapter_options, adapter_cb_info
    );

    // Wait for adapter (in real app, you'd want async handling)
    while (!ctx->adapter) {
        wgpuInstanceProcessEvents(ctx->instance);
    }

    // Request device
    WGPUDeviceDescriptor device_desc = {.label = {"Main Device", WGPU_STRLEN}};

    ctx->device = NULL;
    WGPURequestDeviceCallbackInfo device_cb_info = {
        .callback = device_request_callback,
        .userdata1 = &ctx->device,
        .userdata2 = NULL,
    };
    wgpuAdapterRequestDevice(ctx->adapter, &device_desc, device_cb_info);

    // Wait for device
    while (!ctx->device) {
        wgpuInstanceProcessEvents(ctx->instance);
    }

    // Get queue
    ctx->queue = wgpuDeviceGetQueue(ctx->device);

    // Get preferred surface format
    WGPUSurfaceCapabilities capabilities;
    wgpuSurfaceGetCapabilities(ctx->surface, ctx->adapter, &capabilities);
    ctx->surface_format = capabilities.formats[0];  // Use first available

    log_info("WGPU context initialized successfully");
    return true;
}

static bool wgpu_configure_surface(WGPUContext* ctx, int width, int height) {
    ctx->surface_config =
        (WGPUSurfaceConfiguration){.device = ctx->device,
                                   .usage = WGPUTextureUsage_RenderAttachment,
                                   .format = ctx->surface_format,
                                   .width = width,
                                   .height = height,
                                   .presentMode = WGPUPresentMode_Fifo};

    wgpuSurfaceConfigure(ctx->surface, &ctx->surface_config);
    log_info("Surface configured successfully");
    return true;
}

static void wgpu_destroy(WGPUContext* ctx) {
    if (ctx->queue) wgpuQueueRelease(ctx->queue);
    if (ctx->device) wgpuDeviceRelease(ctx->device);
    if (ctx->adapter) wgpuAdapterRelease(ctx->adapter);
    if (ctx->surface) wgpuSurfaceRelease(ctx->surface);
    if (ctx->instance) wgpuInstanceRelease(ctx->instance);
    log_info("WGPU context destroyed");
}

static bool create_render_pipeline(GraphicsEngine* engine) {
    // Create the vertex buffer
    Vertex vertices[] = {
        {{-0.5f, 0.5f, 0.0f}, {0.5f, 0.0f, 0.5f}},
        {{0.5f, 0.5f, 0.0f}, {0.5f, 0.0f, 0.5f}},
        {{0.5f, -0.5f, 0.0f}, {0.5f, 0.0f, 0.5f}},
        {{-0.5f, -0.5f, 0.0f}, {0.5f, 0.0f, 0.5f}},
    };
    engine->pipeline.vertex_buffer = create_buffer(
        engine->wgpu.device,
        &vertices,
        sizeof(vertices),
        WGPUBufferUsage_Vertex,
        "Vertex Buffer"
    );
    if (!engine->pipeline.vertex_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to create vertex buffer");
        SDL_Quit();
        return false;
    }

    // Create the index buffer
    uint16_t indices[] = {
        // clang-format off
    0, 3, 1,
    1, 3, 2,
        // clang-format on
    };
    engine->pipeline.index_count = sizeof(indices) / sizeof(uint16_t);
    engine->pipeline.index_buffer = create_buffer(
        engine->wgpu.device,
        indices,
        sizeof(indices),
        WGPUBufferUsage_Index,
        "Index Buffer"
    );
    if (!engine->pipeline.index_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Failed to create index buffer");
        SDL_Quit();
        return false;
    }

    // Create shaders
    char* shader_source = load_shader("shaders/color_triangle.wgsl");
    if (!shader_source) {
        return false;
    }

    WGPUShaderSourceWGSL wgsl_desc = {
        .chain.sType = WGPUSType_ShaderSourceWGSL,
        .code = {.data = shader_source, WGPU_STRLEN}
    };
    WGPUShaderModuleDescriptor vs_desc = {
        .nextInChain = &wgsl_desc.chain, .label = {"Vertex Shader", WGPU_STRLEN}
    };
    WGPUShaderModule vertex_shader =
        wgpuDeviceCreateShaderModule(engine->wgpu.device, &vs_desc);

    WGPUShaderModuleDescriptor fs_desc = {
        .nextInChain = &wgsl_desc.chain,
        .label = {"Fragment Shader", WGPU_STRLEN}
    };
    WGPUShaderModule fragment_shader =
        wgpuDeviceCreateShaderModule(engine->wgpu.device, &fs_desc);

    // Define vertex attributes
    WGPUVertexAttribute vertex_attributes[] = {
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = offsetof(Vertex, position),
            .shaderLocation = 0,
        },
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = offsetof(Vertex, color),
            .shaderLocation = 1,
        },
    };

    // Define vertex buffer layout
    WGPUVertexBufferLayout vertex_buffer_layout = {
        .arrayStride = sizeof(Vertex),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 2,
        .attributes = vertex_attributes,
    };

    WGPUColorTargetState color_target_state = {
        .format = engine->wgpu.surface_format,
        .writeMask = WGPUColorWriteMask_All,
    };
    WGPUFragmentState frag_state = {
        .module = fragment_shader,
        .entryPoint = {"fs_main", WGPU_STRLEN},
        .targetCount = 1,
        .targets = &color_target_state,
    };
    // Create render pipeline
    WGPURenderPipelineDescriptor pipeline_desc = {
        .label = {"Basic Pipeline", WGPU_STRLEN},
        .vertex =
            {
                .module = vertex_shader,
                .entryPoint = {"vs_main", WGPU_STRLEN},
                .bufferCount = 1,
                .buffers = &vertex_buffer_layout,
            },
        .fragment = &frag_state,
        .primitive = {.topology = WGPUPrimitiveTopology_TriangleList},
        .multisample = {.count = 1, .mask = 0xFFFFFFFF}
    };

    engine->pipeline.pipeline =
        wgpuDeviceCreateRenderPipeline(engine->wgpu.device, &pipeline_desc);

    // Clean up shaders
    wgpuShaderModuleRelease(vertex_shader);
    wgpuShaderModuleRelease(fragment_shader);
    free(shader_source);

    if (!engine->pipeline.pipeline) {
        log_error("Failed to create render pipeline");
        wgpuBufferRelease(engine->pipeline.vertex_buffer);
        wgpuBufferRelease(engine->pipeline.index_buffer);
        return false;
    }

    log_info("Render pipeline created successfully");
    return true;
}

// Main graphics engine functions
GraphicsEngine* graphics_engine_create(
    const char* title, int width, int height
) {
    GraphicsEngine* engine = malloc(sizeof(GraphicsEngine));
    if (!engine) {
        log_error("Failed to allocate graphics engine");
        return NULL;
    }

    memset(engine, 0, sizeof(GraphicsEngine));

    // Initialize window
    if (!window_init(&engine->window, title, width, height)) {
        free(engine);
        return NULL;
    }

    // Initialize WGPU
    if (!wgpu_init(&engine->wgpu, engine->window.window)) {
        window_destroy(&engine->window);
        free(engine);
        return NULL;
    }

    // Create swap chain
    if (!wgpu_configure_surface(&engine->wgpu, width, height)) {
        wgpu_destroy(&engine->wgpu);
        window_destroy(&engine->window);
        free(engine);
        return NULL;
    }

    // Create basic render pipeline
    if (!create_render_pipeline(engine)) {
        wgpu_destroy(&engine->wgpu);
        window_destroy(&engine->window);
        free(engine);
        return NULL;
    }

    engine->initialized = true;
    log_info("Graphics engine created successfully");
    return engine;
}

void graphics_engine_destroy(GraphicsEngine* engine) {
    if (!engine) return;

    if (engine->pipeline.vertex_buffer) {
        wgpuBufferRelease(engine->pipeline.vertex_buffer);
    }
    if (engine->pipeline.index_buffer) {
        wgpuBufferRelease(engine->pipeline.index_buffer);
    }
    if (engine->pipeline.pipeline) {
        wgpuRenderPipelineRelease(engine->pipeline.pipeline);
    }

    wgpu_destroy(&engine->wgpu);
    window_destroy(&engine->window);
    free(engine);
    log_info("Graphics engine destroyed");
}

static void render_frame(GraphicsEngine* engine) {
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(engine->wgpu.surface, &surface_texture);

    if (surface_texture.status !=
        WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
        log_error("Failed to get current surface texture");
        return;
    }

    WGPUTextureView back_buffer =
        wgpuTextureCreateView(surface_texture.texture, NULL);
    if (!back_buffer) {
        log_error("Failed to create texture view");
        return;
    }

    WGPUCommandEncoderDescriptor cmd_encoder_desc = {
        .label = {"Command Encoder", WGPU_STRLEN}
    };
    WGPUCommandEncoder encoder =
        wgpuDeviceCreateCommandEncoder(engine->wgpu.device, &cmd_encoder_desc);

    WGPURenderPassColorAttachment color_attachment = {
        .view = back_buffer,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = {0.1, 0.1, 0.1, 1.0}  // Dark gray background
    };

    WGPURenderPassDescriptor render_pass_desc = {
        .label = {"Main Render Pass", WGPU_STRLEN},
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment
    };

    WGPURenderPassEncoder pass =
        wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);

    // Set pipeline and vertex buffer
    wgpuRenderPassEncoderSetPipeline(pass, engine->pipeline.pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(
        pass, 0, engine->pipeline.vertex_buffer, 0, WGPU_WHOLE_SIZE
    );
    wgpuRenderPassEncoderSetIndexBuffer(
        pass,
        engine->pipeline.index_buffer,
        WGPUIndexFormat_Uint16,
        0,
        WGPU_WHOLE_SIZE
    );

    // Draw using vertex buffer and index buffer
    wgpuRenderPassEncoderDrawIndexed(
        pass, engine->pipeline.index_count, 1, 0, 0, 0
    );

    wgpuRenderPassEncoderEnd(pass);

    WGPUCommandBufferDescriptor cmd_buffer_desc = {
        .label = {"Command Buffer", WGPU_STRLEN}
    };
    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
    wgpuQueueSubmit(engine->wgpu.queue, 1, &command_buffer);

    wgpuSurfacePresent(engine->wgpu.surface);

    // Clean up
    wgpuCommandBufferRelease(command_buffer);
    wgpuRenderPassEncoderRelease(pass);
    wgpuCommandEncoderRelease(encoder);
    wgpuTextureViewRelease(back_buffer);
}

void graphics_engine_run(GraphicsEngine* engine) {
    if (!engine || !engine->initialized) {
        log_error("Graphics engine not properly initialized");
        return;
    }

    log_info("Starting main loop");
    while (!engine->window.should_quit) {
        window_handle_events(&engine->window);
        render_frame(engine);
    }
    log_info("Main loop ended");
}
