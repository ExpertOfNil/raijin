#ifndef RENDERER_H
#define RENDERER_H

#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "core.h"
#include "mesh.h"
#include "webgpu.h"

typedef struct Uniform {
    mat4 view_proj;
} Uniform;

typedef struct Instance {
    mat4 model_matrix;
    vec4 color;
} Instance;

static WGPUVertexBufferLayout Instance_desc(void) {
    static WGPUVertexAttribute attribs[5] = {
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 0 * 4 * sizeof(f32),
            .shaderLocation = 3,
        },
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 1 * 4 * sizeof(f32),
            .shaderLocation = 4,
        },
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 2 * 4 * sizeof(f32),
            .shaderLocation = 5,
        },
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 3 * 4 * sizeof(f32),
            .shaderLocation = 6,
        },
        {
            .format = WGPUVertexFormat_Float32x4,
            .offset = 4 * 4 * sizeof(f32),
            .shaderLocation = 7,
        },
    };

    return (WGPUVertexBufferLayout){
        .arrayStride = sizeof(Instance),
        .stepMode = WGPUVertexStepMode_Instance,
        .attributes = attribs,
    };
}

void Instance_set_position(Instance* instance, vec3 position) {
    glm_vec3_copy(position, instance->model_matrix[3]);
}

void Instance_from_position_rotation(
    Instance* instance, vec3 position, mat3 rotation, f32 scale, vec4 color
) {
    glm_mat4_identity(instance->model_matrix);
    glm_mat4_ins3(rotation, instance->model_matrix);
    glm_mat4_scale(instance->model_matrix, scale);
    glm_translate(instance->model_matrix, position);
    glm_vec4_copy(color, instance->color);
}

typedef struct DrawCommand {
    MeshType mesh_type;
    Instance instance;
} DrawCommand;

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

// WGPU callback functions
static void adapter_request_callback(
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

static void device_request_callback(
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

DEFINE_DYNAMIC_ARRAY(Mesh, MeshArray)
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
    DrawCommand* draw_commands;
    Mesh meshes[MESH_TYPE_COUNT];
} Renderer;

ReturnStatus Renderer_init_windowed(
    Renderer* renderer, WGPUSurface surface, u32 width, u32 height
) {
    renderer->render_mode = RENDER_MODE_WINDOWED;
    renderer->render_target.windowed.surface = surface;
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
        .compatibleSurface = surface,
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
    WGPUSurfaceCapabilities surface_caps = {0};
    wgpuSurfaceGetCapabilities(surface, renderer->adapter, &surface_caps);
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
        .label = {"Depth Texture View", WGPU_STRLEN}
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
    Mesh_create_cube(&renderer->meshes[0]);

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
    char* default_shader_src =
        load_shader(RAIJIN_ASSETS_DIR "/shaders/default_shader.wgsl");
    WGPUShaderSourceWGSL wgsl_desc = {
        .chain.sType = WGPUSType_ShaderSourceWGSL,
        .code = {.data = default_shader_src, WGPU_STRLEN}
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
        .label = {"Depth Texture View", WGPU_STRLEN}
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
    Mesh_create_cube(&renderer->meshes[0]);

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
    char* default_shader_src =
        load_shader(RAIJIN_ASSETS_DIR "/shaders/default_shader.wgsl");
    WGPUShaderSourceWGSL wgsl_desc = {
        .chain.sType = WGPUSType_ShaderSourceWGSL,
        .code = {.data = default_shader_src, WGPU_STRLEN}
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

    return RETURN_SUCCESS;
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

#endif /* RENDERER_H */
