#ifndef MESH_H
#define MESH_H

#include "cglm/vec3.h"
#include "core.h"
#include "webgpu.h"

typedef struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
} Vertex;

static WGPUVertexBufferLayout Vertex_desc(void) {
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

typedef enum {
    MESH_TYPE_TRIANGLE,
    MESH_TYPE_CUBE,
    MESH_TYPE_TETRAHEDRON,
    MESH_TYPE_SPHERE,
    MESH_TYPE_COUNT,
} MeshType;

DEFINE_DYNAMIC_ARRAY(Vertex, VertexArray)
DEFINE_DYNAMIC_ARRAY(u16, IndexArray)

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

void Mesh_realloc_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
) {
    while (mesh->instance_capacity < new_capacity) {
        mesh->instance_capacity *= 2;
    }
    mesh->instance_buffer = create_buffer(
        device,
        &mesh->indices.items,
        sizeof(mesh->indices.items),
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
        &mesh->indices.items,
        sizeof(mesh->indices.items),
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
