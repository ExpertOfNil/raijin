#ifndef MESH_H
#define MESH_H

#include "cglm/cglm.h"
#include "cglm/vec3.h"
#include "cglm/mat4.h"
#include "core.h"
#include "webgpu.h"

#define DEFAULT_INSTANCE_CAPACITY 256

DEFINE_DYNAMIC_ARRAY(u16, IndexArray)

typedef struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
} Vertex;
DEFINE_DYNAMIC_ARRAY(Vertex, VertexArray)

WGPUVertexBufferLayout Vertex_desc(void) {
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

typedef struct Instance {
    mat4 model_matrix;
    vec4 color;
} Instance;
DEFINE_DYNAMIC_ARRAY(Instance, InstanceArray)

WGPUVertexBufferLayout Instance_desc(void) {
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

typedef enum {
    MESH_TYPE_TRIANGLE,
    MESH_TYPE_CUBE,
    MESH_TYPE_TETRAHEDRON,
    MESH_TYPE_SPHERE,
    MESH_TYPE_COUNT,
} MeshType;

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

void Mesh_realloc_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
) {
    while (mesh->instance_capacity < new_capacity) {
        if (mesh->instance_capacity == 0) {
            mesh->instance_capacity = DEFAULT_INSTANCE_CAPACITY * sizeof(Instance);
        } else {
            mesh->instance_capacity *= 2;
        }
        LOG_DEBUG("New instance capacity: %d", mesh->instance_capacity);
    }
    mesh->instance_buffer = create_buffer(
        device,
        mesh->instance_capacity * sizeof(Instance),
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Mesh Instance Buffer"
    );
}

void Mesh_realloc_edge_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
) {
    while (mesh->instance_capacity < new_capacity) {
        mesh->instance_capacity *= 2;
    }
    mesh->instance_buffer = create_buffer(
        device,
        mesh->instance_capacity * sizeof(Instance),
        WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
        "Mesh Edge Instance Buffer"
    );
}

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

static const u16 CUBE_INDICES[36] = {
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

static const u16 CUBE_EDGE_INDICES[24] = {
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

void Mesh_create_cube(Mesh* mesh) {
    static const u32 n_vertices = ARRAY_COUNT(CUBE_VERTICES);
    static const u32 n_indices = ARRAY_COUNT(CUBE_INDICES);
    static const u32 n_edge_indices = ARRAY_COUNT(CUBE_EDGE_INDICES);

    VertexArray_push_many(&mesh->vertices, CUBE_VERTICES, n_vertices);
    IndexArray_push_many(&mesh->indices, CUBE_INDICES, n_indices);
    IndexArray_push_many(&mesh->edge_indices, CUBE_EDGE_INDICES, n_edge_indices);
}

#endif /* MESH_H */
