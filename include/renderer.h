#ifndef RENDERER_H
#define RENDERER_H

#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "core.h"
#include "mesh.h"
#include "webgpu.h"
#include "wgpu.h"

/* Types */

typedef struct Uniform {
    mat4 view_proj;
    vec3 view_pos;
    f32 _pad;
} Uniform;

typedef struct DrawCommand {
    MeshHandle mesh_handle;
    Instance instance;
} DrawCommand;
DEFINE_DYNAMIC_ARRAY(DrawCommand, DrawCommandArray)

typedef enum {
    RENDER_MODE_HEADLESS,
    RENDER_MODE_WINDOWED,
} RenderMode;

typedef struct WgpuCallbackContext {
    bool completed;
    bool success;
    WGPUAdapter* adapter;
    WGPUDevice* device;
} WgpuCallbackContext;

typedef struct WgpuBufferMapContext {
    WGPUMapAsyncStatus status;
    bool completed;
    WGPUStringView message;
} WgpuBufferMapContext;

typedef struct Renderer {
    bool enable_edges;
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
        } windowed;
    } render_target;
    WGPURenderPipeline solid_pipeline;
    WGPURenderPipeline edges_pipeline;
    WGPUBuffer uniform_buffer;
    WGPUBindGroup uniform_bind_group;
    WGPUTexture depth_texture;
    WGPUTextureView depth_texture_view;
    DrawCommandArray draw_commands;
    MeshArray meshes;
    struct {
        // MeshHandle triangle;
        // MeshHandle tetrahedron;
        MeshHandle cube;
        MeshHandle sphere_uv;
        MeshHandle plane;
        MeshHandle disc;
        MeshHandle cylinder;
        MeshHandle cone;
    } builtin;
} Renderer;

/* Function Prototypes */

void Renderer_create_mesh_buffers(Mesh* mesh, Renderer* renderer);
MeshHandle Renderer_register_mesh(Renderer* renderer, Mesh* mesh_template);
void Renderer_create_depth_texture(
    Renderer* renderer,
    u32 width,
    u32 height,
    const char* label,
    WGPUTextureFormat format
);
ReturnStatus Renderer_create_render_pipeline(
    Renderer* renderer,
    WGPURenderPipeline* pipeline,
    const char* label,
    const char* shader_path,
    const WGPUTextureFormat texture_format,
    const WGPUTextureFormat depth_texture_format,
    const WGPUBindGroupLayout bind_group_layout,
    const WGPUPrimitiveTopology topology,
    const WGPUCullMode cull_mode
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
void Renderer_render_mesh(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
);
void Renderer_render_mesh_edges(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
);
void Renderer_render_pass_solid(
    Renderer* renderer,
    const WGPUCommandEncoder command_encoder,
    const WGPUTextureView texture_view
);
void Renderer_render_pass_edges(
    Renderer* renderer,
    const WGPUCommandEncoder command_encoder,
    const WGPUTextureView texture_view
);
void Renderer_render_to_view(
    Renderer* renderer, const WGPUTextureView texture_view
);
ReturnStatus Renderer_render(Renderer* renderer);
void Renderer_destroy(Renderer* renderer);
void Renderer_handle_resize(Renderer* renderer, u32 width, u32 height);
void Renderer_update_uniforms(
    Renderer* renderer, mat4 proj_matrix, mat4 view_matrix
);
ReturnStatus Renderer_copy_frame_to_buffer(
    Renderer* renderer, u32 width, u32 height, u8* buffer, u32 buffer_capacity
);

void adapter_request_callback(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
);

void device_request_callback(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
);

void buffer_map_callback(
    WGPUMapAsyncStatus status,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
);

/* Functions */

void Renderer_create_mesh_buffers(Mesh* mesh, Renderer* renderer) {
    mesh->vertex_buffer = create_buffer(
        renderer->device,
        mesh->vertices.count * sizeof(Vertex),
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Vertex Buffer"
    );

    wgpuQueueWriteBuffer(
        renderer->queue,
        mesh->vertex_buffer,
        0,
        mesh->vertices.items,
        mesh->vertices.count * sizeof(Vertex)
    );

    mesh->instance_buffer = create_buffer(
        renderer->device,
        mesh->instance_capacity * sizeof(Instance),
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Instance Buffer"
    );

    mesh->index_buffer = create_buffer(
        renderer->device,
        mesh->indices.count * sizeof(u32),
        WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
        "Index Buffer"
    );

    wgpuQueueWriteBuffer(
        renderer->queue,
        mesh->index_buffer,
        0,
        mesh->indices.items,
        mesh->indices.count * sizeof(u32)
    );

    mesh->edge_instance_buffer = create_buffer(
        renderer->device,
        mesh->edge_instance_capacity * sizeof(Instance),
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Edge Instance Buffer"
    );

    mesh->edge_index_buffer = create_buffer(
        renderer->device,
        mesh->edge_indices.count * sizeof(u32),
        WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
        "Edge Index Buffer"
    );

    wgpuQueueWriteBuffer(
        renderer->queue,
        mesh->edge_index_buffer,
        0,
        mesh->edge_indices.items,
        mesh->edge_indices.count * sizeof(u32)
    );
}

MeshHandle Renderer_register_mesh(Renderer* renderer, Mesh* mesh_template) {
    Mesh mesh = {0};
    mesh.vertices = mesh_template->vertices;
    mesh.indices = mesh_template->indices;
    mesh.edge_indices = mesh_template->edge_indices;
    mesh.instance_capacity = DEFAULT_INSTANCE_CAPACITY;
    mesh.edge_instance_capacity = DEFAULT_INSTANCE_CAPACITY;

    Renderer_create_mesh_buffers(&mesh, renderer);
    MeshArray_push(&renderer->meshes, mesh);
    // Handle = index
    return renderer->meshes.count - 1;
}

static void Renderer_register_builtin_meshes(Renderer* renderer) {
    // TODO (mmckenna): missing TRIANGLE
    // TODO (mmckenna): missing TETRAHEDRON
    Mesh cube_mesh = {0};
    Mesh_create_cube(&cube_mesh);
    renderer->builtin.cube = Renderer_register_mesh(renderer, &cube_mesh);

    Mesh sphere_uv_mesh = {0};
    Mesh_create_sphere_uv(&sphere_uv_mesh, 16);
    renderer->builtin.sphere_uv =
        Renderer_register_mesh(renderer, &sphere_uv_mesh);

    Mesh plane_mesh = {0};
    Mesh_create_plane(&plane_mesh);
    renderer->builtin.plane = Renderer_register_mesh(renderer, &plane_mesh);

    Mesh disc_mesh = {0};
    Mesh_create_disc(&disc_mesh, 32);
    renderer->builtin.disc = Renderer_register_mesh(renderer, &disc_mesh);

    Mesh cylinder_mesh = {0};
    Mesh_create_cylinder(&cylinder_mesh, 16);
    renderer->builtin.cylinder =
        Renderer_register_mesh(renderer, &cylinder_mesh);

    Mesh cone_mesh = {0};
    Mesh_create_cone(&cone_mesh, 16);
    renderer->builtin.cone = Renderer_register_mesh(renderer, &cone_mesh);
}

void Renderer_create_depth_texture(
    Renderer* renderer,
    u32 width,
    u32 height,
    const char* label,
    WGPUTextureFormat format
) {
    WGPUTextureDescriptor depth_texture_desc = {
        .label = {label, WGPU_STRLEN},
        .size =
            (WGPUExtent3D){
                .width = width > 0 ? width : 256,
                .height = height > 0 ? height : 256,
                .depthOrArrayLayers = 1,
            },
        .mipLevelCount = 1,
        .sampleCount = 1,
        .dimension = WGPUTextureDimension_2D,
        .format = format,
        .usage = WGPUTextureUsage_RenderAttachment,
        .viewFormats = &format,
        .viewFormatCount = 1,
    };
    if (renderer->depth_texture != NULL) {
        wgpuTextureRelease(renderer->depth_texture);
    }
    renderer->depth_texture =
        wgpuDeviceCreateTexture(renderer->device, &depth_texture_desc);

    if (renderer->depth_texture_view != NULL) {
        wgpuTextureViewRelease(renderer->depth_texture_view);
    }
    char view_label[256] = {0};
    snprintf(view_label, sizeof(view_label), "%s View", label);
    WGPUTextureViewDescriptor depth_texture_view_desc = {
        .label = {view_label, WGPU_STRLEN},
        .format = format,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
    };
    renderer->depth_texture_view = wgpuTextureCreateView(
        renderer->depth_texture, &depth_texture_view_desc
    );
}

// Expects "vs_main" and "fs_main"
ReturnStatus Renderer_create_render_pipeline(
    Renderer* renderer,
    WGPURenderPipeline* pipeline,
    const char* label,
    const char* shader_path,
    const WGPUTextureFormat texture_format,
    const WGPUTextureFormat depth_texture_format,
    const WGPUBindGroupLayout bind_group_layout,
    const WGPUPrimitiveTopology topology,
    const WGPUCullMode cull_mode
) {
    String shader_src = {0};
    ReturnStatus shader_load_status = load_shader(shader_path, &shader_src);
    if (shader_load_status != RETURN_SUCCESS) {
        log_error("Failed to load shader");
        return RETURN_FAILURE;
    }
    char label_buffer[256] = {0};
    snprintf(label_buffer, sizeof(label_buffer), "%s Shader Module", label);
    WGPUShaderSourceWGSL wgsl_desc = {
        .chain.sType = WGPUSType_ShaderSourceWGSL,
        .code = {.data = shader_src.items, shader_src.count}
    };
    WGPUShaderModuleDescriptor shader_desc = {
        .nextInChain = &wgsl_desc.chain,
        .label = {label_buffer, WGPU_STRLEN},
    };
    WGPUShaderModule shader =
        wgpuDeviceCreateShaderModule(renderer->device, &shader_desc);
    WGPUVertexBufferLayout vertex_buffer_layouts[] = {
        Vertex_desc(),
        Instance_desc(),
    };
    WGPUVertexState vert_state = {
        .module = shader,
        .entryPoint = {"vs_main", WGPU_STRLEN},
        .bufferCount = 2,
        .buffers = vertex_buffer_layouts,
    };

    WGPUBlendState blend_state = {
        .color =
            (WGPUBlendComponent){
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            },
        .alpha = (WGPUBlendComponent){
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_One,
            .dstFactor = WGPUBlendFactor_Zero,
        },
    };
    WGPUColorTargetState color_target_state = {
        .format = texture_format,
        .blend = &blend_state,
        .writeMask = WGPUColorWriteMask_All,
    };
    WGPUFragmentState frag_state = {
        .module = shader,
        .entryPoint = {"fs_main", WGPU_STRLEN},
        .targets = &color_target_state,
        .targetCount = 1,
    };
    WGPUDepthStencilState depth_pencil_state = {
        .format = depth_texture_format,
        .depthWriteEnabled = true,
        .depthCompare = WGPUCompareFunction_Less,
    };
    snprintf(label_buffer, sizeof(label_buffer), "%s Layout", label);
    WGPUPipelineLayoutDescriptor solid_pipeline_layout_desc = {
        .label = {label_buffer, WGPU_STRLEN},
        .bindGroupLayouts = &bind_group_layout,
        .bindGroupLayoutCount = 1,
    };
    WGPURenderPipelineDescriptor solid_pipeline_desc = {
        .label = {label, WGPU_STRLEN},
        .layout = wgpuDeviceCreatePipelineLayout(
            renderer->device, &solid_pipeline_layout_desc
        ),
        .vertex = vert_state,
        .fragment = &frag_state,
        .depthStencil = &depth_pencil_state,
        .primitive =
            (WGPUPrimitiveState){
                .topology = topology,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = cull_mode,
                .unclippedDepth = false,
            },
        .multisample = (WGPUMultisampleState){
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = false,
        },
    };
    *pipeline =
        wgpuDeviceCreateRenderPipeline(renderer->device, &solid_pipeline_desc);
    String_free(&shader_src);
    return RETURN_SUCCESS;
}

ReturnStatus Renderer_init_windowed(
    Renderer* renderer,
    const WGPUInstance instance,
    const WGPUSurface surface,
    const u32 width,
    const u32 height
) {
    renderer->render_mode = RENDER_MODE_WINDOWED;
    WgpuCallbackContext cb_ctx = {
        .completed = false,
        .adapter = &renderer->adapter,
        .device = &renderer->device,
    };

    renderer->instance = instance;
    renderer->render_target.windowed.surface = surface;

    // Adapter request
    if (renderer->adapter != NULL) {
        wgpuAdapterRelease(renderer->adapter);
    }
    WGPURequestAdapterOptions adapter_options = {
        // Don't need a surface
        .compatibleSurface = renderer->render_target.windowed.surface,
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .forceFallbackAdapter = false,
    };
    WGPURequestAdapterCallbackInfo adapter_cb_info = {
        .callback = adapter_request_callback,
        .userdata1 = &cb_ctx,
    };
    wgpuInstanceRequestAdapter(instance, &adapter_options, adapter_cb_info);

    while (!cb_ctx.completed) {
        wgpuInstanceProcessEvents(instance);
    }
    if (!cb_ctx.success) {
        return RETURN_FAILURE;
    }
    log_debug("Adapter request successful");

    // Device request
    if (renderer->device != NULL) {
        wgpuDeviceRelease(renderer->device);
    }
    WGPUDeviceDescriptor device_desc = {.label = {"Device", WGPU_STRLEN}};
    WGPURequestDeviceCallbackInfo device_cb_info = {
        .callback = device_request_callback,
        .userdata1 = &cb_ctx,
    };

    wgpuAdapterRequestDevice(renderer->adapter, &device_desc, device_cb_info);

    while (!cb_ctx.completed) {
        wgpuInstanceProcessEvents(instance);
    }
    if (!cb_ctx.success) {
        log_error("Device request error");
        wgpuAdapterRelease(renderer->adapter);
        return RETURN_FAILURE;
    }
    log_debug("Device request successful");

    // Get device queue
    renderer->queue = wgpuDeviceGetQueue(renderer->device);

    // Create render target
    WGPUSurfaceCapabilities surface_caps = {0};
    wgpuSurfaceGetCapabilities(
        renderer->render_target.windowed.surface,
        renderer->adapter,
        &surface_caps
    );
    if (surface_caps.formatCount == 0) {
        log_error("No supported surface formats found");
        return RETURN_FAILURE;
    }
    log_debug("%ld surface formats found.", surface_caps.formatCount);
    WGPUTextureFormat texture_format = surface_caps.formats[0];
    renderer->render_target.windowed.surface_config =
        (WGPUSurfaceConfiguration){
            .usage = WGPUTextureUsage_RenderAttachment,
            .format = texture_format,
            .width = width,
            .height = height,
            .presentMode = WGPUPresentMode_Fifo,
            .device = renderer->device,
        };
    wgpuSurfaceConfigure(
        renderer->render_target.windowed.surface,
        &renderer->render_target.windowed.surface_config
    );
    log_debug(
        "Configured surface size: [%d, %d]",
        renderer->render_target.windowed.surface_config.width,
        renderer->render_target.windowed.surface_config.height
    );

    // Create depth texture
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    Renderer_create_depth_texture(
        renderer, width, height, "Depth Texture", depth_texture_format
    );

    // Create uniform buffer
    mat4 proj_matrix = {0};
    glm_perspective(
        glm_rad(60.0), (f32)width / (f32)height, 0.1, 1000.0, proj_matrix
    );
    mat4 view_matrix = {0};
    glm_mat4_identity(view_matrix);
    WGPUBufferDescriptor uniform_buffer_desc = {
        .label = {"Uniform Buffer", WGPU_STRLEN},
        .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
        .size = sizeof(Uniform),
        .mappedAtCreation = false,
    };
    renderer->uniform_buffer =
        wgpuDeviceCreateBuffer(renderer->device, &uniform_buffer_desc);

    // Create meshes
    Renderer_register_builtin_meshes(renderer);

    // Create bind group layout
    WGPUBindGroupLayoutEntry bind_group_layout_entries[] = {
        // Uniforms entry.  Currently the same for all render pipelines.
        (WGPUBindGroupLayoutEntry){
            .binding = 0,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer = (WGPUBufferBindingLayout){
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(Uniform),
            },
        },
    };
    WGPUBindGroupLayoutDescriptor bind_group_layout_desc = {
        .label = {"Bind Group Layout", WGPU_STRLEN},
        .entries = bind_group_layout_entries,
        .entryCount = 1,
    };
    WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(
        renderer->device, &bind_group_layout_desc
    );

    // Create bind group
    WGPUBindGroupEntry bind_group_entries[] = {
        (WGPUBindGroupEntry){
            .binding = 0,
            .buffer = renderer->uniform_buffer,
            .offset = 0,
            .size = sizeof(Uniform),
        },
    };
    WGPUBindGroupDescriptor bind_group_desc = {
        .label = {"Bind Group", WGPU_STRLEN},
        .layout = bind_group_layout,
        .entries = bind_group_entries,
        .entryCount = 1,
    };
    renderer->uniform_bind_group =
        wgpuDeviceCreateBindGroup(renderer->device, &bind_group_desc);

    // Create solid render pipeline
    ReturnStatus create_pipeline_status = Renderer_create_render_pipeline(
        renderer,
        &renderer->solid_pipeline,
        "Solid Pipeline",
        RAIJIN_ASSETS_DIR "/shaders/solid_shader.wgsl",
        texture_format,
        depth_texture_format,
        bind_group_layout,
        WGPUPrimitiveTopology_TriangleList,
        WGPUCullMode_None
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }

    // Create edges render pipeline
    create_pipeline_status = Renderer_create_render_pipeline(
        renderer,
        &renderer->edges_pipeline,
        "Edges Pipeline",
        RAIJIN_ASSETS_DIR "/shaders/edges_shader.wgsl",
        texture_format,
        depth_texture_format,
        bind_group_layout,
        WGPUPrimitiveTopology_LineList,
        WGPUCullMode_None
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }
    return RETURN_SUCCESS;
}

ReturnStatus Renderer_init_headless(
    Renderer* renderer, const WGPUInstance instance, u32 width, u32 height
) {
    renderer->render_mode = RENDER_MODE_HEADLESS;
    WgpuCallbackContext cb_ctx = {
        .completed = false,
        .adapter = &renderer->adapter,
        .device = &renderer->device,
    };

    renderer->instance = instance;

    // Adapter request
    if (renderer->adapter != NULL) {
        wgpuAdapterRelease(renderer->adapter);
    }
    WGPURequestAdapterOptions adapter_options = {
        // Don't need a surface
        .compatibleSurface = NULL,
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .forceFallbackAdapter = false,
    };
    WGPURequestAdapterCallbackInfo adapter_cb_info = {
        .callback = adapter_request_callback,
        .userdata1 = &cb_ctx,
    };
    wgpuInstanceRequestAdapter(
        renderer->instance, &adapter_options, adapter_cb_info
    );

    // TODO (mmckenna) : Handle this async
    while (!cb_ctx.completed) {
        wgpuInstanceProcessEvents(renderer->instance);
    }
    if (!cb_ctx.success) {
        return RETURN_FAILURE;
    }

    // Device request
    if (renderer->device != NULL) {
        wgpuDeviceRelease(renderer->device);
    }
    WGPUDeviceDescriptor device_desc = {.label = {"Device", WGPU_STRLEN}};
    WGPURequestDeviceCallbackInfo device_cb_info = {
        .callback = device_request_callback,
        .userdata1 = &cb_ctx,
    };

    wgpuAdapterRequestDevice(renderer->adapter, &device_desc, device_cb_info);

    // TODO (mckenna) : Handle this async
    while (!cb_ctx.completed) {
        wgpuInstanceProcessEvents(renderer->instance);
    }
    if (!cb_ctx.success) {
        wgpuAdapterRelease(renderer->adapter);
        return RETURN_FAILURE;
    }

    // Get device queue
    renderer->queue = wgpuDeviceGetQueue(renderer->device);

    // Create render target
    // TODO (mmckenna) : Look at different formats, including `Bgra8UnormSrgb`
    // WGPUTextureFormat texture_format = WGPUTextureFormat_RGBA8Unorm;
    WGPUTextureFormat texture_format = WGPUTextureFormat_RGBA8UnormSrgb;
    WGPUTextureDescriptor texture_desc = {
        .label = {"Headless Texture", WGPU_STRLEN},
        .size =
            (WGPUExtent3D){
                .width = width > 0 ? width : 1,
                .height = height > 0 ? height : 1,
                .depthOrArrayLayers = 1,
            },
        .mipLevelCount = 1,
        .sampleCount = 1,
        .dimension = WGPUTextureDimension_2D,
        .format = texture_format,
        .usage = WGPUTextureUsage_RenderAttachment |
                 WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc,
        .viewFormats = &texture_format,
        .viewFormatCount = 1,
    };
    renderer->render_target.headless.texture =
        wgpuDeviceCreateTexture(renderer->device, &texture_desc);

    // Create depth texture
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    Renderer_create_depth_texture(
        renderer, width, height, "Depth Texture", depth_texture_format
    );

    // Create uniform buffer
    mat4 proj_matrix = {0};
    glm_perspective(
        glm_rad(60.0), (f32)width / (f32)height, 0.1, 1000.0, proj_matrix
    );
    mat4 view_matrix = {0};
    glm_mat4_identity(view_matrix);
    WGPUBufferDescriptor uniform_buffer_desc = {
        .label = {"Uniform Buffer", WGPU_STRLEN},
        .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
        .size = sizeof(Uniform),
        .mappedAtCreation = false,
    };
    renderer->uniform_buffer =
        wgpuDeviceCreateBuffer(renderer->device, &uniform_buffer_desc);

    // Create meshes
    Renderer_register_builtin_meshes(renderer);

    // Create bind group layout
    WGPUBindGroupLayoutEntry bind_group_layout_entries[] = {
        // Uniforms entry.  Currently the same for all render pipelines.
        (WGPUBindGroupLayoutEntry){
            .binding = 0,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer = (WGPUBufferBindingLayout){
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(Uniform),
            },
        },
    };
    WGPUBindGroupLayoutDescriptor bind_group_layout_desc = {
        .label = {"Bind Group Layout", WGPU_STRLEN},
        .entries = bind_group_layout_entries,
        .entryCount = 1,
    };
    WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(
        renderer->device, &bind_group_layout_desc
    );

    // Create bind group
    WGPUBindGroupEntry bind_group_entries[] = {
        (WGPUBindGroupEntry){
            .binding = 0,
            .buffer = renderer->uniform_buffer,
            .offset = 0,
            .size = sizeof(Uniform),
        },
    };
    WGPUBindGroupDescriptor bind_group_desc = {
        .label = {"Bind Group", WGPU_STRLEN},
        .layout = bind_group_layout,
        .entries = bind_group_entries,
        .entryCount = 1,
    };
    renderer->uniform_bind_group =
        wgpuDeviceCreateBindGroup(renderer->device, &bind_group_desc);

    // Create solid render pipeline
    ReturnStatus create_pipeline_status = Renderer_create_render_pipeline(
        renderer,
        &renderer->solid_pipeline,
        "Solid Pipeline",
        RAIJIN_ASSETS_DIR "/shaders/solid_shader.wgsl",
        texture_format,
        depth_texture_format,
        bind_group_layout,
        WGPUPrimitiveTopology_TriangleList,
        WGPUCullMode_None
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }

    // Create edges render pipeline
    create_pipeline_status = Renderer_create_render_pipeline(
        renderer,
        &renderer->edges_pipeline,
        "Edges Pipeline",
        RAIJIN_ASSETS_DIR "/shaders/edges_shader.wgsl",
        texture_format,
        depth_texture_format,
        bind_group_layout,
        WGPUPrimitiveTopology_LineList,
        WGPUCullMode_None
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }
    return RETURN_SUCCESS;
}

// TODO (mmckenna): Target for arena allocator
void Renderer_render_mesh(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
) {
    InstanceArray instances;
    InstanceArray_init(&instances);
    for (u32 i = 0; i < renderer->draw_commands.count; ++i) {
        DrawCommand* cmd = &renderer->draw_commands.items[i];
        if (cmd->mesh_handle == mesh_handle) {
            InstanceArray_push(&instances, cmd->instance);
        }
    }

    // No instances to render
    if (instances.count == 0) {
        return;
    }

    Mesh* mesh = &renderer->meshes.items[mesh_handle];
    if (instances.count > mesh->instance_capacity) {
        Mesh_realloc_instance_buffer(mesh, renderer->device, instances.count);
    }
    wgpuQueueWriteBuffer(
        renderer->queue,
        mesh->instance_buffer,
        0,
        instances.items,
        instances.count * sizeof(Instance)
    );
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass_encoder,
        0,
        mesh->vertex_buffer,
        0,
        mesh->vertices.count * sizeof(Vertex)
    );
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass_encoder,
        1,
        mesh->instance_buffer,
        0,
        instances.count * sizeof(Instance)
    );
    wgpuRenderPassEncoderSetIndexBuffer(
        render_pass_encoder,
        mesh->index_buffer,
        WGPUIndexFormat_Uint32,
        0,
        mesh->indices.count * sizeof(u32)
    );
    wgpuRenderPassEncoderDrawIndexed(
        render_pass_encoder, mesh->indices.count, instances.count, 0, 0, 0
    );
    InstanceArray_free(&instances);
}

// TODO (mmckenna): Target for arena allocator
void Renderer_render_mesh_edges(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
) {
    InstanceArray instances;
    InstanceArray_init(&instances);
    for (u32 i = 0; i < renderer->draw_commands.count; ++i) {
        DrawCommand* cmd = &renderer->draw_commands.items[i];
        if (cmd->mesh_handle == mesh_handle) {
            Instance instance = {0};
            memcpy(&instance, &cmd->instance, sizeof(instance));
            glm_vec4_one(instance.color);
            InstanceArray_push(&instances, instance);
        }
    }

    // No instances to render
    if (instances.count == 0) {
        return;
    }

    Mesh* mesh = &renderer->meshes.items[mesh_handle];
    if (instances.count > mesh->edge_instance_capacity) {
        Mesh_realloc_edge_instance_buffer(
            mesh, renderer->device, instances.count
        );
    }
    wgpuQueueWriteBuffer(
        renderer->queue,
        mesh->edge_instance_buffer,
        0,
        instances.items,
        instances.count * sizeof(Instance)
    );
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass_encoder,
        0,
        mesh->vertex_buffer,
        0,
        mesh->vertices.count * sizeof(Vertex)
    );
    wgpuRenderPassEncoderSetVertexBuffer(
        render_pass_encoder,
        1,
        mesh->edge_instance_buffer,
        0,
        instances.count * sizeof(Instance)
    );
    wgpuRenderPassEncoderSetIndexBuffer(
        render_pass_encoder,
        mesh->edge_index_buffer,
        WGPUIndexFormat_Uint16,
        0,
        mesh->edge_indices.count * sizeof(u32)
    );
    wgpuRenderPassEncoderDrawIndexed(
        render_pass_encoder, mesh->edge_indices.count, instances.count, 0, 0, 0
    );
    InstanceArray_free(&instances);
}

void Renderer_render_pass_solid(
    Renderer* renderer,
    const WGPUCommandEncoder command_encoder,
    const WGPUTextureView texture_view
) {
    WGPURenderPassColorAttachment color_attachment = {
        .view = texture_view,
        .loadOp = WGPULoadOp_Clear,
        .clearValue =
            (WGPUColor){
                .r = 0.03,
                .g = 0.03,
                .b = 0.03,
                .a = 1.0,
            },
        .storeOp = WGPUStoreOp_Store,
    };
    WGPURenderPassDepthStencilAttachment depth_stencil_attachment = {
        .view = renderer->depth_texture_view,
        .depthLoadOp = WGPULoadOp_Clear,
        .depthClearValue = 1.0,
        .depthStoreOp = WGPUStoreOp_Store,
    };
    WGPURenderPassDescriptor render_pass_desc = {
        .label = {"Render Pass", WGPU_STRLEN},
        .colorAttachments = &color_attachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &depth_stencil_attachment,
    };
    WGPURenderPassEncoder render_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_desc);
    wgpuRenderPassEncoderSetPipeline(
        render_pass_encoder, renderer->solid_pipeline
    );
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 0, renderer->uniform_bind_group, 0, NULL
    );

    // Render all meshes
    for (u32 i = 0; i < renderer->meshes.count; ++i) {
        Renderer_render_mesh(renderer, i, render_pass_encoder);
    }

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
    return;
}

void Renderer_render_pass_edges(
    Renderer* renderer,
    const WGPUCommandEncoder command_encoder,
    const WGPUTextureView texture_view
) {
    WGPURenderPassColorAttachment color_attachment = {
        .view = texture_view,
        .loadOp = WGPULoadOp_Load,
        .clearValue =
            (WGPUColor){
                .r = 0.03,
                .g = 0.03,
                .b = 0.03,
                .a = 1.0,
            },
        .storeOp = WGPUStoreOp_Store,
    };
    WGPURenderPassDepthStencilAttachment depth_stencil_attachment = {
        .view = renderer->depth_texture_view,
        .depthLoadOp = WGPULoadOp_Load,
        .depthClearValue = 1.0,
        .depthStoreOp = WGPUStoreOp_Store,
    };
    WGPURenderPassDescriptor render_pass_desc = {
        .label = {"Edges Render Pass", WGPU_STRLEN},
        .colorAttachments = &color_attachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &depth_stencil_attachment,
    };
    WGPURenderPassEncoder render_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_desc);
    wgpuRenderPassEncoderSetPipeline(
        render_pass_encoder, renderer->edges_pipeline
    );
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 0, renderer->uniform_bind_group, 0, NULL
    );

    // Render all meshes
    for (u32 i = 0; i < renderer->meshes.count; ++i) {
        Renderer_render_mesh_edges(renderer, i, render_pass_encoder);
    }

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
    return;
}

void Renderer_render_to_view(
    Renderer* renderer, const WGPUTextureView texture_view
) {
    WGPUCommandEncoderDescriptor command_encoder_desc = {
        .label = {"Encoder", WGPU_STRLEN}

    };
    WGPUCommandEncoder command_encoder =
        wgpuDeviceCreateCommandEncoder(renderer->device, &command_encoder_desc);

    Renderer_render_pass_solid(renderer, command_encoder, texture_view);
    if (renderer->enable_edges) {
        Renderer_render_pass_edges(renderer, command_encoder, texture_view);
    }

    WGPUCommandBufferDescriptor command_buffer_desc = {
        .label = {"Command Buffer", WGPU_STRLEN}
    };
    WGPUCommandBuffer command_buffer =
        wgpuCommandEncoderFinish(command_encoder, &command_buffer_desc);

    wgpuQueueSubmit(renderer->queue, 1, &command_buffer);

    // Cleanup
    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);
    return;
}

// Provide a single interface for all render modes
ReturnStatus Renderer_render(Renderer* renderer) {
    WGPUTextureViewDescriptor texture_view_desc = {
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All,
    };
    WGPUTextureView texture_view = {0};
    ReturnStatus status = RETURN_SUCCESS;
    switch (renderer->render_mode) {
        case RENDER_MODE_HEADLESS: {
            texture_view_desc.label =
                (WGPUStringView){"Headless Texture View", WGPU_STRLEN};
            texture_view_desc.format = WGPUTextureFormat_RGBA8UnormSrgb;
            texture_view = wgpuTextureCreateView(
                renderer->render_target.headless.texture, &texture_view_desc
            );
            if (texture_view != NULL) {
                Renderer_render_to_view(renderer, texture_view);
                wgpuTextureViewRelease(texture_view);
            }
        } break;
        case RENDER_MODE_WINDOWED: {
            texture_view_desc.label =
                (WGPUStringView){"Windowed Texture View", WGPU_STRLEN};
            texture_view_desc.format =
                renderer->render_target.windowed.surface_config.format;
            WGPUSurfaceTexture surface_texture = {0};
            wgpuSurfaceGetCurrentTexture(
                renderer->render_target.windowed.surface, &surface_texture
            );
            // TODO (mmckenna): Handle each status variant
            if (surface_texture.status !=
                WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
                log_error("Failed to get surface texture");
                // TODO (mmckenna) reconfigure surface and re-initialize depth
                // texture
                status = RETURN_FAILURE;
                if (surface_texture.texture != NULL) {
                    wgpuTextureRelease(surface_texture.texture);
                }
                break;
            }

            texture_view = wgpuTextureCreateView(
                surface_texture.texture, &texture_view_desc
            );
            if (texture_view != NULL) {
                Renderer_render_to_view(renderer, texture_view);
                WGPUStatus present_status = wgpuSurfacePresent(
                    renderer->render_target.windowed.surface
                );
                // TODO (mmckenna): Handle each status variant
                if (present_status != WGPUStatus_Success) {
                    log_error("Failed to present surface");
                    status = RETURN_FAILURE;
                }
                wgpuTextureViewRelease(texture_view);
                wgpuTextureRelease(surface_texture.texture);
            }
        } break;
    }
    DrawCommandArray_clear(&renderer->draw_commands);
    return status;
}

void Renderer_destroy(Renderer* renderer) {
    DrawCommandArray_free(&renderer->draw_commands);

    u32 builtin_count = sizeof(renderer->builtin) / sizeof(MeshHandle);
    log_debug("Builtin count: %d", builtin_count);
    for (u32 i = 0; i < builtin_count; ++i) {
        Mesh* mesh = &renderer->meshes.items[i];
        if (mesh->vertex_buffer != NULL) {
            wgpuBufferRelease(mesh->vertex_buffer);
        }
        if (mesh->index_buffer != NULL) {
            wgpuBufferRelease(mesh->index_buffer);
        }
        if (mesh->instance_buffer != NULL) {
            wgpuBufferRelease(mesh->instance_buffer);
        }
        if (mesh->edge_index_buffer != NULL) {
            wgpuBufferRelease(mesh->edge_index_buffer);
        }
        if (mesh->edge_instance_buffer != NULL) {
            wgpuBufferRelease(mesh->edge_instance_buffer);
        }
        if (mesh->vertices.items != NULL) {
            VertexArray_free(&mesh->vertices);
        }
        if (mesh->indices.items != NULL) {
            IndexArray_free(&mesh->indices);
        }
        if (mesh->edge_indices.items != NULL) {
            IndexArray_free(&mesh->edge_indices);
        }
        log_debug("Mesh %d free.", i);
    }
    MeshArray_free(&renderer->meshes);
    log_debug("Mesh array free.");

    if (renderer->uniform_buffer != NULL) {
        wgpuBufferRelease(renderer->uniform_buffer);
        log_debug("Uniform buffer released.");
    }
    if (renderer->solid_pipeline != NULL) {
        wgpuRenderPipelineRelease(renderer->solid_pipeline);
        log_debug("Solid pipeline released.");
    }
    if (renderer->edges_pipeline != NULL) {
        wgpuRenderPipelineRelease(renderer->edges_pipeline);
        log_debug("Edges pipeline released.");
    }
    if (renderer->depth_texture_view != NULL) {
        wgpuTextureViewRelease(renderer->depth_texture_view);
        log_debug("Depth texture view released.");
    }
    if (renderer->depth_texture != NULL) {
        wgpuTextureRelease(renderer->depth_texture);
        log_debug("Depth texture released.");
    }
    switch (renderer->render_mode) {
        case RENDER_MODE_WINDOWED: {
            if (renderer->render_target.windowed.surface != NULL) {
                wgpuSurfaceRelease(renderer->render_target.windowed.surface);
                log_debug("Window surface released.");
            }
        } break;
        case RENDER_MODE_HEADLESS: {
            if (renderer->render_target.headless.texture != NULL) {
                wgpuTextureRelease(renderer->render_target.headless.texture);
                log_debug("Headless texture released.");
            }
        } break;
    }
    if (renderer->queue != NULL) {
        wgpuQueueRelease(renderer->queue);
        log_debug("Queue released.");
    }
    if (renderer->device != NULL) {
        wgpuDeviceRelease(renderer->device);
        log_debug("Device released.");
    }
    if (renderer->adapter != NULL) {
        wgpuAdapterRelease(renderer->adapter);
        log_debug("Adapter released.");
    }
    if (renderer->instance != NULL) {
        wgpuInstanceRelease(renderer->instance);
        log_debug("Instance released.");
    }
}

void Renderer_handle_resize(Renderer* renderer, u32 width, u32 height) {
    // Avoid zero-size textures (minimized window)
    if (width == 0 || height == 0) return;

    renderer->render_target.windowed.surface_config.width = width;
    renderer->render_target.windowed.surface_config.height = height;
    wgpuSurfaceConfigure(
        renderer->render_target.windowed.surface,
        &renderer->render_target.windowed.surface_config
    );

    // Recreate depth texture
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    WGPUTextureDescriptor depth_texture_desc = {
        .label = {"Depth Texture", WGPU_STRLEN},
        .usage = WGPUTextureUsage_RenderAttachment,
        .dimension = WGPUTextureDimension_2D,
        .size = {width, height, 1},
        .format = depth_texture_format,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormats = &depth_texture_format,
        .viewFormatCount = 1,
    };

    if (renderer->depth_texture != NULL) {
        wgpuTextureRelease(renderer->depth_texture);
    }
    renderer->depth_texture =
        wgpuDeviceCreateTexture(renderer->device, &depth_texture_desc);

    if (renderer->depth_texture_view != NULL) {
        wgpuTextureViewRelease(renderer->depth_texture_view);
    }
    WGPUTextureViewDescriptor depth_texture_view_desc = {
        .label = {"Depth Texture View", WGPU_STRLEN},
        .format = depth_texture_format,
        .dimension = WGPUTextureViewDimension_2D,
        .mipLevelCount = 1,
        .baseMipLevel = 0,
        .arrayLayerCount = 1,
        .baseArrayLayer = 0,
        .aspect = WGPUTextureAspect_DepthOnly,
    };
    renderer->depth_texture_view = wgpuTextureCreateView(
        renderer->depth_texture, &depth_texture_view_desc
    );

    log_debug("Surface configured successfully");
    return;
}

void Renderer_update_uniforms(
    Renderer* renderer, mat4 proj_matrix, mat4 view_matrix
) {
    Uniform uniform = {0};
    mat4 xform;
    glm_mat4_inv(view_matrix, xform);
    glm_vec4_copy3(xform[3], uniform.view_pos);
    glm_mat4_mul(proj_matrix, view_matrix, uniform.view_proj);
    wgpuQueueWriteBuffer(
        renderer->queue, renderer->uniform_buffer, 0, &uniform, sizeof(Uniform)
    );
}

ReturnStatus Renderer_copy_frame_to_buffer(
    Renderer* renderer, u32 width, u32 height, u8* buffer, u32 buffer_capacity
) {
    if (buffer == NULL) {
        log_error("Copy frame destination buffer was NULL.");
        return RETURN_FAILURE;
    }

    if (renderer->render_mode != RENDER_MODE_HEADLESS) {
        log_error(
            "You must be using headless render mode to copy frame to buffer."
        );
        return RETURN_FAILURE;
    }

    // RGBA8 assumed
    u32 bytes_per_pixel = 4;
    u32 buffer_size = width * height * bytes_per_pixel;

    if (buffer_capacity < buffer_size) {
        log_error(
            "Buffer of size %d is too small.  Expected %d minimum.",
            buffer_capacity,
            buffer_size
        );
        return RETURN_FAILURE;
    }

    WGPUBuffer staging_buffer = create_buffer(
        renderer->device,
        buffer_size,
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead,
        "Readback Staging Buffer"
    );
    if (staging_buffer == NULL) {
        log_error("Failed to create staging buffer.");
        return RETURN_FAILURE;
    }

    WGPUCommandEncoderDescriptor encoder_desc = {
        .label = {"Texture Readback Encoder", WGPU_STRLEN},
    };
    WGPUCommandEncoder encoder =
        wgpuDeviceCreateCommandEncoder(renderer->device, &encoder_desc);

    WGPUTexelCopyTextureInfo src = {
        .texture = renderer->render_target.headless.texture,
        .mipLevel = 0,
        .origin = {0, 0, 0},
        .aspect = WGPUTextureAspect_All,
    };

    WGPUTexelCopyBufferInfo dst = {
        .buffer = staging_buffer,
        .layout = {
            .offset = 0,
            .bytesPerRow = width * 4,
            .rowsPerImage = height,
        },
    };

    WGPUExtent3D copy_size = {width, height, 1};
    wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &copy_size);

    WGPUCommandBufferDescriptor cmd_buf_desc = {
        .label = {"Texture Readback Command Buffer", WGPU_STRLEN},
    };
    WGPUCommandBuffer cmd_buffer =
        wgpuCommandEncoderFinish(encoder, &cmd_buf_desc);
    wgpuQueueSubmit(renderer->queue, 1, &cmd_buffer);

    WgpuBufferMapContext buffer_map_ctx = {
        .completed = false,
        .status = WGPUMapAsyncStatus_Unknown,
    };
    WGPUBufferMapCallbackInfo map_callback_info = {
        .mode = WGPUCallbackMode_AllowProcessEvents,
        .callback = buffer_map_callback,
        .userdata1 = &buffer_map_ctx,
    };
    WGPUFuture future = wgpuBufferMapAsync(
        staging_buffer, WGPUMapMode_Read, 0, buffer_size, map_callback_info
    );

    // WGPUFutureWaitInfo wait_info = {
    //     .future = future,
    // };
    // wgpuInstanceWaitAny(renderer->instance, 1, &wait_info, UINT64_MAX);
    // while (wgpuBufferGetMapState(staging_buffer) ==
    // WGPUBufferMapState_Unmapped) {
    while (!buffer_map_ctx.completed) {
        // wgpuDevicePoll(renderer->device, false, NULL);
        wgpuInstanceProcessEvents(renderer->instance);
    }

    const void* mapped_data =
        wgpuBufferGetConstMappedRange(staging_buffer, 0, buffer_size);
    if (mapped_data != NULL) {
        memcpy(buffer, mapped_data, buffer_size);
    }

    wgpuBufferUnmap(staging_buffer);
    wgpuBufferRelease(staging_buffer);
    wgpuCommandBufferRelease(cmd_buffer);
    wgpuCommandEncoderRelease(encoder);

    return mapped_data == NULL ? RETURN_FAILURE : RETURN_SUCCESS;
}

/* WGPU callback functions */

void adapter_request_callback(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
) {
    WgpuCallbackContext* ctx = (WgpuCallbackContext*)userdata1;
    ctx->completed = true;
    if (status == WGPURequestAdapterStatus_Success) {
        log_info("Adapter acquired successfully");
        ctx->success = true;
        *ctx->adapter = adapter;
    } else {
        log_error(
            "Failed to acquire adapter: %.*s", (int)message.length, message.data
        );
        ctx->success = false;
    }
}

void device_request_callback(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
) {
    WgpuCallbackContext* ctx = (WgpuCallbackContext*)userdata1;
    ctx->completed = true;
    if (status == WGPURequestDeviceStatus_Success) {
        log_info("Device acquired successfully");
        ctx->success = true;
        *ctx->device = device;
    } else {
        log_error(
            "Failed to acquire device: %.*s", (int)message.length, message.data
        );
        ctx->success = false;
    }
}

void buffer_map_callback(
    WGPUMapAsyncStatus status,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
) {
    if (userdata1 != NULL) {
        WgpuBufferMapContext* ctx = (WgpuBufferMapContext*)userdata1;
        ctx->completed = true;
        ctx->status = status;
        if (message.length != 0) {
            ctx->message = message;
        }
        if (status != WGPUMapAsyncStatus_Success) {
            if (message.length > 0) {
                log_error("Failed to map buffer: %s", message.data);
            } else {
                log_error("Failed to map buffer: UNKNOWN ERROR");
            }
        }
    }
}

#endif /* RENDERER_H */
