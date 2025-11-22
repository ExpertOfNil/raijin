#ifndef MESH_H
#define MESH_H

#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "core.h"
#include "webgpu.h"
#include "cimpl_core.h"
#include "cimpl_glm.h"

#define DEFAULT_INSTANCE_CAPACITY 256

/* Types */

DEFINE_DYNAMIC_ARRAY(u16, IndexArray)

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

/* Function Prototypes */

void Instance_set_position(Instance* instance, vec3 position);
void Instance_from_position_rotation(
    Instance* instance, vec3 position, mat3 rotation, f32 scale, vec4 color
);
void Mesh_realloc_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
);
void Mesh_realloc_edge_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
);
void Mesh_create_cube(Mesh* mesh);

/* Static Definitions */

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

void Mesh_create_sphere_uv(Mesh* mesh, u32 divisions) {
	u32 longitude = 2 * divisions;
	u32 latitude = divisions;

	u32 n_vertices = 2 + (latitude - 1) * longitude;
	// 2 tris per quad
	u32 n_indices = 6 * longitude * (latitude - 1);
	u32 n_edge_indices = 2 * longitude * ((latitude - 1) + longitude + (latitude - 2));

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
		f32 phi = (f32)i * PI / (f32)latitude; // [0, π]
		f32 y = cosf(phi);
		f32 r = sinf(phi);

		for (u32 j = 0; j < longitude; ++j) {
			f32 theta = (f32)j * 2.0f * PI / (f32)longitude; // [0, 2π)
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
        IndexArray_push(&mesh->indices, 1 + (u16)top_index);
        IndexArray_push(&mesh->indices, 1 + (u16)next);
        IndexArray_push(&mesh->indices, 1 + (u16)i);
	}

	// Middle quads
	for (u32 i = 0; i < (latitude - 2); ++i) {
		u32 row = 1 + i * longitude;
		u32 next_row = row + longitude;

		for (u32 j = 0; j < longitude; ++j) {
			u32 next = (j + 1) % longitude;

			u16 a = (u16)(row + j);
			u16 b = (u16)(row + next);
			u16 c = (u16)(next_row + j);
			u16 d = (u16)(next_row + next);

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
		IndexArray_push(&mesh->indices, (u16)(base + j));
		IndexArray_push(&mesh->indices, (u16)(base + next));
		IndexArray_push(&mesh->indices, (u16)(bottom_index));
	}

	// === Edge Indices ===
	for (u32 j = 0; j < longitude; ++j) {
		// Top pole to first ring
		IndexArray_push(&mesh->edge_indices, (u16)(top_index));
		IndexArray_push(&mesh->edge_indices, (u16)(1 + j));

		// Connect rings vertically
		for (u32 i = 0; i < (latitude - 2); ++i) {
			u32 current_ring = 1 + i * longitude;
			u32 next_ring = current_ring + longitude;
			IndexArray_push(&mesh->edge_indices, (u16)(current_ring + j));
			IndexArray_push(&mesh->edge_indices, (u16)(next_ring + j));
		}

		// Last ring to bottom pole
		u32 last_ring = 1 + (latitude - 2) * longitude;
		IndexArray_push(&mesh->edge_indices, (u16)(last_ring + j));
		IndexArray_push(&mesh->edge_indices, (u16)(bottom_index));
	}

	// Latitude rings (horizontal circles)
	for (u32 i = 1; i < latitude; ++i) {
		u32 ring_start = 1 + (i - 1) * longitude;
		for (u32 j = 0; j < longitude; ++j) {
			u32 next = (j + 1) % longitude;
            IndexArray_push(&mesh->edge_indices, (u16)(ring_start + j));
            IndexArray_push(&mesh->edge_indices, (u16)(ring_start + next));
		}
	}
}

#endif /* MESH_H */
