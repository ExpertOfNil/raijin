#ifndef RENDERER_H
#define RENDERER_H

#include <inttypes.h>
#include <stddef.h>

#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "cimpl_core.h"
#include "core.h"
#include "logging.h"
#include "mesh.h"
#include "webgpu.h"
#include "wgpu.h"

extern const unsigned char raijin_solid_shader_wgsl[];
extern const size_t raijin_solid_shader_wgsl_size;

extern const unsigned char raijin_edges_shader_wgsl[];
extern const size_t raijin_edges_shader_wgsl_size;

/* Types */

enum {
    BUILTIN_PLANE,
    BUILTIN_DISC,
    BUILTIN_CUBE,
    BUILTIN_CYLINDER,
    BUILTIN_CONE,
    BUILTIN_SPHERE_UV,
    BUILTIN_COUNT,
};

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

typedef struct WgpuAdapterRequestContext {
    bool completed;
    WGPURequestAdapterStatus status;
    WGPUAdapter adapter;
} WgpuAdapterRequestContext;

typedef struct WgpuDeviceRequestContext {
    bool completed;
    WGPURequestDeviceStatus status;
    WGPUDevice device;
} WgpuDeviceRequestContext;

typedef struct WgpuDeviceLostContext {
    bool completed;
    WGPUDeviceLostReason reason;
} WgpuDeviceLostContext;

typedef struct WgpuBufferMapContext {
    bool active;
    bool completed;
    bool unmap_requested;
    WGPUMapAsyncStatus status;
    WGPUBuffer staging_buffer;
} WgpuBufferMapContext;

typedef struct RendererPipelineDesc {
    const char* label;
    const unsigned char* shader_data;
    size_t shader_size;
    WGPUTextureFormat texture_format;
    WGPUTextureFormat depth_texture_format;
    const WGPUBindGroupLayout bind_group_layout;
    WGPUPrimitiveTopology topology;
    WGPUCullMode cull_mode;
    WGPUCompareFunction depth_compare;
    bool depth_write_enabled;
    bool encode_output_srgb;
} RendererPipelineDesc;

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
    WgpuWaitConfig wait_config;
    WgpuAdapterRequestContext adapter_request;
    WgpuDeviceRequestContext device_request;
    WgpuDeviceLostContext device_lost;
    WGPUFuture device_lost_future;
    bool device_lost_future_available;
    WgpuBufferMapContext buffer_map;
    struct {
        // MeshHandle triangle;
        // MeshHandle tetrahedron;
        MeshHandle plane;
        MeshHandle disc;
        MeshHandle cube;
        MeshHandle cylinder;
        MeshHandle cone;
        MeshHandle sphere_uv;
    } builtin;
} Renderer;

/* Function Prototypes */

ReturnStatus Renderer_create_mesh_buffers(Mesh* mesh, Renderer* renderer);
MeshHandle Renderer_register_mesh(Renderer* renderer, Mesh* mesh_template);
ReturnStatus Renderer_create_depth_texture(
    Renderer* renderer,
    u32 width,
    u32 height,
    const char* label,
    WGPUTextureFormat format
);
ReturnStatus Renderer_create_render_pipeline(
    Renderer* renderer,
    WGPURenderPipeline* pipeline,
    const RendererPipelineDesc* desc
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
ReturnStatus Renderer_render_mesh(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
);
ReturnStatus Renderer_render_mesh_edges(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
);
ReturnStatus Renderer_render_pass_solid(
    Renderer* renderer,
    const WGPUCommandEncoder command_encoder,
    const WGPUTextureView texture_view
);
ReturnStatus Renderer_render_pass_edges(
    Renderer* renderer,
    const WGPUCommandEncoder command_encoder,
    const WGPUTextureView texture_view
);
ReturnStatus Renderer_render_to_view(
    Renderer* renderer, const WGPUTextureView texture_view
);
ReturnStatus Renderer_render(Renderer* renderer);
void Renderer_destroy(Renderer* renderer);
ReturnStatus Renderer_handle_resize(Renderer* renderer, u32 width, u32 height);
void Renderer_update_uniforms(
    Renderer* renderer, mat4 proj_matrix, mat4 view_matrix
);
ReturnStatus Renderer_copy_frame_to_buffer(
    Renderer* renderer, u32 width, u32 height, u8* buffer, u64 buffer_capacity
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

void device_lost_callback(
    const WGPUDevice* device,
    WGPUDeviceLostReason reason,
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

ReturnStatus Renderer_create_mesh_buffers(Mesh* mesh, Renderer* renderer) {
    if (mesh == NULL || renderer == NULL || renderer->device == NULL ||
        renderer->queue == NULL) {
        return RETURN_FAILURE;
    }
    if (mesh->vertices.items == NULL || mesh->vertices.count == 0) {
        log_error("Mesh has no vertices\n");
        return RETURN_FAILURE;
    }
    if (mesh->indices.items == NULL || mesh->indices.count == 0) {
        log_error("Mesh has no triangle indices\n");
        return RETURN_FAILURE;
    }
    if (mesh->instance_capacity == 0) {
        log_error("Mesh instance capacity is zero\n");
        return RETURN_FAILURE;
    }
    if (mesh->edge_indices.count > 0 && mesh->edge_indices.items == NULL) {
        log_error("Mesh edge indices are invalid\n");
        return RETURN_FAILURE;
    }
    if (mesh->edge_indices.count > 0 && mesh->edge_instance_capacity == 0) {
        log_error("Mesh edge instance capacity is zero\n");
        return RETURN_FAILURE;
    }
    if (mesh->vertex_buffer != NULL || mesh->instance_buffer != NULL ||
        mesh->index_buffer != NULL || mesh->edge_instance_buffer != NULL ||
        mesh->edge_index_buffer != NULL) {
        log_error("Mesh GPU buffers already exist\n");
        return RETURN_FAILURE;
    }

    uint64_t vertex_size = (uint64_t)mesh->vertices.count * sizeof(Vertex);
    uint64_t instance_size =
        (uint64_t)mesh->instance_capacity * sizeof(Instance);
    uint64_t index_size = (uint64_t)mesh->indices.count * sizeof(uint32_t);

    Mesh candidate = {0};

    candidate.vertex_buffer = create_buffer(
        renderer->device,
        vertex_size,
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Vertex Buffer"
    );
    if (candidate.vertex_buffer == NULL) {
        goto failure;
    }

    candidate.instance_buffer = create_buffer(
        renderer->device,
        instance_size,
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Instance Buffer"
    );
    if (candidate.instance_buffer == NULL) {
        goto failure;
    }

    candidate.index_buffer = create_buffer(
        renderer->device,
        mesh->indices.count * sizeof(u32),
        WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
        "Index Buffer"
    );
    if (candidate.index_buffer == NULL) {
        goto failure;
    }

    if (mesh->edge_indices.count > 0) {
        uint64_t edge_instance_size =
            (uint64_t)mesh->edge_instance_capacity * sizeof(Instance);
        uint64_t edge_index_size =
            (uint64_t)mesh->edge_indices.count * sizeof(uint32_t);

        candidate.edge_instance_buffer = create_buffer(
            renderer->device,
            edge_instance_size,
            WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
            "Edge Instance Buffer"
        );
        if (candidate.edge_instance_buffer == NULL) {
            goto failure;
        }

        candidate.edge_index_buffer = create_buffer(
            renderer->device,
            edge_index_size,
            WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
            "Edge Index Buffer"
        );
        if (candidate.edge_index_buffer == NULL) {
            goto failure;
        }
    }

    wgpuQueueWriteBuffer(
        renderer->queue,
        candidate.vertex_buffer,
        0,
        mesh->vertices.items,
        vertex_size
    );

    wgpuQueueWriteBuffer(
        renderer->queue,
        candidate.index_buffer,
        0,
        mesh->indices.items,
        index_size
    );

    if (candidate.edge_index_buffer != NULL) {
        wgpuQueueWriteBuffer(
            renderer->queue,
            candidate.edge_index_buffer,
            0,
            mesh->edge_indices.items,
            (uint64_t)mesh->edge_indices.count * sizeof(uint32_t)
        );
    }

    mesh->vertex_buffer = candidate.vertex_buffer;
    mesh->instance_buffer = candidate.instance_buffer;
    mesh->index_buffer = candidate.index_buffer;
    mesh->edge_instance_buffer = candidate.edge_instance_buffer;
    mesh->edge_index_buffer = candidate.edge_index_buffer;

    return RETURN_SUCCESS;

failure:
    Mesh_release_gpu_buffers(&candidate);
    return RETURN_FAILURE;
}

MeshHandle Renderer_register_mesh(Renderer* renderer, Mesh* mesh_template) {
    if (renderer == NULL || mesh_template == NULL) return INVALID_MESH_HANDLE;
    if (renderer->meshes.count == UINT32_MAX) {
        log_error("Mesh handle capacity exhausted\n");
        return INVALID_MESH_HANDLE;
    }

    size_t required_count = (size_t)renderer->meshes.count + 1;
    if (MeshArray_reserve(&renderer->meshes, required_count)) {
        return INVALID_MESH_HANDLE;
    }
    Mesh mesh = {
        .vertices = mesh_template->vertices,
        .indices = mesh_template->indices,
        .edge_indices = mesh_template->edge_indices,
        .instance_capacity = DEFAULT_INSTANCE_CAPACITY,
        .edge_instance_capacity = DEFAULT_INSTANCE_CAPACITY,
    };

    if (Renderer_create_mesh_buffers(&mesh, renderer) != RETURN_SUCCESS) {
        return INVALID_MESH_HANDLE;
    }
    if (MeshArray_push(&renderer->meshes, mesh) != RETURN_OK) {
        Mesh_release_gpu_buffers(&mesh);
        return INVALID_MESH_HANDLE;
    }
    // Handle = index
    return renderer->meshes.count - 1;
}

static ReturnStatus Renderer_register_builtin_meshes(Renderer* renderer) {
    if (renderer == NULL) return RETURN_FAILURE;
    // Ensure constant values for builtin meshes
    if (renderer->meshes.count != 0) return RETURN_FAILURE;

    // TODO (mmckenna): missing TRIANGLE
    // TODO (mmckenna): missing TETRAHEDRON

    Mesh candidates[BUILTIN_COUNT] = {0};
    MeshHandle handles[BUILTIN_COUNT] = {0};
    for (uint32_t i = 0; i < BUILTIN_COUNT; ++i) {
        handles[i] = INVALID_MESH_HANDLE;
    }

    renderer->builtin.plane = INVALID_MESH_HANDLE;
    renderer->builtin.disc = INVALID_MESH_HANDLE;
    renderer->builtin.cube = INVALID_MESH_HANDLE;
    renderer->builtin.cylinder = INVALID_MESH_HANDLE;
    renderer->builtin.cone = INVALID_MESH_HANDLE;
    renderer->builtin.sphere_uv = INVALID_MESH_HANDLE;

    if (MeshArray_reserve(&renderer->meshes, BUILTIN_COUNT) != RETURN_OK) {
        goto failure;
    }
    if (Mesh_create_plane(&candidates[BUILTIN_PLANE]) != RETURN_SUCCESS) {
        goto failure;
    }
    if (Mesh_create_disc(&candidates[BUILTIN_DISC], 32) != RETURN_SUCCESS) {
        goto failure;
    }
    if (Mesh_create_cube(&candidates[BUILTIN_CUBE]) != RETURN_SUCCESS) {
        goto failure;
    }
    if (Mesh_create_cylinder(&candidates[BUILTIN_CYLINDER], 16) !=
        RETURN_SUCCESS) {
        goto failure;
    }
    if (Mesh_create_cone(&candidates[BUILTIN_CONE], 16) != RETURN_SUCCESS) {
        goto failure;
    }
    if (Mesh_create_sphere_uv(&candidates[BUILTIN_SPHERE_UV], 16) !=
        RETURN_SUCCESS) {
        goto failure;
    }

    for (uint32_t i = 0; i < BUILTIN_COUNT; ++i) {
        MeshHandle handle = Renderer_register_mesh(renderer, &candidates[i]);
        if (handle == INVALID_MESH_HANDLE) {
            goto failure;
        }
        // Shallow-transferred to renderer.  Clear local copy.
        candidates[i] = (Mesh){0};

        if (handle != i) goto failure;
        handles[i] = handle;
    }

    renderer->builtin.plane = handles[BUILTIN_PLANE];
    renderer->builtin.disc = handles[BUILTIN_DISC];
    renderer->builtin.cube = handles[BUILTIN_CUBE];
    renderer->builtin.cylinder = handles[BUILTIN_CYLINDER];
    renderer->builtin.cone = handles[BUILTIN_CONE];
    renderer->builtin.sphere_uv = handles[BUILTIN_SPHERE_UV];

    return RETURN_SUCCESS;

failure:
    for (uint32_t i = 0; i < BUILTIN_COUNT; ++i) {
        Mesh_release_cpu_arrays(&candidates[i]);
    }

    while (renderer->meshes.count > 0) {
        uint32_t index = renderer->meshes.count - 1;
        Mesh* mesh = &renderer->meshes.items[index];

        Mesh_release_gpu_buffers(mesh);
        Mesh_release_cpu_arrays(mesh);
        *mesh = (Mesh){0};
        renderer->meshes.count = index;
    }

    return RETURN_FAILURE;
}

ReturnStatus Renderer_create_depth_texture(
    Renderer* renderer,
    u32 width,
    u32 height,
    const char* label,
    WGPUTextureFormat format
) {
    // Texture creation
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
    WGPUTexture new_texture =
        wgpuDeviceCreateTexture(renderer->device, &depth_texture_desc);
    if (new_texture == NULL) {
        log_error("Failed to create new depth texture\n");
        return RETURN_FAILURE;
    }

    // Texture view creation
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
    WGPUTextureView new_view =
        wgpuTextureCreateView(new_texture, &depth_texture_view_desc);
    if (new_view == NULL) {
        log_error("Failed to create new depth texture view\n");
        wgpuTextureRelease(new_texture);
        return RETURN_FAILURE;
    }

    if (renderer->depth_texture_view != NULL) {
        wgpuTextureViewRelease(renderer->depth_texture_view);
    }
    if (renderer->depth_texture != NULL) {
        wgpuTextureRelease(renderer->depth_texture);
    }

    renderer->depth_texture = new_texture;
    renderer->depth_texture_view = new_view;
    return RETURN_SUCCESS;
}

// Expects "vs_main" and "fs_main"
ReturnStatus Renderer_create_render_pipeline(
    Renderer* renderer,
    WGPURenderPipeline* pipeline,
    const RendererPipelineDesc* desc
) {
    if (!desc->shader_data || desc->shader_size == 0) return RETURN_FAILURE;

    char label_buffer[256] = {0};
    snprintf(
        label_buffer, sizeof(label_buffer), "%s Shader Module", desc->label
    );
    WGPUShaderSourceWGSL wgsl_desc = {
        .chain.sType = WGPUSType_ShaderSourceWGSL,
        .code = {
            .data = (const char*)desc->shader_data,
            .length = desc->shader_size,
        }
    };
    WGPUShaderModuleDescriptor shader_desc = {
        .nextInChain = &wgsl_desc.chain,
        .label = {label_buffer, WGPU_STRLEN},
    };
    WGPUShaderModule shader =
        wgpuDeviceCreateShaderModule(renderer->device, &shader_desc);
    if (!shader) {
        log_error("Failed to create solid shader module\n");
        return RETURN_FAILURE;
    }

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
        .format = desc->texture_format,
        .blend = &blend_state,
        .writeMask = WGPUColorWriteMask_All,
    };
    WGPUConstantEntry fragment_constant = {
        .key = {"encode_output_srgb", WGPU_STRLEN},
        .value = desc->encode_output_srgb ? 1.0 : 0.0,
    };
    WGPUFragmentState frag_state = {
        .module = shader,
        .entryPoint = {"fs_main", WGPU_STRLEN},
        .constantCount = 1,
        .constants = &fragment_constant,
        .targets = &color_target_state,
        .targetCount = 1,
    };
    WGPUDepthStencilState depth_pencil_state = {
        .format = desc->depth_texture_format,
        .depthWriteEnabled = desc->depth_write_enabled,
        .depthCompare = desc->depth_compare,
    };
    snprintf(label_buffer, sizeof(label_buffer), "%s Layout", desc->label);
    WGPUPipelineLayoutDescriptor solid_pipeline_layout_desc = {
        .label = {label_buffer, WGPU_STRLEN},
        .bindGroupLayouts = &desc->bind_group_layout,
        .bindGroupLayoutCount = 1,
    };
    WGPUPipelineLayout solid_pipeline_layout = wgpuDeviceCreatePipelineLayout(
        renderer->device, &solid_pipeline_layout_desc
    );
    if (!solid_pipeline_layout) {
        log_error("Failed to create solid pipeline layout\n");
        wgpuShaderModuleRelease(shader);
        return RETURN_FAILURE;
    }
    WGPURenderPipelineDescriptor solid_pipeline_desc = {
        .label = {desc->label, WGPU_STRLEN},
        .layout = solid_pipeline_layout,
        .vertex = vert_state,
        .fragment = &frag_state,
        .depthStencil = &depth_pencil_state,
        .primitive =
            (WGPUPrimitiveState){
                .topology = desc->topology,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = desc->cull_mode,
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
    wgpuPipelineLayoutRelease(solid_pipeline_layout);
    wgpuShaderModuleRelease(shader);
    if (*pipeline == NULL) {
        log_error("Failed to create solid render pipeline\n");
        return RETURN_FAILURE;
    }
    return RETURN_SUCCESS;
}

static ReturnStatus Renderer_request_adapter(
    Renderer* renderer, const WGPURequestAdapterOptions* options
) {
    if (renderer == NULL || renderer->instance == NULL || options == NULL) {
        return RETURN_FAILURE;
    }

    ReturnStatus status = RETURN_FAILURE;
    WgpuAdapterRequestContext* context = &renderer->adapter_request;
    *context = (WgpuAdapterRequestContext){
        .completed = false,
        .status = WGPURequestAdapterStatus_Unknown,
        .adapter = NULL,
    };

    WGPURequestAdapterCallbackInfo callback_info = {
        .mode = WGPUCallbackMode_AllowProcessEvents,
        .callback = adapter_request_callback,
        .userdata1 = context,
    };
    const bool* completion_flags[] = {&context->completed};

    WGPUFuture future =
        wgpuInstanceRequestAdapter(renderer->instance, options, callback_info);

    WGPUFutureWaitInfo wait_info = {
        .future = future,
        .completed = false,
    };

    status = wait_for_gpu_futures(
        renderer->instance,
        &renderer->wait_config,
        &wait_info,
        1,
        completion_flags,
        1,
        WGPU_INIT_TIMEOUT_NS
    );

    if (status != RETURN_SUCCESS) goto cleanup;
    if (context->status != WGPURequestAdapterStatus_Success ||
        context->adapter == NULL) {
        status = RETURN_FAILURE;
        goto cleanup;
    }

    renderer->adapter = context->adapter;
    context->adapter = NULL;
    status = RETURN_SUCCESS;

cleanup:
    if (context->adapter != NULL) {
        wgpuAdapterRelease(context->adapter);
        context->adapter = NULL;
    }
    return status;
}

static ReturnStatus Renderer_request_device(Renderer* renderer) {
    if (renderer == NULL || renderer->instance == NULL ||
        renderer->adapter == NULL) {
        return RETURN_FAILURE;
    }

    ReturnStatus status = RETURN_FAILURE;
    WgpuDeviceRequestContext* context = &renderer->device_request;

    *context = (WgpuDeviceRequestContext){
        .completed = false,
        .status = WGPURequestDeviceStatus_Unknown,
        .device = NULL,
    };

    renderer->device_lost = (WgpuDeviceLostContext){
        .completed = false,
        .reason = WGPUDeviceLostReason_Unknown,
    };
    renderer->device_lost_future = (WGPUFuture){0};
    renderer->device_lost_future_available = false;

    WGPUDeviceDescriptor device_desc = {
        .label = {"Device", WGPU_STRLEN},
        .deviceLostCallbackInfo = {
            .mode = WGPUCallbackMode_AllowProcessEvents,
            .callback = device_lost_callback,
            .userdata1 = &renderer->device_lost,
        },
    };

    WGPURequestDeviceCallbackInfo callback_info = {
        .mode = WGPUCallbackMode_AllowProcessEvents,
        .callback = device_request_callback,
        .userdata1 = context,
    };

    const bool* completion_flags[] = {&context->completed};

    WGPUFuture future = wgpuAdapterRequestDevice(
        renderer->adapter, &device_desc, callback_info
    );

    WGPUFutureWaitInfo wait_info = {
        .future = future,
        .completed = false,
    };

    status = wait_for_gpu_futures(
        renderer->instance,
        &renderer->wait_config,
        &wait_info,
        1,
        completion_flags,
        1,
        WGPU_INIT_TIMEOUT_NS
    );

    if (status != RETURN_SUCCESS) goto cleanup;

    if (context->status != WGPURequestDeviceStatus_Success ||
        context->device == NULL) {
        status = RETURN_FAILURE;
        goto cleanup;
    }

    renderer->device = context->device;
    context->device = NULL;

    if (renderer->wait_config.timed_wait_enabled &&
        renderer->wait_config.timed_wait_max_count >= 2) {
        renderer->device_lost_future =
            wgpuDeviceGetLostFuture(renderer->device);
        if (renderer->device_lost_future.id != 0) {
            renderer->device_lost_future_available = true;
        } else {
            log_warn("Device-lost future unavailable. Using bounded polling\n");
        }
    }
    status = RETURN_SUCCESS;

cleanup:
    if (context->device != NULL) {
        wgpuDeviceRelease(context->device);
        context->device = NULL;
    }

    return status;
}

ReturnStatus Renderer_init_windowed(
    Renderer* renderer,
    const WGPUInstance instance,
    const WGPUSurface surface,
    const u32 width,
    const u32 height
) {
    renderer->render_mode = RENDER_MODE_WINDOWED;
    renderer->instance = instance;
    renderer->render_target.windowed.surface = surface;

    // Adapter request
    WGPURequestAdapterOptions adapter_options = {
        // Don't need a surface
        .compatibleSurface = renderer->render_target.windowed.surface,
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .forceFallbackAdapter = false,
    };

    ReturnStatus adapter_status =
        Renderer_request_adapter(renderer, &adapter_options);
    if (adapter_status != RETURN_SUCCESS) return adapter_status;
    log_debug("Adapter request successful\n");

    // Device request
    ReturnStatus device_status = Renderer_request_device(renderer);
    if (device_status != RETURN_SUCCESS) return device_status;
    log_debug("Device request successful\n");

    // Get device queue
    renderer->queue = wgpuDeviceGetQueue(renderer->device);
    if (renderer->queue == NULL) {
        log_error("Failed to get device queue\n");
        return RETURN_FAILURE;
    }

    // Create render target
    WGPUSurfaceCapabilities surface_caps = {0};
    WGPUStatus surface_caps_status = wgpuSurfaceGetCapabilities(
        renderer->render_target.windowed.surface,
        renderer->adapter,
        &surface_caps
    );
    if (surface_caps_status != WGPUStatus_Success ||
        surface_caps.formatCount == 0) {
        log_error("No supported surface formats found\n");
        wgpuSurfaceCapabilitiesFreeMembers(surface_caps);
        return RETURN_FAILURE;
    }
    log_debug("%ld surface formats found.\n", surface_caps.formatCount);
    WGPUTextureFormat texture_format = WGPUTextureFormat_Undefined;
    bool output_requires_srgb_encoding = false;

    for (size_t i = 0; i < surface_caps.formatCount; ++i) {
        WGPUTextureFormat format = surface_caps.formats[i];
        if (format == WGPUTextureFormat_BGRA8UnormSrgb ||
            format == WGPUTextureFormat_RGBA8UnormSrgb) {
            texture_format = format;
            break;
        }
    }
    if (texture_format == WGPUTextureFormat_Undefined) {
        for (size_t i = 0; i < surface_caps.formatCount; ++i) {
            WGPUTextureFormat format = surface_caps.formats[i];
            if (format == WGPUTextureFormat_BGRA8Unorm ||
                format == WGPUTextureFormat_RGBA8Unorm) {
                texture_format = format;
                output_requires_srgb_encoding = true;
                break;
            }
        }
    }
    if (texture_format == WGPUTextureFormat_Undefined) {
        log_error("No supported SDR surface formats found\n");
        wgpuSurfaceCapabilitiesFreeMembers(surface_caps);
        return RETURN_FAILURE;
    }
    wgpuSurfaceCapabilitiesFreeMembers(surface_caps);

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
        "Configured surface size: [%d, %d]\n",
        renderer->render_target.windowed.surface_config.width,
        renderer->render_target.windowed.surface_config.height
    );

    // Create depth texture
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    if (Renderer_create_depth_texture(
            renderer, width, height, "Depth Texture", depth_texture_format
        ) != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }

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
    if (renderer->uniform_buffer == NULL) {
        log_error("Failed to create uniform buffer\n");
        return RETURN_FAILURE;
    }

    // Create meshes
    ReturnStatus status = Renderer_register_builtin_meshes(renderer);
    if (status != RETURN_SUCCESS) return status;

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
    if (bind_group_layout == NULL) {
        log_error("Failed to create bind group layout\n");
        return RETURN_FAILURE;
    }

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
    if (renderer->uniform_bind_group == NULL) {
        log_error("Failed to create uniform bind group\n");
        wgpuBindGroupLayoutRelease(bind_group_layout);
        return RETURN_FAILURE;
    }

    // Create solid render pipeline
    RendererPipelineDesc solid_pipeline_desc = {
        .label = "Solid Pipeline",
        .shader_data = raijin_solid_shader_wgsl,
        .shader_size = raijin_solid_shader_wgsl_size,
        .texture_format = texture_format,
        .depth_texture_format = depth_texture_format,
        .bind_group_layout = bind_group_layout,
        .topology = WGPUPrimitiveTopology_TriangleList,
        .cull_mode = WGPUCullMode_None,
        .depth_compare = WGPUCompareFunction_Less,
        .depth_write_enabled = true,
        .encode_output_srgb = output_requires_srgb_encoding
    };
    ReturnStatus create_pipeline_status = Renderer_create_render_pipeline(
        renderer, &renderer->solid_pipeline, &solid_pipeline_desc
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        wgpuBindGroupLayoutRelease(bind_group_layout);
        return RETURN_FAILURE;
    }

    // Create edges render pipeline
    RendererPipelineDesc edges_pipeline_desc = {
        .label = "Edges Pipeline",
        .shader_data = raijin_edges_shader_wgsl,
        .shader_size = raijin_edges_shader_wgsl_size,
        .texture_format = texture_format,
        .depth_texture_format = depth_texture_format,
        .bind_group_layout = bind_group_layout,
        .topology = WGPUPrimitiveTopology_LineList,
        .cull_mode = WGPUCullMode_None,
        .depth_compare = WGPUCompareFunction_LessEqual,
        .depth_write_enabled = false,
        .encode_output_srgb = output_requires_srgb_encoding
    };
    create_pipeline_status = Renderer_create_render_pipeline(
        renderer, &renderer->edges_pipeline, &edges_pipeline_desc
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        wgpuBindGroupLayoutRelease(bind_group_layout);
        return RETURN_FAILURE;
    }

    wgpuBindGroupLayoutRelease(bind_group_layout);
    return RETURN_SUCCESS;
}

ReturnStatus Renderer_init_headless(
    Renderer* renderer, const WGPUInstance instance, u32 width, u32 height
) {
    renderer->render_mode = RENDER_MODE_HEADLESS;
    renderer->instance = instance;

    // Adapter request
    WGPURequestAdapterOptions adapter_options = {
        // Don't need a surface
        .compatibleSurface = NULL,
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .forceFallbackAdapter = false,
    };
    ReturnStatus adapter_status =
        Renderer_request_adapter(renderer, &adapter_options);
    if (adapter_status != RETURN_SUCCESS) return adapter_status;
    log_debug("Adapter request successful\n");

    // Device request
    ReturnStatus device_status = Renderer_request_device(renderer);
    if (device_status != RETURN_SUCCESS) return device_status;
    log_debug("Device request successful\n");

    // Get device queue
    renderer->queue = wgpuDeviceGetQueue(renderer->device);
    if (renderer->queue == NULL) {
        log_error("Failed to get device queue\n");
        return RETURN_FAILURE;
    }

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
    if (renderer->render_target.headless.texture == NULL) {
        log_error("Failed to create headless render texture\n");
        return RETURN_FAILURE;
    }

    // Create depth texture
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    if (Renderer_create_depth_texture(
            renderer, width, height, "Depth Texture", depth_texture_format
        ) != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }

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
    if (renderer->uniform_buffer == NULL) {
        log_error("Failed to create uniform buffer\n");
        return RETURN_FAILURE;
    }

    // Create meshes
    ReturnStatus status = Renderer_register_builtin_meshes(renderer);
    if (status != RETURN_SUCCESS) return status;

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
    if (bind_group_layout == NULL) {
        log_error("Failed to create bind group layout\n");
        return RETURN_FAILURE;
    }

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
    if (renderer->uniform_bind_group == NULL) {
        log_error("Failed to create uniform bind group\n");
        wgpuBindGroupLayoutRelease(bind_group_layout);
        return RETURN_FAILURE;
    }

    // Create solid render pipeline
    RendererPipelineDesc solid_pipeline_desc = {
        .label = "Solid Pipeline",
        .shader_data = raijin_solid_shader_wgsl,
        .shader_size = raijin_solid_shader_wgsl_size,
        .texture_format = texture_format,
        .depth_texture_format = depth_texture_format,
        .bind_group_layout = bind_group_layout,
        .topology = WGPUPrimitiveTopology_TriangleList,
        .cull_mode = WGPUCullMode_None,
        .depth_compare = WGPUCompareFunction_Less,
        .depth_write_enabled = true,
        .encode_output_srgb = false
    };
    ReturnStatus create_pipeline_status = Renderer_create_render_pipeline(
        renderer, &renderer->solid_pipeline, &solid_pipeline_desc
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        wgpuBindGroupLayoutRelease(bind_group_layout);
        return RETURN_FAILURE;
    }

    // Create edges render pipeline

    RendererPipelineDesc edges_pipeline_desc = {
        .label = "Edges Pipeline",
        .shader_data = raijin_edges_shader_wgsl,
        .shader_size = raijin_edges_shader_wgsl_size,
        .texture_format = texture_format,
        .depth_texture_format = depth_texture_format,
        .bind_group_layout = bind_group_layout,
        .topology = WGPUPrimitiveTopology_LineList,
        .cull_mode = WGPUCullMode_None,
        .depth_compare = WGPUCompareFunction_LessEqual,
        .depth_write_enabled = false,
        .encode_output_srgb = false
    };
    create_pipeline_status = Renderer_create_render_pipeline(
        renderer, &renderer->edges_pipeline, &edges_pipeline_desc
    );
    if (create_pipeline_status != RETURN_SUCCESS) {
        wgpuBindGroupLayoutRelease(bind_group_layout);
        return RETURN_FAILURE;
    }

    wgpuBindGroupLayoutRelease(bind_group_layout);
    return RETURN_SUCCESS;
}

// TODO (mmckenna): Target for arena allocator
ReturnStatus Renderer_render_mesh(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
) {
    ReturnStatus status = RETURN_SUCCESS;
    InstanceArray instances;
    InstanceArray_init(&instances);
    for (u32 i = 0; i < renderer->draw_commands.count; ++i) {
        DrawCommand* cmd = &renderer->draw_commands.items[i];
        if (cmd->mesh_handle == mesh_handle) {
            if (InstanceArray_push(&instances, cmd->instance) != RETURN_OK) {
                log_error("Failed to push to instance array\n");
                status = RETURN_FAILURE;
                goto cleanup;
            }
        }
    }

    // No instances to render
    if (instances.count == 0) {
        goto cleanup;
    }

    Mesh* mesh = &renderer->meshes.items[mesh_handle];
    if (instances.count > mesh->instance_capacity) {
        if (Mesh_realloc_instance_buffer(
                mesh, renderer->device, instances.count
            ) != RETURN_SUCCESS) {
            log_error("Failed to reallocate instance array\n");
            status = RETURN_FAILURE;
            goto cleanup;
        }
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

cleanup:
    InstanceArray_free(&instances);
    return status;
}

// TODO (mmckenna): Target for arena allocator
ReturnStatus Renderer_render_mesh_edges(
    Renderer* renderer,
    const MeshHandle mesh_handle,
    const WGPURenderPassEncoder render_pass_encoder
) {
    ReturnStatus status = RETURN_SUCCESS;
    InstanceArray instances;
    InstanceArray_init(&instances);

    Mesh* mesh = &renderer->meshes.items[mesh_handle];
    // Allow mesh generation without creating optional edge buffers
    if (mesh->edge_indices.count == 0) {
        goto cleanup;
    }

    for (u32 i = 0; i < renderer->draw_commands.count; ++i) {
        DrawCommand* cmd = &renderer->draw_commands.items[i];
        if (cmd->mesh_handle == mesh_handle) {
            Instance instance = {0};
            memcpy(&instance, &cmd->instance, sizeof(instance));
            glm_vec4_one(instance.color);
            if (InstanceArray_push(&instances, instance) != RETURN_OK) {
                log_error("Failed to push to instance array\n");
                status = RETURN_FAILURE;
                goto cleanup;
            }
        }
    }

    // No instances to render
    if (instances.count == 0) {
        goto cleanup;
    }
    if (instances.count > mesh->edge_instance_capacity) {
        if (Mesh_realloc_edge_instance_buffer(
                mesh, renderer->device, instances.count
            ) != RETURN_SUCCESS) {
            log_error("Failed to reallocate instance array\n");
            status = RETURN_FAILURE;
            goto cleanup;
        }
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
        WGPUIndexFormat_Uint32,
        0,
        mesh->edge_indices.count * sizeof(u32)
    );
    wgpuRenderPassEncoderDrawIndexed(
        render_pass_encoder, mesh->edge_indices.count, instances.count, 0, 0, 0
    );

cleanup:
    InstanceArray_free(&instances);
    return status;
}

ReturnStatus Renderer_render_pass_solid(
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
    if (render_pass_encoder == NULL) {
        log_error("Failed to begin solid render pass\n");
        return RETURN_FAILURE;
    }
    wgpuRenderPassEncoderSetPipeline(
        render_pass_encoder, renderer->solid_pipeline
    );
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 0, renderer->uniform_bind_group, 0, NULL
    );

    // Render all meshes
    for (u32 i = 0; i < renderer->meshes.count; ++i) {
        if (Renderer_render_mesh(renderer, i, render_pass_encoder) !=
            RETURN_SUCCESS) {
            log_error("Failed to render mesh %d\n", i);
            wgpuRenderPassEncoderEnd(render_pass_encoder);
            wgpuRenderPassEncoderRelease(render_pass_encoder);
            return RETURN_FAILURE;
        }
    }

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
    return RETURN_SUCCESS;
}

ReturnStatus Renderer_render_pass_edges(
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
    if (render_pass_encoder == NULL) {
        log_error("Failed to begin edges render pass\n");
        return RETURN_FAILURE;
    }
    wgpuRenderPassEncoderSetPipeline(
        render_pass_encoder, renderer->edges_pipeline
    );
    wgpuRenderPassEncoderSetBindGroup(
        render_pass_encoder, 0, renderer->uniform_bind_group, 0, NULL
    );

    // Render all meshes
    for (u32 i = 0; i < renderer->meshes.count; ++i) {
        if (Renderer_render_mesh_edges(renderer, i, render_pass_encoder) !=
            RETURN_SUCCESS) {
            log_error("Failed to render mesh edges %d\n", i);
            wgpuRenderPassEncoderEnd(render_pass_encoder);
            wgpuRenderPassEncoderRelease(render_pass_encoder);
            return RETURN_FAILURE;
        }
    }

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
    return RETURN_SUCCESS;
}

ReturnStatus Renderer_render_to_view(
    Renderer* renderer, const WGPUTextureView texture_view
) {
    ReturnStatus status = RETURN_FAILURE;
    WGPUCommandEncoder command_encoder = NULL;
    WGPUCommandBuffer command_buffer = NULL;

    WGPUCommandEncoderDescriptor command_encoder_desc = {
        .label = {"Encoder", WGPU_STRLEN}

    };
    command_encoder =
        wgpuDeviceCreateCommandEncoder(renderer->device, &command_encoder_desc);
    if (command_encoder == NULL) {
        log_error("Failed to create command encoder\n");
        goto cleanup;
    }

    if (Renderer_render_pass_solid(renderer, command_encoder, texture_view) !=
        RETURN_SUCCESS) {
        log_error("Failed solid render pass\n");
        goto cleanup;
    }

    // Optional edges render pass
    if (renderer->enable_edges) {
        if (Renderer_render_pass_edges(
                renderer, command_encoder, texture_view
            ) != RETURN_SUCCESS) {
            log_error("Failed edges render pass\n");
            goto cleanup;
        }
    }

    WGPUCommandBufferDescriptor command_buffer_desc = {
        .label = {"Command Buffer", WGPU_STRLEN}
    };
    command_buffer =
        wgpuCommandEncoderFinish(command_encoder, &command_buffer_desc);
    if (command_buffer == NULL) {
        log_error("Failed to create command buffer\n");
        status = RETURN_FAILURE;
        goto cleanup;
    }

    wgpuQueueSubmit(renderer->queue, 1, &command_buffer);
    status = RETURN_SUCCESS;

cleanup:
    if (command_buffer != NULL) wgpuCommandBufferRelease(command_buffer);
    if (command_encoder != NULL) wgpuCommandEncoderRelease(command_encoder);
    return status;
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
            if (texture_view == NULL) {
                log_error("Failed to create headless texture view\n");
                status = RETURN_FAILURE;
                break;
            }
            if (Renderer_render_to_view(renderer, texture_view) !=
                RETURN_SUCCESS) {
                status = RETURN_FAILURE;
            }
            wgpuTextureViewRelease(texture_view);
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

            switch (surface_texture.status) {
                case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
                    break;
                case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
                    log_warn("Sub-optimal WGPU surface texture status\n");
                    break;
                case WGPUSurfaceGetCurrentTextureStatus_Timeout:
                    log_warn("Timed out acquiring surface texture\n");
                    status = RETURN_TIMEOUT;
                    break;
                case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
                    log_error("Device lost while acquiring surface texture\n");
                    status = RETURN_DEVICE_LOST;
                    break;
                case WGPUSurfaceGetCurrentTextureStatus_Outdated:
                    log_error("Surface is outdated\n");
                    status = RETURN_FAILURE;
                    break;
                case WGPUSurfaceGetCurrentTextureStatus_Lost:
                    log_error("Surface was lost\n");
                    status = RETURN_FAILURE;
                    break;
                case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
                    log_error("Out of memory acquiring surface texture\n");
                    status = RETURN_FAILURE;
                    break;
                case WGPUSurfaceGetCurrentTextureStatus_Error:
                default:
                    log_error(
                        "Failed to acquire surface texture: status=%u\n",
                        (uint32_t)surface_texture.status
                    );
                    status = RETURN_FAILURE;
                    break;
            }

            if (status != RETURN_SUCCESS) {
                if (surface_texture.texture != NULL) {
                    wgpuTextureRelease(surface_texture.texture);
                }
                break;
            }

            if (surface_texture.texture == NULL) {
                log_error("Surface texture must not be NULL\n");
                status = RETURN_FAILURE;
                break;
            }

            texture_view = wgpuTextureCreateView(
                surface_texture.texture, &texture_view_desc
            );
            if (texture_view == NULL) {
                log_error("Failed to create windowed texture view\n");
                wgpuTextureRelease(surface_texture.texture);
                status = RETURN_FAILURE;
                break;
            }
            if (Renderer_render_to_view(renderer, texture_view) !=
                RETURN_SUCCESS) {
                wgpuTextureViewRelease(texture_view);
                wgpuTextureRelease(surface_texture.texture);
                status = RETURN_FAILURE;
                break;
            }
            WGPUStatus present_status =
                wgpuSurfacePresent(renderer->render_target.windowed.surface);
            // TODO (mmckenna): Handle each status variant
            if (present_status != WGPUStatus_Success) {
                log_error("Failed to present surface\n");
                status = RETURN_FAILURE;
            }
            wgpuTextureViewRelease(texture_view);
            wgpuTextureRelease(surface_texture.texture);
        } break;
        default: {
            log_error("Unknown render mode: %d\n", renderer->render_mode);
            status = RETURN_FAILURE;
        } break;
    }
    renderer->draw_commands.count = 0;
    return status;
}

static void Renderer_cancel_buffer_map(Renderer* renderer) {
    if (renderer == NULL) return;

    WgpuBufferMapContext* ctx = &renderer->buffer_map;
    if (!ctx->active || ctx->staging_buffer == NULL || ctx->unmap_requested) {
        return;
    }

    if (ctx->completed && ctx->status != WGPUMapAsyncStatus_Success) return;

    ctx->unmap_requested = true;
    wgpuBufferUnmap(ctx->staging_buffer);
}

static void Renderer_retire_buffer_map(Renderer* renderer) {
    if (renderer == NULL) return;

    WgpuBufferMapContext* ctx = &renderer->buffer_map;
    if (!ctx->active || !ctx->completed) return;
    if (ctx->status == WGPUMapAsyncStatus_Success && !ctx->unmap_requested &&
        ctx->staging_buffer != NULL) {
        ctx->unmap_requested = true;
        wgpuBufferUnmap(ctx->staging_buffer);
    }
    if (ctx->staging_buffer != NULL) wgpuBufferRelease(ctx->staging_buffer);

    *ctx = (WgpuBufferMapContext){.status = WGPUMapAsyncStatus_Unknown};
}

void Renderer_destroy(Renderer* renderer) {
    Renderer_cancel_buffer_map(renderer);
    if (renderer->instance != NULL) {
        wgpuInstanceProcessEvents(renderer->instance);
    }
    Renderer_retire_buffer_map(renderer);

    DrawCommandArray_free(&renderer->draw_commands);

    for (u32 i = 0; i < renderer->meshes.count; ++i) {
        Mesh* mesh = &renderer->meshes.items[i];
        Mesh_release_gpu_buffers(mesh);
        Mesh_release_cpu_arrays(mesh);
        log_debug("Mesh %d free.\n", i);
    }
    MeshArray_free(&renderer->meshes);
    log_debug("Mesh array free.\n");

    if (renderer->uniform_bind_group != NULL) {
        wgpuBindGroupRelease(renderer->uniform_bind_group);
        log_debug("Uniform bind group released.\n");
    }
    if (renderer->uniform_buffer != NULL) {
        wgpuBufferRelease(renderer->uniform_buffer);
        log_debug("Uniform buffer released.\n");
    }
    if (renderer->solid_pipeline != NULL) {
        wgpuRenderPipelineRelease(renderer->solid_pipeline);
        log_debug("Solid pipeline released.\n");
    }
    if (renderer->edges_pipeline != NULL) {
        wgpuRenderPipelineRelease(renderer->edges_pipeline);
        log_debug("Edges pipeline released.\n");
    }
    if (renderer->depth_texture_view != NULL) {
        wgpuTextureViewRelease(renderer->depth_texture_view);
        log_debug("Depth texture view released.\n");
    }
    if (renderer->depth_texture != NULL) {
        wgpuTextureRelease(renderer->depth_texture);
        log_debug("Depth texture released.\n");
    }
    switch (renderer->render_mode) {
        case RENDER_MODE_WINDOWED: {
            if (renderer->render_target.windowed.surface != NULL) {
                wgpuSurfaceUnconfigure(
                    renderer->render_target.windowed.surface
                );
                wgpuSurfaceRelease(renderer->render_target.windowed.surface);
                log_debug("Window surface released.\n");
            }
        } break;
        case RENDER_MODE_HEADLESS: {
            if (renderer->render_target.headless.texture != NULL) {
                wgpuTextureRelease(renderer->render_target.headless.texture);
                log_debug("Headless texture released.\n");
            }
        } break;
    }

    if (renderer->buffer_map.active &&
        renderer->buffer_map.staging_buffer != NULL) {
        wgpuBufferRelease(renderer->buffer_map.staging_buffer);
        renderer->buffer_map.staging_buffer = NULL;
    }

    if (renderer->queue != NULL) {
        wgpuQueueRelease(renderer->queue);
        log_debug("Queue released.\n");
    }

    if (renderer->device_request.device != NULL) {
        wgpuDeviceRelease(renderer->device_request.device);
        renderer->device_request.device = NULL;
    }
    if (renderer->device != NULL) {
        wgpuDeviceRelease(renderer->device);
        log_debug("Device released.\n");
    }

    if (renderer->adapter_request.adapter != NULL) {
        wgpuAdapterRelease(renderer->adapter_request.adapter);
        renderer->adapter_request.adapter = NULL;
    }
    if (renderer->adapter != NULL) {
        wgpuAdapterRelease(renderer->adapter);
        log_debug("Adapter released.\n");
    }

    if (renderer->instance != NULL) {
        wgpuInstanceRelease(renderer->instance);
        log_debug("Instance released.\n");
    }

    renderer->buffer_map = (WgpuBufferMapContext){
        .status = WGPUMapAsyncStatus_Unknown,
    };
}

ReturnStatus Renderer_handle_resize(Renderer* renderer, u32 width, u32 height) {
    // Avoid zero-size textures (minimized window)
    if (width == 0 || height == 0) {
        log_error("Invalid render size: width=%d, height=%d\n", width, height);
        return RETURN_FAILURE;
    }

    // Recreate depth texture
    if (Renderer_create_depth_texture(
            renderer,
            width,
            height,
            "Depth Texture",
            WGPUTextureFormat_Depth24Plus
        ) != RETURN_SUCCESS) {
        return RETURN_FAILURE;
    }

    renderer->render_target.windowed.surface_config.width = width;
    renderer->render_target.windowed.surface_config.height = height;
    wgpuSurfaceConfigure(
        renderer->render_target.windowed.surface,
        &renderer->render_target.windowed.surface_config
    );

    log_debug(
        "Surface configured successfully: width=%d, height=%d\n", width, height
    );
    return RETURN_SUCCESS;
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
    Renderer* renderer, u32 width, u32 height, u8* buffer, u64 buffer_capacity
) {
    ReturnStatus status = RETURN_FAILURE;
    WGPUBuffer staging_buffer = NULL;
    WGPUCommandEncoder encoder = NULL;
    WGPUCommandBuffer command_buffer = NULL;

    if (buffer == NULL) {
        log_error("Copy frame destination buffer was NULL.\n");
        goto cleanup;
    }

    if (renderer->render_mode != RENDER_MODE_HEADLESS) {
        log_error(
            "You must be using headless render mode to copy frame to buffer.\n"
        );
        goto cleanup;
    }

    wgpuInstanceProcessEvents(renderer->instance);
    if (renderer->device_lost.completed) {
        Renderer_cancel_buffer_map(renderer);
        wgpuInstanceProcessEvents(renderer->instance);
        Renderer_retire_buffer_map(renderer);
        return RETURN_DEVICE_LOST;
    }
    if (renderer->buffer_map.active) {
        if (!renderer->buffer_map.completed) return RETURN_TIMEOUT;

        WGPUMapAsyncStatus previous_status = renderer->buffer_map.status;
        Renderer_retire_buffer_map(renderer);

        if (previous_status == WGPUMapAsyncStatus_InstanceDropped) {
            return RETURN_DEVICE_LOST;
        }
    }

    // RGBA8 assumed
    const u64 bytes_per_pixel = 4;
    const u64 alignment = 256;
    const u64 unpadded_bytes_per_row = (u64)width * bytes_per_pixel;

    if (unpadded_bytes_per_row > UINT32_MAX - (alignment - 1)) {
        log_error(
            "Unexpected unpadded bytes per row: %" PRIu64 "\n",
            unpadded_bytes_per_row
        );
        goto cleanup;
    }

    const u64 padded_bytes_per_row =
        (unpadded_bytes_per_row + alignment - 1) & ~(alignment - 1);

    if (height != 0 && padded_bytes_per_row > UINT64_MAX / (u64)height) {
        log_error(
            "Unexpected padded bytes per row: %" PRIu64 "\n",
            padded_bytes_per_row
        );
        goto cleanup;
    }

    const u64 staging_buffer_size = padded_bytes_per_row * (u64)height;
    const u64 output_buffer_size = unpadded_bytes_per_row * (u64)height;
    if (staging_buffer_size > SIZE_MAX || output_buffer_size > SIZE_MAX) {
        log_error("Readback size exceeds SIZE_MAX\n");
        goto cleanup;
    }

    const size_t staging_size = (size_t)staging_buffer_size;
    const size_t output_row_size = (size_t)unpadded_bytes_per_row;
    const size_t staging_row_size = (size_t)padded_bytes_per_row;

    if (buffer_capacity < output_buffer_size) {
        log_error(
            "Buffer of size %" PRIu64 " is too small.  Expected %" PRIu64
            " minimum.\n",
            buffer_capacity,
            output_buffer_size
        );
        goto cleanup;
    }

    staging_buffer = create_buffer(
        renderer->device,
        staging_buffer_size,
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead,
        "Readback Staging Buffer"
    );
    if (staging_buffer == NULL) {
        log_error("Failed to create staging buffer.\n");
        goto cleanup;
    }

    WGPUCommandEncoderDescriptor encoder_desc = {
        .label = {"Texture Readback Encoder", WGPU_STRLEN},
    };
    encoder = wgpuDeviceCreateCommandEncoder(renderer->device, &encoder_desc);
    if (!encoder) {
        goto cleanup;
    }

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
            .bytesPerRow = (uint32_t)padded_bytes_per_row,
            .rowsPerImage = height,
        },
    };

    WGPUExtent3D copy_size = {width, height, 1};
    wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &copy_size);

    WGPUCommandBufferDescriptor cmd_buf_desc = {
        .label = {"Texture Readback Command Buffer", WGPU_STRLEN},
    };
    command_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buf_desc);
    if (!command_buffer) {
        goto cleanup;
    }
    wgpuQueueSubmit(renderer->queue, 1, &command_buffer);

    WgpuBufferMapContext* map_ctx = &renderer->buffer_map;
    *map_ctx = (WgpuBufferMapContext){
        .active = true,
        .completed = false,
        .unmap_requested = false,
        .status = WGPUMapAsyncStatus_Unknown,
        .staging_buffer = staging_buffer,
    };
    staging_buffer = NULL;
    WGPUBufferMapCallbackInfo map_callback_info = {
        .mode = WGPUCallbackMode_AllowProcessEvents,
        .callback = buffer_map_callback,
        .userdata1 = map_ctx,
    };

    WGPUFuture map_future = wgpuBufferMapAsync(
        map_ctx->staging_buffer,
        WGPUMapMode_Read,
        0,
        staging_size,
        map_callback_info
    );

    WGPUFutureWaitInfo wait_infos[2] = {
        {
            .future = map_future,
            .completed = false,
        },
    };
    size_t wait_info_count = 1;

    if (renderer->device_lost_future_available) {
        wait_infos[wait_info_count++] = (WGPUFutureWaitInfo){
            .future = renderer->device_lost_future,
            .completed = false,
        };
    }

    const bool* completion_flags[] = {
        &map_ctx->completed,
        &renderer->device_lost.completed,
    };

    ReturnStatus wait_status = wait_for_gpu_futures(
        renderer->instance,
        &renderer->wait_config,
        wait_infos,
        wait_info_count,
        completion_flags,
        2,
        WGPU_READBACK_TIMEOUT_NS
    );

    wgpuInstanceProcessEvents(renderer->instance);

    if (renderer->device_lost.completed) {
        status = RETURN_DEVICE_LOST;
        Renderer_cancel_buffer_map(renderer);
        wgpuInstanceProcessEvents(renderer->instance);
        Renderer_retire_buffer_map(renderer);
        goto cleanup;
    }

    if (map_ctx->completed) {
        if (map_ctx->status == WGPUMapAsyncStatus_InstanceDropped) {
            status = RETURN_DEVICE_LOST;
            Renderer_retire_buffer_map(renderer);
            goto cleanup;
        }

        if (map_ctx->status != WGPUMapAsyncStatus_Success) {
            Renderer_retire_buffer_map(renderer);
            goto cleanup;
        }

        const void* mapped_data = wgpuBufferGetConstMappedRange(
            map_ctx->staging_buffer, 0, staging_size
        );
        if (mapped_data == NULL) {
            log_error("Failed to get mapped readback range\n");
            Renderer_retire_buffer_map(renderer);
            goto cleanup;
        }

        const u8* source = mapped_data;
        for (u32 row = 0; row < height; ++row) {
            size_t output_offset = (size_t)row * output_row_size;
            size_t staging_offset = (size_t)row * staging_row_size;
            memcpy(
                buffer + output_offset, source + staging_offset, output_row_size
            );
        }

        status = RETURN_SUCCESS;
        Renderer_retire_buffer_map(renderer);
        goto cleanup;
    }

    status = wait_status == RETURN_TIMEOUT ? RETURN_TIMEOUT : RETURN_FAILURE;

    Renderer_cancel_buffer_map(renderer);
    wgpuInstanceProcessEvents(renderer->instance);

    if (renderer->device_lost.completed) status = RETURN_DEVICE_LOST;
    Renderer_retire_buffer_map(renderer);

cleanup:
    if (command_buffer != NULL) wgpuCommandBufferRelease(command_buffer);
    if (encoder != NULL) wgpuCommandEncoderRelease(encoder);
    if (staging_buffer != NULL) wgpuBufferRelease(staging_buffer);
    return status;
}

/* WGPU callback functions */

void adapter_request_callback(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
) {
    (void)userdata2;

    WgpuAdapterRequestContext* ctx = (WgpuAdapterRequestContext*)userdata1;
    if (ctx == NULL) {
        if (adapter != NULL) wgpuAdapterRelease(adapter);
        return;
    }

    ctx->status = status;
    ctx->adapter = adapter;

    if (status == WGPURequestAdapterStatus_Success) {
        log_info("Adapter acquired successfully\n");
    } else if (message.length > 0) {
        log_error(
            "Failed to acquire adapter: %.*s\n",
            (int)message.length,
            message.data
        );
    } else {
        log_error("Failed to acquire adapter\n");
    }

    ctx->completed = true;
}

void device_request_callback(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
) {
    (void)userdata2;

    WgpuDeviceRequestContext* ctx = (WgpuDeviceRequestContext*)userdata1;
    if (ctx == NULL) {
        if (device != NULL) wgpuDeviceRelease(device);
        return;
    }

    ctx->status = status;
    ctx->device = device;

    if (status == WGPURequestDeviceStatus_Success) {
        log_info("Device acquired successfully\n");
    } else if (message.length > 0) {
        log_error(
            "Failed to acquire device: %.*s\n",
            (int)message.length,
            message.data
        );
    } else {
        log_error("Failed to acquire device\n");
    }

    ctx->completed = true;
}

void device_lost_callback(
    const WGPUDevice* device,
    WGPUDeviceLostReason reason,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
) {
    (void)device;
    (void)userdata2;

    WgpuDeviceLostContext* ctx = (WgpuDeviceLostContext*)userdata1;

    if (ctx == NULL) return;

    ctx->reason = reason;

    if (reason == WGPUDeviceLostReason_Destroyed) {
        log_debug("Device destroyed\n");
    } else if (message.length > 0) {
        log_error(
            "Device lost: reason=%u, message=%.*s\n",
            (uint32_t)reason,
            (int)message.length,
            message.data
        );
    } else {
        log_error("Device lost: reason=%u\n", (uint32_t)reason);
    }

    ctx->completed = true;
}

void buffer_map_callback(
    WGPUMapAsyncStatus status,
    WGPUStringView message,
    void* userdata1,
    void* userdata2
) {
    (void)userdata2;

    WgpuBufferMapContext* ctx = (WgpuBufferMapContext*)userdata1;

    if (ctx == NULL) return;

    ctx->status = status;

    bool expected_abort =
        ctx->unmap_requested && status == WGPUMapAsyncStatus_Aborted;
    if (status != WGPUMapAsyncStatus_Success && !expected_abort) {
        if (message.length > 0) {
            log_error(
                "Failed to map buffer: %.*s\n",
                (int)message.length,
                message.data
            );
        } else {
            log_error("Failed to map buffer\n");
        }
    }

    ctx->completed = true;
}

#endif /* RENDERER_H */
