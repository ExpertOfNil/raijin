#ifndef RENDERER_H
#define RENDERER_H

#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "core.h"
#include "mesh.h"
#include "webgpu.h"

/* Types */

typedef struct Uniform {
    mat4 view_proj;
} Uniform;

typedef struct DrawCommand {
    MeshType mesh_type;
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

typedef struct Renderer {
    bool enable_edges;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    RenderMode render_mode;
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
    Mesh meshes[MESH_TYPE_COUNT];
} Renderer;

/* Function Prototypes */

void Renderer_create_mesh_buffers(Mesh* mesh, Renderer* renderer);
ReturnStatus Renderer_init_windowed(
    Renderer* renderer,
    const WGPUInstance instance,
    const u32 width,
    const u32 height
);
ReturnStatus Renderer_init_headless(Renderer* renderer, u32 width, u32 height);
void Renderer_render_mesh(
    Renderer* renderer,
    const MeshType mesh_type,
    const WGPURenderPassEncoder render_pass_encoder
);
void Renderer_render_pass_solid(
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

static inline void adapter_request_callback(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    WGPUStringView msg,
    void* userdata1,
    void* userdata2
);

static inline void device_request_callback(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView msg,
    void* userdata1,
    void* userdata2
);

/* Functions */

void Renderer_create_mesh_buffers(Mesh* mesh, Renderer* renderer) {
    // Vertex buffer
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

    // Instance buffer
    mesh->instance_buffer = create_buffer(
        renderer->device,
        mesh->instance_capacity * sizeof(Instance),
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Index Buffer"
    );

    // Index buffer
    mesh->index_buffer = create_buffer(
        renderer->device,
        mesh->indices.count * sizeof(u16),
        WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
        "Index Buffer"
    );

    wgpuQueueWriteBuffer(
        renderer->queue,
        mesh->index_buffer,
        0,
        mesh->indices.items,
        mesh->indices.count * sizeof(u16)
    );

    // Edge instance buffer
    mesh->edge_instance_buffer = create_buffer(
        renderer->device,
        mesh->edge_instance_capacity * sizeof(Instance),
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Index Buffer"
    );

    // Index buffer
    mesh->edge_index_buffer = create_buffer(
        renderer->device,
        mesh->edge_indices.count * sizeof(u16),
        WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
        "Index Buffer"
    );

    wgpuQueueWriteBuffer(
        renderer->queue,
        mesh->edge_index_buffer,
        0,
        mesh->edge_indices.items,
        mesh->edge_indices.count * sizeof(u16)
    );
}

ReturnStatus Renderer_init_windowed(
    Renderer* renderer,
    const WGPUInstance instance,
    const u32 width,
    const u32 height
) {
    renderer->render_mode = RENDER_MODE_WINDOWED;
    WgpuCallbackContext cb_ctx = {
        .completed = false,
        .adapter = &renderer->adapter,
        .device = &renderer->device,
    };

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

    // TODO (mmckenna) : Handle this async
    while (!cb_ctx.completed) {
        wgpuInstanceProcessEvents(instance);
    }
    if (!cb_ctx.success) {
        return RETURN_FAILURE;
    }
    LOG_DEBUG("Adapter request successful");

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
        wgpuInstanceProcessEvents(instance);
    }
    if (!cb_ctx.success) {
        LOG_ERROR("Device request error");
        wgpuAdapterRelease(renderer->adapter);
        return RETURN_FAILURE;
    }
    LOG_DEBUG("Device request successful");

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
        LOG_ERROR("No supported surface formats found");
        return RETURN_FAILURE;
    }
    LOG_DEBUG("%ld surface formats found.", surface_caps.formatCount);
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
    LOG_DEBUG(
        "Configured surface size: [%d, %d]",
        renderer->render_target.windowed.surface_config.width,
        renderer->render_target.windowed.surface_config.height
    );

    // Create depth texture
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    WGPUTextureDescriptor depth_texture_desc = {
        .label = {"Depth Texture", WGPU_STRLEN},
        .size =
            (WGPUExtent3D){
                .width = width > 0 ? width : 256,
                .height = height > 0 ? height : 256,
                .depthOrArrayLayers = 1,
            },
        .mipLevelCount = 1,
        .sampleCount = 1,
        .dimension = WGPUTextureDimension_2D,
        .format = depth_texture_format,
        .usage = WGPUTextureUsage_RenderAttachment,
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
        .format = WGPUTextureFormat_Depth24Plus,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
    };
    renderer->depth_texture_view = wgpuTextureCreateView(
        renderer->depth_texture, &depth_texture_view_desc
    );

    // Create uniform buffer
    mat4 proj_matrix = {0};
    glm_perspective_rh_no(
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
    Mesh_create_cube(&renderer->meshes[MESH_TYPE_CUBE]);
    Renderer_create_mesh_buffers(&renderer->meshes[MESH_TYPE_CUBE], renderer);

    // Create bind group layout
    WGPUBindGroupLayoutEntry bind_group_layout_entries[] = {
        // Uniforms entry.  Currently the same for all render pipelines.
        (WGPUBindGroupLayoutEntry){
            .binding = 0,
            .visibility = WGPUShaderStage_Vertex,
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
    CharArray default_shader_src = {0};
    ReturnStatus shader_load_status = load_shader(
        RAIJIN_ASSETS_DIR "/shaders/default_shader.wgsl", &default_shader_src
    );
    if (shader_load_status != RETURN_SUCCESS) {
        LOG_ERROR("Failed to load shader");
        // TODO (mmcknna) : what to do when shader loading fails?
    }
    WGPUShaderSourceWGSL wgsl_desc = {
        .chain.sType = WGPUSType_ShaderSourceWGSL,
        .code = {.data = default_shader_src.items, default_shader_src.count}
    };
    WGPUShaderModuleDescriptor default_shader_desc = {
        .nextInChain = &wgsl_desc.chain,
        .label = {"Default Shader", WGPU_STRLEN},
    };
    WGPUShaderModule default_shader =
        wgpuDeviceCreateShaderModule(renderer->device, &default_shader_desc);
    WGPUVertexBufferLayout vertex_buffer_layouts[] = {
        Vertex_desc(),
        Instance_desc(),
    };
    WGPUVertexState vert_state = {
        .module = default_shader,
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
        .module = default_shader,
        .entryPoint = {"fs_main", WGPU_STRLEN},
        .targets = &color_target_state,
        .targetCount = 1,
    };
    WGPUDepthStencilState depth_pencil_state = {
        .format = depth_texture_format,
        .depthWriteEnabled = true,
        .depthCompare = WGPUCompareFunction_Less,
    };
    WGPUPipelineLayoutDescriptor solid_pipeline_layout_desc = {
        .label = {"Solid Pipeline Layout", WGPU_STRLEN},
        .bindGroupLayouts = &bind_group_layout,
        .bindGroupLayoutCount = 1,
    };
    WGPURenderPipelineDescriptor solid_pipeline_desc = {
        .label = {"Solid Pipeline", WGPU_STRLEN},
        .layout = wgpuDeviceCreatePipelineLayout(
            renderer->device, &solid_pipeline_layout_desc
        ),
        .vertex = vert_state,
        .fragment = &frag_state,
        .depthStencil = &depth_pencil_state,
        .primitive =
            (WGPUPrimitiveState){
                .topology = WGPUPrimitiveTopology_TriangleList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_Back,
                .unclippedDepth = false,
            },
        .multisample = (WGPUMultisampleState){
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = false,
        },
    };
    renderer->solid_pipeline =
        wgpuDeviceCreateRenderPipeline(renderer->device, &solid_pipeline_desc);

    // Create edges render pipeline
    WGPUFragmentState edges_frag_state = {
        .module = default_shader,
        .entryPoint = {"edges_fs_main", WGPU_STRLEN},
        .targets = &color_target_state,
    };
    WGPUPipelineLayoutDescriptor edges_pipeline_layout_desc = {
        .label = {"Edges Pipeline Layout", WGPU_STRLEN},
        .bindGroupLayouts = &bind_group_layout,
        .bindGroupLayoutCount = 1,
    };
    WGPUDepthStencilState edges_depth_pencil_state = {
        .format = WGPUTextureFormat_Depth24Plus,
        .depthWriteEnabled = false,
        .depthCompare = WGPUCompareFunction_Less,
    };
    WGPURenderPipelineDescriptor edges_pipeline_desc = {
        .label = {"Edges Pipeline", WGPU_STRLEN},
        .layout = wgpuDeviceCreatePipelineLayout(
            renderer->device, &edges_pipeline_layout_desc
        ),
        .vertex = vert_state,
        .fragment = &edges_frag_state,
        .depthStencil = &edges_depth_pencil_state,
        .primitive =
            (WGPUPrimitiveState){
                .topology = WGPUPrimitiveTopology_LineList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_Back,
                .unclippedDepth = false,
            },
        .multisample = (WGPUMultisampleState){
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = false,
        },
    };
    renderer->edges_pipeline =
        wgpuDeviceCreateRenderPipeline(renderer->device, &edges_pipeline_desc);

    CharArray_free(&default_shader_src);
    return RETURN_SUCCESS;
}

ReturnStatus Renderer_init_headless(Renderer* renderer, u32 width, u32 height) {
    renderer->render_mode = RENDER_MODE_HEADLESS;
    WgpuCallbackContext cb_ctx = {
        .completed = false,
        .adapter = &renderer->adapter,
        .device = &renderer->device,
    };
    WGPUInstanceDescriptor instance_desc = {0};
    WGPUInstance instance = wgpuCreateInstance(&instance_desc);
    if (instance == NULL) {
        LOG_ERROR("Failed to create WGPU instance");
        return RETURN_FAILURE;
    }

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
    wgpuInstanceRequestAdapter(instance, &adapter_options, adapter_cb_info);

    // TODO (mmckenna) : Handle this async
    while (!cb_ctx.completed) {
        wgpuInstanceProcessEvents(instance);
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
        wgpuInstanceProcessEvents(instance);
    }
    if (!cb_ctx.success) {
        wgpuAdapterRelease(renderer->adapter);
        return RETURN_FAILURE;
    }

    // Get device queue
    renderer->queue = wgpuDeviceGetQueue(renderer->device);

    // Create render target
    // TODO (mmckenna) : Look at different formats, including `Bgra8UnormSrgb`
    WGPUTextureFormat texture_format = WGPUTextureFormat_RGBA8Unorm;
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
    WGPUTextureDescriptor depth_texture_desc = {
        .label = {"Depth Texture", WGPU_STRLEN},
        .size =
            (WGPUExtent3D){
                .width = width > 0 ? width : 1,
                .height = height > 0 ? height : 1,
                .depthOrArrayLayers = 1,
            },
        .mipLevelCount = 1,
        .sampleCount = 1,
        .dimension = WGPUTextureDimension_2D,
        .format = depth_texture_format,
        .usage = WGPUTextureUsage_RenderAttachment,
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
        .format = WGPUTextureFormat_Depth24Plus,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
    };
    renderer->depth_texture_view = wgpuTextureCreateView(
        renderer->depth_texture, &depth_texture_view_desc
    );

    // Create uniform buffer
    mat4 proj_matrix = {0};
    glm_perspective_rh_no(
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
    Mesh_create_cube(&renderer->meshes[MESH_TYPE_CUBE]);

    // Create bind group layout
    WGPUBindGroupLayoutEntry bind_group_layout_entries[] = {
        // Uniforms entry.  Currently the same for all render pipelines.
        (WGPUBindGroupLayoutEntry){
            .binding = 0,
            .visibility = WGPUShaderStage_Vertex,
            .buffer = (WGPUBufferBindingLayout){
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = 0,
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
    WGPUBindGroupEntry bind_group_enties[] = {
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
        .entries = bind_group_enties,
    };
    wgpuDeviceCreateBindGroup(renderer->device, &bind_group_desc);

    // Create solid render pipeline
    CharArray default_shader_src = {0};
    ReturnStatus shader_load_status = load_shader(
        RAIJIN_ASSETS_DIR "/shaders/default_shader.wgsl", &default_shader_src
    );
    if (shader_load_status != RETURN_SUCCESS) {
        LOG_ERROR("Failed to load shader");
        // TODO (mmcknna) : what to do when shader loading fails?
    }
    WGPUShaderSourceWGSL wgsl_desc = {
        .chain.sType = WGPUSType_ShaderSourceWGSL,
        .code = {.data = default_shader_src.items, default_shader_src.count}
    };
    WGPUShaderModuleDescriptor default_shader_desc = {
        .nextInChain = &wgsl_desc.chain,
        .label = {"Default Shader", WGPU_STRLEN},
    };
    WGPUShaderModule default_shader =
        wgpuDeviceCreateShaderModule(renderer->device, &default_shader_desc);
    WGPUVertexBufferLayout vertex_buffer_layouts[] = {
        Vertex_desc(),
        Instance_desc(),
    };
    WGPUVertexState vert_state = {
        .module = default_shader,
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
        .module = default_shader,
        .entryPoint = {"fs_main", WGPU_STRLEN},
        .targets = &color_target_state,
    };
    WGPUDepthStencilState depth_pencil_state = {
        .format = depth_texture_format,
        .depthWriteEnabled = true,
        .depthCompare = WGPUCompareFunction_Less,
    };
    WGPUPipelineLayoutDescriptor solid_pipeline_layout_desc = {
        .label = {"Solid Pipeline Layout", WGPU_STRLEN},
        .bindGroupLayouts = &bind_group_layout,
        .bindGroupLayoutCount = 1,
    };
    WGPURenderPipelineDescriptor solid_pipeline_desc = {
        .label = {"Solid Pipeline", WGPU_STRLEN},
        .layout = wgpuDeviceCreatePipelineLayout(
            renderer->device, &solid_pipeline_layout_desc
        ),
        .vertex = vert_state,
        .fragment = &frag_state,
        .depthStencil = &depth_pencil_state,
        .primitive =
            (WGPUPrimitiveState){
                .topology = WGPUPrimitiveTopology_TriangleList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_Back,
                .unclippedDepth = false,
            },
        .multisample = (WGPUMultisampleState){
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = false,
        },
    };
    renderer->solid_pipeline =
        wgpuDeviceCreateRenderPipeline(renderer->device, &solid_pipeline_desc);

    // Create edges render pipeline
    WGPUFragmentState edges_frag_state = {
        .module = default_shader,
        .entryPoint = {"edges_fs_main", WGPU_STRLEN},
        .targets = &color_target_state,
    };
    WGPUPipelineLayoutDescriptor edges_pipeline_layout_desc = {
        .label = {"Edges Pipeline Layout", WGPU_STRLEN},
        .bindGroupLayouts = &bind_group_layout,
        .bindGroupLayoutCount = 1,
    };
    WGPUDepthStencilState edges_depth_pencil_state = {
        .format = WGPUTextureFormat_Depth24Plus,
        .depthWriteEnabled = false,
        .depthCompare = WGPUCompareFunction_Less,
    };
    WGPURenderPipelineDescriptor edges_pipeline_desc = {
        .label = {"Edges Pipeline", WGPU_STRLEN},
        .layout = wgpuDeviceCreatePipelineLayout(
            renderer->device, &edges_pipeline_layout_desc
        ),
        .vertex = vert_state,
        .fragment = &edges_frag_state,
        .depthStencil = &edges_depth_pencil_state,
        .primitive =
            (WGPUPrimitiveState){
                .topology = WGPUPrimitiveTopology_LineList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_Back,
                .unclippedDepth = false,
            },
        .multisample = (WGPUMultisampleState){
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = false,
        },
    };
    renderer->edges_pipeline =
        wgpuDeviceCreateRenderPipeline(renderer->device, &edges_pipeline_desc);

    CharArray_free(&default_shader_src);
    return RETURN_SUCCESS;
}

// TODO (mmckenna): Target for arena allocator
void Renderer_render_mesh(
    Renderer* renderer,
    const MeshType mesh_type,
    const WGPURenderPassEncoder render_pass_encoder
) {
    InstanceArray instances;
    InstanceArray_init(&instances);
    for (u32 i = 0; i < renderer->draw_commands.count; ++i) {
        DrawCommand* cmd = &renderer->draw_commands.items[i];
        if (cmd->mesh_type == mesh_type) {
            InstanceArray_push(&instances, cmd->instance);
        }
    }

    // No instances to render
    if (instances.count == 0) {
        return;
    }

    Mesh* mesh = &renderer->meshes[mesh_type];
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
        WGPUIndexFormat_Uint16,
        0,
        mesh->indices.count * sizeof(u16)
    );
    wgpuRenderPassEncoderDrawIndexed(
        render_pass_encoder, mesh->indices.count, instances.count, 0, 0, 0
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
                .r = 0.01,
                .g = 0.01,
                .b = 0.01,
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
    // TODO (mmckenna) : render mesh instances
    for (u32 i = 0; i < MESH_TYPE_COUNT; ++i) {
        switch (i) {
            case MESH_TYPE_TRIANGLE: {
                Renderer_render_mesh(
                    renderer, MESH_TYPE_TRIANGLE, render_pass_encoder
                );
            } break;
            case MESH_TYPE_CUBE: {
                Renderer_render_mesh(
                    renderer, MESH_TYPE_CUBE, render_pass_encoder
                );
            } break;
            case MESH_TYPE_TETRAHEDRON: {
                Renderer_render_mesh(
                    renderer, MESH_TYPE_TETRAHEDRON, render_pass_encoder
                );
            } break;
            case MESH_TYPE_SPHERE: {
                Renderer_render_mesh(
                    renderer, MESH_TYPE_SPHERE, render_pass_encoder
                );
            } break;
        }
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
    // TODO (mmckenna) : Outline render pass

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
            texture_view_desc.format = WGPUTextureFormat_RGBA8Unorm;
            texture_view = wgpuTextureCreateView(
                renderer->render_target.headless.texture, &texture_view_desc
            );
            if (texture_view != NULL) wgpuTextureViewRelease(texture_view);
        } break;
        case RENDER_MODE_WINDOWED: {
            texture_view_desc.label =
                (WGPUStringView){"Headless Texture View", WGPU_STRLEN};
            texture_view_desc.format =
                renderer->render_target.windowed.surface_config.format;
            WGPUSurfaceTexture surface_texture = {0};
            wgpuSurfaceGetCurrentTexture(
                renderer->render_target.windowed.surface, &surface_texture
            );
            // TODO (mmckenna): Handle each status variant
            if (surface_texture.status !=
                WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
                LOG_ERROR("Failed to get surface texture");
                // TODO (mmckenna) reconfigure surface and re-initialize depth
                // texture
                status = RETURN_FAILURE;
                break;
            }

            if (status == RETURN_SUCCESS) {
                texture_view = wgpuTextureCreateView(
                    surface_texture.texture, &texture_view_desc
                );
                Renderer_render_to_view(renderer, texture_view);
                WGPUStatus present_status = wgpuSurfacePresent(
                    renderer->render_target.windowed.surface
                );
                // TODO (mmckenna): Handle each status variant
                if (present_status != WGPUStatus_Success) {
                    LOG_ERROR("Failed to present surface");
                    status = RETURN_FAILURE;
                }
            }
            if (texture_view != NULL) wgpuTextureViewRelease(texture_view);
            if (surface_texture.texture != NULL) {
                wgpuTextureRelease(surface_texture.texture);
            }
        } break;
    }
    LOG_DEBUG("Command count: %ld", renderer->draw_commands.count);
    DrawCommandArray_reset(&renderer->draw_commands);
    LOG_DEBUG("Command count after: %ld", renderer->draw_commands.count);
    return status;
}

void Renderer_destroy(Renderer* renderer) {
    if (renderer->uniform_buffer != NULL) {
        wgpuBufferRelease(renderer->uniform_buffer);
    }
    if (renderer->solid_pipeline != NULL) {
        wgpuRenderPipelineRelease(renderer->solid_pipeline);
    }
    if (renderer->edges_pipeline != NULL) {
        wgpuRenderPipelineRelease(renderer->edges_pipeline);
    }
    if (renderer->depth_texture_view != NULL) {
        wgpuTextureViewRelease(renderer->depth_texture_view);
    }
    if (renderer->depth_texture != NULL) {
        wgpuTextureRelease(renderer->depth_texture);
    }
    switch (renderer->render_mode) {
        case RENDER_MODE_WINDOWED: {
            if (renderer->render_target.windowed.surface != NULL) {
                wgpuSurfaceRelease(renderer->render_target.windowed.surface);
            }
        } break;
        case RENDER_MODE_HEADLESS: {
            if (renderer->render_target.headless.texture != NULL) {
                wgpuTextureRelease(renderer->render_target.headless.texture);
            }
        } break;
    }
    if (renderer->queue != NULL) wgpuQueueRelease(renderer->queue);
    if (renderer->device != NULL) wgpuDeviceRelease(renderer->device);
    if (renderer->adapter != NULL) wgpuAdapterRelease(renderer->adapter);
}

void Renderer_handle_resize(Renderer* renderer, u32 width, u32 height) {
    renderer->render_target.windowed.surface_config.width = width;
    renderer->render_target.windowed.surface_config.height = height;
    wgpuSurfaceConfigure(
        renderer->render_target.windowed.surface,
        &renderer->render_target.windowed.surface_config
    );
    LOG_INFO("Surface configured successfully");
    return;
}

void Renderer_update_uniforms(
    Renderer* renderer, mat4 proj_matrix, mat4 view_matrix
) {
    Uniform uniform = {0};
    glm_mat4_mul(proj_matrix, view_matrix, uniform.view_proj);
    wgpuQueueWriteBuffer(
        renderer->queue, renderer->uniform_buffer, 0, &uniform, sizeof(Uniform)
    );
}

/* WGPU callback functions */

static inline void adapter_request_callback(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    WGPUStringView msg,
    void* userdata1,
    void* userdata2
) {
    WgpuCallbackContext* ctx = (WgpuCallbackContext*)userdata1;
    ctx->completed = true;
    if (status == WGPURequestAdapterStatus_Success) {
        LOG_INFO("Adapter acquired successfully");
        ctx->success = true;
        *ctx->adapter = adapter;
    } else {
        LOG_ERROR("Failed to acquire adapter: %.*s", (int)msg.length, msg.data);
        ctx->success = false;
    }
}

static inline void device_request_callback(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView msg,
    void* userdata1,
    void* userdata2
) {
    WgpuCallbackContext* ctx = (WgpuCallbackContext*)userdata1;
    ctx->completed = true;
    if (status == WGPURequestDeviceStatus_Success) {
        LOG_INFO("Device acquired successfully");
        ctx->success = true;
        *ctx->device = device;
    } else {
        LOG_ERROR("Failed to acquire device: %.*s", (int)msg.length, msg.data);
        ctx->success = false;
    }
}

#endif /* RENDERER_H */
