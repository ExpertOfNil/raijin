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
void Mesh_realloc_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
);
void Mesh_realloc_edge_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
);
void Mesh_create_cube(Mesh* mesh);
void Mesh_create_sphere_uv(Mesh* mesh, u32 divisions);
void Mesh_create_disc(Mesh* mesh, u32 divisions);
void Mesh_create_cylinder(Mesh* mesh, u32 divisions);
void Mesh_create_cone(Mesh* mesh, u32 divisions);

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

/* Functions */

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

// Helper to create instance transform for a cylinder between two points
void Instance_from_line(
    Instance* instance, vec3 start, vec3 end, f32 radius, vec4 color
) {
    vec3 axis;
    glm_vec3_sub(end, start, axis);
    f32 length = glm_vec3_norm(axis);
    glm_vec3_normalize(axis);

    // Create quaternion that rotates Y-axis to align with our axis
    vec3 up = {0.0f, 1.0f, 0.0f};
    vec4 quat;
    glm_quat_from_vecs(up, axis, quat);

    // Convert quaternion to rotation matrix
    mat4 rotation;
    glm_quat_mat4(quat, rotation);

    // Manually build the matrix: scale the rotation matrix, then set
    // translation
    glm_vec4_scale(rotation[0], radius, instance->model_matrix[0]);
    glm_vec4_scale(rotation[1], length, instance->model_matrix[1]);
    glm_vec4_scale(rotation[2], radius, instance->model_matrix[2]);
    // Set translation directly in 4th column
    instance->model_matrix[3][0] = start[0];
    instance->model_matrix[3][1] = start[1];
    instance->model_matrix[3][2] = start[2];
    instance->model_matrix[3][3] = 1.0f;

    glm_vec4_copy(color, instance->color);
}

void Mesh_realloc_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
) {
    while (mesh->instance_capacity < new_capacity) {
        if (mesh->instance_capacity == 0) {
            mesh->instance_capacity = DEFAULT_INSTANCE_CAPACITY;
        } else {
            mesh->instance_capacity *= 2;
        }
        log_debug("New instance capacity: %d", mesh->instance_capacity);
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
    while (mesh->edge_instance_capacity < new_capacity) {
        if (mesh->edge_instance_capacity == 0) {
            mesh->edge_instance_capacity = DEFAULT_INSTANCE_CAPACITY;
        } else {
            mesh->edge_instance_capacity *= 2;
        }
        log_debug("New edge instance capacity: %d", mesh->instance_capacity);
    }
    mesh->edge_instance_buffer = create_buffer(
        device,
        mesh->edge_instance_capacity * sizeof(Instance),
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        "Mesh Edge Instance Buffer"
    );
}

void Mesh_create_cube(Mesh* mesh) {
    static const u32 n_vertices = ARRAY_COUNT(CUBE_VERTICES);
    static const u32 n_indices = ARRAY_COUNT(CUBE_INDICES);
    static const u32 n_edge_indices = ARRAY_COUNT(CUBE_EDGE_INDICES);

    VertexArray_push_many(&mesh->vertices, CUBE_VERTICES, n_vertices);
    IndexArray_push_many(&mesh->indices, CUBE_INDICES, n_indices);
    IndexArray_push_many(
        &mesh->edge_indices, CUBE_EDGE_INDICES, n_edge_indices
    );
}

void Mesh_create_disc(Mesh* mesh, u32 divisions) {
    f32 theta = 2.0f * GLM_PI / divisions;
    u32 n_vertices = divisions + 1;
    u32 n_indices = divisions * 3;
    u32 n_edge_indices = divisions * 2;

    VertexArray_reserve(&mesh->vertices, n_vertices);
    IndexArray_reserve(&mesh->indices, n_indices);
    IndexArray_reserve(&mesh->edge_indices, n_edge_indices);

    Vertex* vertex = &mesh->vertices.items[0];
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, vertex->position);
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, vertex->normal);
    mesh->vertices.count = 1;

    for (u32 i = 0; i < divisions; ++i) {
        f32 angle = (f32)i * theta;
        f32 x = cosf(angle);
        f32 z = sinf(angle);

        vertex = &mesh->vertices.items[mesh->vertices.count];
        glm_vec3_copy((vec3){x, 0.0f, z}, vertex->position);
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, vertex->normal);
        mesh->vertices.count++;
    }

    for (u32 i = 0; i < divisions; ++i) {
        u32 next = 1 + (i + 1) % divisions;
        IndexArray_push(&mesh->indices, 0);
        IndexArray_push(&mesh->indices, next);
        IndexArray_push(&mesh->indices, 1 + i);
    }

    for (u32 i = 0; i < divisions; ++i) {
        u32 next = 1 + (i + 1) % divisions;
        IndexArray_push(&mesh->edge_indices, 1 + i);
        IndexArray_push(&mesh->edge_indices, next);
    }
}

void Mesh_create_sphere_uv(Mesh* mesh, u32 divisions) {
    u32 longitude = 2 * divisions;
    u32 latitude = divisions;

    u32 n_vertices = 2 + (latitude - 1) * longitude;
    // 2 tris per quad
    u32 n_indices = 6 * longitude * (latitude - 1);
    u32 n_edge_indices =
        2 * longitude * ((latitude - 1) + longitude + (latitude - 2));

    VertexArray_reserve(&mesh->vertices, n_vertices);
    IndexArray_reserve(&mesh->indices, n_indices);
    IndexArray_reserve(&mesh->edge_indices, n_edge_indices);

    u32* idx = &mesh->vertices.count;
    *idx = 0;
    Vertex* vertex = &mesh->vertices.items[*idx];

    // Top pole
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, vertex->position);
    glm_vec3_normalize_to(vertex->position, vertex->normal);
    u32 top_index = 0;
    *idx += 1;

    // Rings (excluding poles)
    for (u32 i = 1; i < latitude; ++i) {
        f32 phi = (f32)i * GLM_PI / (f32)latitude;  // [0, π]
        f32 y = cosf(phi);
        f32 r = sinf(phi);

        for (u32 j = 0; j < longitude; ++j) {
            f32 theta = (f32)j * 2.0f * GLM_PI / (f32)longitude;  // [0, 2π)
            f32 x = r * cos(theta);
            f32 z = r * sin(theta);

            vertex = &mesh->vertices.items[*idx];
            glm_vec3_copy((vec3){x, y, z}, vertex->position);
            glm_vec3_normalize_to(vertex->position, vertex->normal);
            *idx += 1;
        }
    }

    // Bottom pole
    vertex = &mesh->vertices.items[*idx];
    glm_vec3_copy((vec3){0, -1, 0}, vertex->position);
    glm_vec3_normalize_to(vertex->position, vertex->normal);
    u32 bottom_index = *idx;
    *idx += 1;

    // === Indices ===

    // Top cap
    for (u32 i = 0; i < longitude; ++i) {
        u32 next = (i + 1) % longitude;
        IndexArray_push(&mesh->indices, 1 + top_index);
        IndexArray_push(&mesh->indices, 1 + next);
        IndexArray_push(&mesh->indices, 1 + i);
    }

    // Middle quads
    for (u32 i = 0; i < (latitude - 2); ++i) {
        u32 row = 1 + i * longitude;
        u32 next_row = row + longitude;

        for (u32 j = 0; j < longitude; ++j) {
            u32 next = (j + 1) % longitude;

            u32 a = row + j;
            u32 b = row + next;
            u32 c = next_row + j;
            u32 d = next_row + next;

            IndexArray_push(&mesh->indices, a);
            IndexArray_push(&mesh->indices, b);
            IndexArray_push(&mesh->indices, c);
            IndexArray_push(&mesh->indices, b);
            IndexArray_push(&mesh->indices, d);
            IndexArray_push(&mesh->indices, c);
        }
    }

    // Bottom cap
    u32 base = 1 + (latitude - 2) * longitude;
    for (u32 j = 0; j < longitude; ++j) {
        u32 next = (j + 1) % longitude;
        IndexArray_push(&mesh->indices, base + j);
        IndexArray_push(&mesh->indices, base + next);
        IndexArray_push(&mesh->indices, bottom_index);
    }

    // === Edge Indices ===
    for (u32 j = 0; j < longitude; ++j) {
        // Top pole to first ring
        IndexArray_push(&mesh->edge_indices, top_index);
        IndexArray_push(&mesh->edge_indices, 1 + j);

        // Connect rings vertically
        for (u32 i = 0; i < (latitude - 2); ++i) {
            u32 current_ring = 1 + i * longitude;
            u32 next_ring = current_ring + longitude;
            IndexArray_push(&mesh->edge_indices, current_ring + j);
            IndexArray_push(&mesh->edge_indices, next_ring + j);
        }

        // Last ring to bottom pole
        u32 last_ring = 1 + (latitude - 2) * longitude;
        IndexArray_push(&mesh->edge_indices, last_ring + j);
        IndexArray_push(&mesh->edge_indices, bottom_index);
    }

    // Latitude rings (horizontal circles)
    for (u32 i = 1; i < latitude; ++i) {
        u32 ring_start = 1 + (i - 1) * longitude;
        for (u32 j = 0; j < longitude; ++j) {
            u32 next = (j + 1) % longitude;
            IndexArray_push(&mesh->edge_indices, ring_start + j);
            IndexArray_push(&mesh->edge_indices, ring_start + next);
        }
    }
}

void Mesh_create_cylinder(Mesh* mesh, u32 divisions) {
    f32 radius = 1.0f;
    f32 height = 1.0f;
    // vertices: divisions * 2 (sides) + 2 (cap centers)
    u32 n_vertices = divisions * 2 + 2;
    // indices: divisions * 6 (sides) + divisions * 3 * 2 (two caps)
    u32 n_indices = divisions * 6 + divisions * 6;
    u32 n_edge_indices = divisions * 4 + (divisions / 4) * 2;
    VertexArray_reserve(&mesh->vertices, n_vertices);
    IndexArray_reserve(&mesh->indices, n_indices);
    IndexArray_reserve(&mesh->edge_indices, n_edge_indices);
    f32 theta = 2.0f * GLM_PI / divisions;
    // Generate side vertices
    for (u32 i = 0; i < divisions; ++i) {
        f32 angle = (f32)i * theta;
        f32 x = radius * cosf(angle);
        f32 z = radius * sinf(angle);
        // Bottom vertex (at y=0)
        Vertex* bottom_vertex = &mesh->vertices.items[mesh->vertices.count++];
        glm_vec3_copy((vec3){x, 0.0f, z}, bottom_vertex->position);
        vec3 normal = {x, 0.0f, z};
        glm_vec3_normalize(normal);
        glm_vec3_copy(normal, bottom_vertex->normal);
        // Top vertex (at y=height)
        Vertex* top_vertex = &mesh->vertices.items[mesh->vertices.count++];
        glm_vec3_copy((vec3){x, height, z}, top_vertex->position);
        glm_vec3_copy(normal, top_vertex->normal);
    }
    // Add center vertices for caps
    u32 bottom_center = mesh->vertices.count;
    Vertex* bottom_center_vertex =
        &mesh->vertices.items[mesh->vertices.count++];
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, bottom_center_vertex->position);
    glm_vec3_copy((vec3){0.0f, -1.0f, 0.0f}, bottom_center_vertex->normal);
    u32 top_center = mesh->vertices.count;
    Vertex* top_center_vertex = &mesh->vertices.items[mesh->vertices.count++];
    glm_vec3_copy((vec3){0.0f, height, 0.0f}, top_center_vertex->position);
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, top_center_vertex->normal);
    // Generate indices for side faces
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 bottom_current = i * 2;
        u32 top_current = i * 2 + 1;
        u32 bottom_next = next * 2;
        u32 top_next = next * 2 + 1;
        // First triangle
        IndexArray_push(&mesh->indices, bottom_current);
        IndexArray_push(&mesh->indices, top_current);
        IndexArray_push(&mesh->indices, bottom_next);
        // Second triangle
        IndexArray_push(&mesh->indices, bottom_next);
        IndexArray_push(&mesh->indices, top_current);
        IndexArray_push(&mesh->indices, top_next);
    }
    // Generate indices for bottom cap (winding order matters for culling)
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 bottom_current = i * 2;
        u32 bottom_next = next * 2;

        IndexArray_push(&mesh->indices, bottom_center);
        IndexArray_push(&mesh->indices, bottom_next);
        IndexArray_push(&mesh->indices, bottom_current);
    }
    // Generate indices for top cap
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 top_current = i * 2 + 1;
        u32 top_next = next * 2 + 1;

        IndexArray_push(&mesh->indices, top_center);
        IndexArray_push(&mesh->indices, top_current);
        IndexArray_push(&mesh->indices, top_next);
    }

    // Generate edge indices

    // This variant does not include lengthwise lines
    /*
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 bottom_current = i * 2;
        u32 top_current = i * 2 + 1;
        u32 bottom_next = next * 2;
        u32 top_next = next * 2 + 1;
        // Bottom ring
        IndexArray_push(&mesh->edge_indices, bottom_current);
        IndexArray_push(&mesh->edge_indices, bottom_next);
        // Top ring
        IndexArray_push(&mesh->edge_indices, top_current);
        IndexArray_push(&mesh->edge_indices, top_next);
    }
    */

    // This variant includes lengthwise lines
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 bottom_current = i * 2;
        u32 top_current = i * 2 + 1;
        u32 bottom_next = next * 2;
        u32 top_next = next * 2 + 1;
        // Bottom ring
        IndexArray_push(&mesh->edge_indices, bottom_current);
        IndexArray_push(&mesh->edge_indices, bottom_next);
        // Top ring
        IndexArray_push(&mesh->edge_indices, top_current);
        IndexArray_push(&mesh->edge_indices, top_next);

        // ADD THIS: Vertical lines (only every few divisions to avoid clutter)
        if (i % (divisions / 4) == 0) {
            IndexArray_push(&mesh->edge_indices, bottom_current);
            IndexArray_push(&mesh->edge_indices, top_current);
        }
    }
}

void Mesh_create_cone(Mesh* mesh, u32 divisions) {
    f32 radius = 1.0f;
    f32 height = 1.0f;
    // vertices: divisions (base ring) + 1 (tip) + 1 (base center)
    u32 n_vertices = divisions + 2;
    // indices: divisions * 3 (side triangles) + divisions * 3 (base cap)
    u32 n_indices = divisions * 6;
    u32 n_edge_indices = divisions * 3;  // base ring + edges to tip
    VertexArray_reserve(&mesh->vertices, n_vertices);
    IndexArray_reserve(&mesh->indices, n_indices);
    IndexArray_reserve(&mesh->edge_indices, n_edge_indices);
    f32 theta = 2.0f * GLM_PI / divisions;
    // Generate base ring vertices
    for (u32 i = 0; i < divisions; ++i) {
        f32 angle = (f32)i * theta;
        f32 x = radius * cosf(angle);
        f32 z = radius * sinf(angle);
        Vertex* base_vertex = &mesh->vertices.items[mesh->vertices.count++];
        glm_vec3_copy((vec3){x, 0.0f, z}, base_vertex->position);

        // Normal for cone side - points outward and up
        // Slant height calculation for proper normal
        f32 slant = sqrtf(radius * radius + height * height);
        vec3 normal = {x * height / slant, radius / slant, z * height / slant};
        glm_vec3_normalize(normal);
        glm_vec3_copy(normal, base_vertex->normal);
    }
    // Add tip vertex
    u32 tip_index = mesh->vertices.count;
    Vertex* tip_vertex = &mesh->vertices.items[mesh->vertices.count++];
    glm_vec3_copy((vec3){0.0f, height, 0.0f}, tip_vertex->position);
    // Tip normal - average of all side normals (points up and out)
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, tip_vertex->normal);
    // Add base center vertex
    u32 base_center = mesh->vertices.count;
    Vertex* base_center_vertex = &mesh->vertices.items[mesh->vertices.count++];
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, base_center_vertex->position);
    glm_vec3_copy((vec3){0.0f, -1.0f, 0.0f}, base_center_vertex->normal);
    // Generate indices for side triangles
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;

        IndexArray_push(&mesh->indices, i);
        IndexArray_push(&mesh->indices, tip_index);
        IndexArray_push(&mesh->indices, next);
    }
    // Generate indices for base cap
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;

        IndexArray_push(&mesh->indices, base_center);
        IndexArray_push(&mesh->indices, next);
        IndexArray_push(&mesh->indices, i);
    }
    // Generate edge indices
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;

        // Base ring
        IndexArray_push(&mesh->edge_indices, i);
        IndexArray_push(&mesh->edge_indices, next);

        // Edge to tip (only every few divisions to avoid clutter)
        if (i % (divisions / 4) == 0) {
            IndexArray_push(&mesh->edge_indices, i);
            IndexArray_push(&mesh->edge_indices, tip_index);
        }
    }
}

#endif /* MESH_H */
