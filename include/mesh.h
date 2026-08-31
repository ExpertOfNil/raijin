#ifndef MESH_H
#define MESH_H

#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "cimpl_core.h"
#include "core.h"
#include "webgpu.h"

#define DEFAULT_INSTANCE_CAPACITY 256

/* Types */

typedef u32 MeshHandle;
#define INVALID_MESH_HANDLE ((MeshHandle) - 1)

DEFINE_DYNAMIC_ARRAY(u32, IndexArray)

typedef struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
} Vertex;
DEFINE_DYNAMIC_ARRAY(Vertex, VertexArray)

typedef struct Instance {
    mat4 model_matrix;
    vec4 color;
} Instance;
DEFINE_DYNAMIC_ARRAY(Instance, InstanceArray)

typedef struct Mesh {
    VertexArray vertices;
    IndexArray indices;
    IndexArray edge_indices;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;
    WGPUBuffer instance_buffer;
    u32 instance_capacity;
    WGPUBuffer edge_index_buffer;
    WGPUBuffer edge_instance_buffer;
    u32 edge_instance_capacity;
} Mesh;
DEFINE_DYNAMIC_ARRAY(Mesh, MeshArray)

typedef u32 AssemblyHandle;
#define INVALID_ASSEMBLY_HANDLE ((AssemblyHandle) - 1)

typedef struct AssemblyComponent {
    MeshHandle mesh_handle;
    Instance instance;
} AssemblyComponent;
DEFINE_DYNAMIC_ARRAY(AssemblyComponent, AssemblyComponentArray)

typedef struct Assembly {
    AssemblyComponentArray components;
} Assembly;
DEFINE_DYNAMIC_ARRAY(Assembly, AssemblyArray)

/* Function Prototypes */

void Instance_set_position(Instance* instance, vec3 position);
void Instance_from_position_rotation(
    Instance* instance, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Instance_from_line(
    Instance* instance, vec3 start, vec3 end, f32 radius, vec4 color
);
ReturnStatus Mesh_realloc_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
);
ReturnStatus Mesh_realloc_edge_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
);
bool Mesh_cpu_arrays_are_empty(const Mesh* mesh);
// 2D Shapes
ReturnStatus Mesh_create_plane(Mesh* mesh);
ReturnStatus Mesh_create_disc(Mesh* mesh, u32 divisions);
// 3D Shapes
ReturnStatus Mesh_create_cube(Mesh* mesh);
ReturnStatus Mesh_create_sphere_uv(Mesh* mesh, u32 divisions);
ReturnStatus Mesh_create_cylinder(Mesh* mesh, u32 divisions);
ReturnStatus Mesh_create_cone(Mesh* mesh, u32 divisions);

void Mesh_release_gpu_buffers(Mesh* mesh);
void Mesh_release_cpu_arrays(Mesh* mesh);

/* Static Definitions */

static const u32 CUBE_INDICES[36] = {
    // clang-format off
        // Front
        0, 1, 3,
        0, 3, 2,
        // Back
        5, 4, 6,
        5, 6, 7,
        // Left
        1, 5, 7,
        1, 7, 3,
        // Right
        4, 0, 2,
        4, 2, 6,
        // Top
        4, 5, 1,
        4, 1, 0,
        // Bottom
        7, 6, 2,
        7, 2, 3,
    // clang-format on
};

static const u32 CUBE_EDGE_INDICES[24] = {
    // clang-format off
        0, 1,
        1, 3,
        3, 2,
        2, 0,

        4, 5,
        5, 7,
        7, 6,
        6, 4,

        0, 4,
        1, 5,
        2, 6,
        3, 7,
    // clang-format on
};

static const Vertex CUBE_VERTICES[8] = {
    {
        .position = {1.0, 1.0, 1.0},
        .color = {1.0, 1.0, 1.0},
        .normal = {0.577, 0.577, 0.577},
    },
    {
        .position = {-1.0, 1.0, 1.0},
        .color = {0.0, 0.0, 1.0},
        .normal = {-0.577, 0.577, 0.577},
    },
    {
        .position = {1.0, -1.0, 1.0},
        .color = {1.0, 0.0, 0.0},
        .normal = {0.577, -0.577, 0.577},
    },
    {
        .position = {-1.0, -1.0, 1.0},
        .color = {0.0, 0.0, 1.0},
        .normal = {-0.577, -0.577, 0.577},
    },
    {
        .position = {1.0, 1.0, -1.0},
        .color = {1.0, 0.0, 0.0},
        .normal = {0.577, 0.577, -0.577},
    },
    {
        .position = {-1.0, 1.0, -1.0},
        .color = {0.0, 0.0, 1.0},
        .normal = {-0.577, 0.577, -0.577},
    },
    {
        .position = {1.0, -1.0, -1.0},
        .color = {1.0, 0.0, 0.0},
        .normal = {0.577, -0.577, -0.577},
    },
    {
        .position = {-1.0, -1.0, -1.0},
        .color = {0.0, 0.0, 1.0},
        .normal = {-0.577, -0.577, -0.577},
    },
};

static inline WGPUVertexBufferLayout Vertex_desc(void) {
    static WGPUVertexAttribute attribs[3] = {
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = 0 * 3 * sizeof(f32),
            .shaderLocation = 0,
        },
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = 1 * 3 * sizeof(f32),
            .shaderLocation = 1,
        },
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = 2 * 3 * sizeof(f32),
            .shaderLocation = 2,
        },
    };

    return (WGPUVertexBufferLayout){
        .arrayStride = sizeof(Vertex),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 3,
        .attributes = attribs,
    };
}

static inline WGPUVertexBufferLayout Instance_desc(void) {
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
        .attributeCount = 5,
    };
}

#endif /* MESH_H */
