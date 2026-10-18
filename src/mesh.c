#include "mesh.h"

#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "cglm/vec3.h"
#include "cimpl_core.h"
#include "core.h"
#include "logging.h"

static ReturnStatus mesh_grow_instance_buffer(
    WGPUBuffer* buffer,
    u32* capacity,
    const WGPUDevice device,
    u32 required_capacity,
    const char* label
) {
    if (buffer == NULL || capacity == NULL || device == NULL) {
        return RETURN_FAILURE;
    }

    if (required_capacity <= *capacity) {
        return RETURN_SUCCESS;
    }

    u32 candidate_capacity = *capacity;
    if (candidate_capacity == 0) {
        candidate_capacity = DEFAULT_INSTANCE_CAPACITY;
    }

    while (candidate_capacity < required_capacity) {
        if (candidate_capacity > UINT32_MAX / 2) {
            candidate_capacity = required_capacity;
            break;
        }

        candidate_capacity *= 2;
    }

    if ((u64)candidate_capacity > UINT64_MAX / sizeof(Instance)) {
        log_error("Instance buffer size overflow\n");
        return RETURN_FAILURE;
    }

    u64 candidate_size = (u64)candidate_capacity * sizeof(Instance);
    WGPUBuffer candidate_buffer = create_buffer(
        device,
        candidate_size,
        WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        label
    );

    if (candidate_buffer == NULL) {
        return RETURN_FAILURE;
    }

    WGPUBuffer old_buffer = *buffer;

    *buffer = candidate_buffer;
    *capacity = candidate_capacity;

    if (old_buffer != NULL) {
        wgpuBufferRelease(old_buffer);
    }

    log_debug("New instance capacity: %u\n", candidate_capacity);
    return RETURN_SUCCESS;
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

MeshGeometryView MeshGeometry_view(const MeshGeometry* geometry) {
    if (geometry == NULL) return (MeshGeometryView){0};

    return (MeshGeometryView){
        .vertices = geometry->vertices.items,
        .vertex_count = geometry->vertices.count,
        .indices = geometry->indices.items,
        .index_count = geometry->indices.count,
        .edge_indices = geometry->edge_indices.items,
        .edge_index_count = geometry->edge_indices.count,
    };
}

bool MeshGeometry_is_empty(const MeshGeometry* geometry) {
    if (geometry == NULL) return false;

    return geometry->vertices.items == NULL && geometry->vertices.count == 0 &&
           geometry->vertices.capacity == 0 &&
           geometry->indices.items == NULL && geometry->indices.count == 0 &&
           geometry->indices.capacity == 0 &&
           geometry->edge_indices.items == NULL &&
           geometry->edge_indices.count == 0 &&
           geometry->edge_indices.capacity == 0;
}

void MeshGeometry_release(MeshGeometry* geometry) {
    if (geometry == NULL) return;

    VertexArray_free(&geometry->vertices);
    IndexArray_free(&geometry->indices);
    IndexArray_free(&geometry->edge_indices);
}

ReturnStatus MeshGeometryView_validate(const MeshGeometryView* geometry) {
    if (geometry == NULL) return RETURN_FAILURE;

    if (geometry->vertices == NULL || geometry->vertex_count == 0) {
        return RETURN_FAILURE;
    }

    if (geometry->indices == NULL || geometry->index_count == 0) {
        return RETURN_FAILURE;
    }

    // The solid pipeline uses triangle-list topology
    if (geometry->index_count % UINT32_C(3) != 0) {
        return RETURN_FAILURE;
    }

    /*
     * Edge geometry is optional, but nonempty input must consist of complete
     * line-list pairs
     */
    if (geometry->edge_index_count > 0 && geometry->edge_indices == NULL) {
        return RETURN_FAILURE;
    }

    if (geometry->edge_index_count % UINT32_C(2) != 0) {
        return RETURN_FAILURE;
    }

    // wgpuQueueWriteBuffer takes a size_t byte count
    if ((size_t)geometry->vertex_count >
            SIZE_MAX / sizeof(*geometry->vertices) ||
        (size_t)geometry->index_count > SIZE_MAX / sizeof(*geometry->indices) ||
        (size_t)geometry->edge_index_count >
            SIZE_MAX / sizeof(*geometry->edge_indices)) {
        return RETURN_FAILURE;
    }

    for (u32 i = 0; i < geometry->index_count; ++i) {
        if (geometry->indices[i] >= geometry->vertex_count) {
            return RETURN_FAILURE;
        }
    }

    for (u32 i = 0; i < geometry->edge_index_count; ++i) {
        if (geometry->edge_indices[i] >= geometry->vertex_count) {
            return RETURN_FAILURE;
        }
    }

    return RETURN_SUCCESS;
}

void Mesh_release_gpu_buffers(Mesh* mesh) {
    if (mesh == NULL) return;

    if (mesh->edge_index_buffer != NULL) {
        wgpuBufferRelease(mesh->edge_index_buffer);
        mesh->edge_index_buffer = NULL;
    }

    if (mesh->index_buffer != NULL) {
        wgpuBufferRelease(mesh->index_buffer);
        mesh->index_buffer = NULL;
    }

    if (mesh->instance_buffer != NULL) {
        wgpuBufferRelease(mesh->instance_buffer);
        mesh->instance_buffer = NULL;
    }

    if (mesh->vertex_buffer != NULL) {
        wgpuBufferRelease(mesh->vertex_buffer);
        mesh->vertex_buffer = NULL;
    }

    mesh->instance_capacity = 0;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    mesh->edge_index_count = 0;
}

ReturnStatus Mesh_realloc_instance_buffer(
    Mesh* mesh, const WGPUDevice device, u32 new_capacity
) {
    if (mesh == NULL) return RETURN_FAILURE;
    return (mesh_grow_instance_buffer(
        &mesh->instance_buffer,
        &mesh->instance_capacity,
        device,
        new_capacity,
        "Mesh Instance Buffer"
    ));
}

ReturnStatus MeshGeometry_create_cube(MeshGeometry* geometry) {
    if (!MeshGeometry_is_empty(geometry)) {
        return RETURN_FAILURE;
    }

    MeshGeometry candidate = {0};
    static const u32 n_vertices = ARRAY_COUNT(CUBE_VERTICES);
    static const u32 n_indices = ARRAY_COUNT(CUBE_INDICES);
    static const u32 n_edge_indices = ARRAY_COUNT(CUBE_EDGE_INDICES);

    if (VertexArray_push_many(&candidate.vertices, CUBE_VERTICES, n_vertices) !=
        RETURN_OK) {
        goto failure;
    }
    if (IndexArray_push_many(&candidate.indices, CUBE_INDICES, n_indices) !=
        RETURN_OK) {
        goto failure;
    }
    if (IndexArray_push_many(
            &candidate.edge_indices, CUBE_EDGE_INDICES, n_edge_indices
        ) != RETURN_OK) {
        goto failure;
    }

    geometry->vertices = candidate.vertices;
    geometry->indices = candidate.indices;
    geometry->edge_indices = candidate.edge_indices;
    return RETURN_SUCCESS;

failure:
    MeshGeometry_release(&candidate);
    return RETURN_FAILURE;
}

ReturnStatus MeshGeometry_create_plane(MeshGeometry* geometry) {
    if (!MeshGeometry_is_empty(geometry)) {
        return RETURN_FAILURE;
    }
    MeshGeometry candidate = {0};
    static const Vertex vertices[] = {
        {
            .position = {-0.5f, 0.0f, 0.5f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 1.0f, 0.0f},
        },
        {
            .position = {-0.5f, 0.0f, -0.5f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 1.0f, 0.0f},
        },
        {
            .position = {0.5f, 0.0f, -0.5f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 1.0f, 0.0f},
        },
        {
            .position = {0.5f, 0.0f, 0.5f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 1.0f, 0.0f},
        },
    };

    // clang-format off
    static const u32 indices[] = {
        0, 2, 1,
        0, 3, 2,
    };
    // clang-format on

    // clang-format off
    static const u32 edge_indices[] = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
    };
    // clang-format on

    if (VertexArray_push_many(
            &candidate.vertices, vertices, ARRAY_COUNT(vertices)
        ) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_push_many(
            &candidate.indices, indices, ARRAY_COUNT(indices)
        ) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_push_many(
            &candidate.edge_indices, edge_indices, ARRAY_COUNT(edge_indices)
        ) != RETURN_OK) {
        goto failure;
    }

    geometry->vertices = candidate.vertices;
    geometry->indices = candidate.indices;
    geometry->edge_indices = candidate.edge_indices;
    return RETURN_SUCCESS;

failure:
    MeshGeometry_release(&candidate);
    return RETURN_FAILURE;
}

ReturnStatus MeshGeometry_create_disc(MeshGeometry* geometry, u32 divisions) {
    if (!MeshGeometry_is_empty(geometry)) return RETURN_FAILURE;

    if (divisions < 3 || divisions > UINT32_MAX / 3) {
        log_error("Invalid disc division count: %u\n", divisions);
        return RETURN_FAILURE;
    }

    f32 theta = 2.0f * GLM_PI / divisions;
    u32 n_vertices = divisions + 1;
    u32 n_indices = divisions * 3;
    u32 n_edge_indices = divisions * 2;

    MeshGeometry candidate = {0};

    if (VertexArray_reserve(&candidate.vertices, n_vertices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.indices, n_indices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.edge_indices, n_edge_indices) !=
        RETURN_OK) {
        goto failure;
    }

    candidate.vertices.items[candidate.vertices.count++] = (Vertex){
        .position = {0.0f, 0.0f, 0.0f},
        .color = {1.0f, 1.0f, 1.0f},
        .normal = {0.0f, 1.0f, 0.0f},
    };

    for (u32 i = 0; i < divisions; ++i) {
        f32 angle = (f32)i * theta;
        f32 x = cosf(angle);
        f32 z = sinf(angle);

        candidate.vertices.items[candidate.vertices.count++] = (Vertex){
            .position = {x, 0.0f, z},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 1.0f, 0.0f},
        };
    }

    for (u32 i = 0; i < divisions; ++i) {
        u32 next = 1 + (i + 1) % divisions;
        u32 triangle[] = {0, next, 1 + i};
        if (IndexArray_push_many(
                &candidate.indices, triangle, ARRAY_COUNT(triangle)
            ) != RETURN_OK) {
            goto failure;
        }
    }

    for (u32 i = 0; i < divisions; ++i) {
        u32 next = 1 + (i + 1) % divisions;
        u32 edge[] = {1 + i, next};
        if (IndexArray_push_many(
                &candidate.edge_indices, edge, ARRAY_COUNT(edge)
            ) != RETURN_OK) {
            goto failure;
        }
    }

    geometry->vertices = candidate.vertices;
    geometry->indices = candidate.indices;
    geometry->edge_indices = candidate.edge_indices;
    return RETURN_SUCCESS;

failure:
    MeshGeometry_release(&candidate);
    return RETURN_FAILURE;
}

ReturnStatus MeshGeometry_create_sphere_uv(
    MeshGeometry* geometry, u32 divisions
) {
    if (!MeshGeometry_is_empty(geometry)) return RETURN_FAILURE;
    if (divisions < 3 || divisions > UINT32_MAX / 2) {
        log_error("Invalid sphere division count: %u\n", divisions);
        return RETURN_FAILURE;
    }

    u32 longitude = 2 * divisions;
    u32 latitude = divisions;

    uint64_t ring_vertex_count = (uint64_t)(latitude - 1) * (uint64_t)longitude;
    if (ring_vertex_count > UINT32_MAX / 6) {
        log_error("Sphere division count is too large: %u\n", divisions);
        return RETURN_FAILURE;
    }

    uint64_t edge_segment_count =
        (uint64_t)longitude * (2 * (uint64_t)latitude - 1);
    if (edge_segment_count > UINT32_MAX / 2) {
        log_error("Sphere edge count is too large: %u\n", divisions);
        return RETURN_FAILURE;
    }

    uint32_t n_vertices = (uint32_t)(2 + ring_vertex_count);
    // 2 tris per quad
    uint32_t n_indices = (uint32_t)(6 * ring_vertex_count);
    uint32_t n_edge_indices = (uint32_t)(2 * edge_segment_count);

    MeshGeometry candidate = {0};

    if (VertexArray_reserve(&candidate.vertices, n_vertices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.indices, n_indices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.edge_indices, n_edge_indices) !=
        RETURN_OK) {
        goto failure;
    }

    // Top pole
    uint32_t top_index = candidate.vertices.count;
    candidate.vertices.items[candidate.vertices.count++] = (Vertex){
        .position = {0.0f, 1.0f, 0.0f},
        .color = {1.0f, 1.0f, 1.0f},
        .normal = {0.0f, 1.0f, 0.0f},
    };

    // Rings (excluding poles)
    for (u32 i = 1; i < latitude; ++i) {
        f32 phi = (f32)i * GLM_PI / (f32)latitude;  // [0, pi]
        f32 y = cosf(phi);
        f32 r = sinf(phi);

        for (u32 j = 0; j < longitude; ++j) {
            f32 theta = (f32)j * 2.0f * GLM_PI / (f32)longitude;  // [0, 2*pi)
            f32 x = r * cosf(theta);
            f32 z = r * sinf(theta);

            candidate.vertices.items[candidate.vertices.count++] = (Vertex){
                .position = {x, y, z},
                .color = {1.0f, 1.0f, 1.0f},
                .normal = {x, y, z},
            };
        }
    }

    // Bottom pole
    uint32_t bottom_index = candidate.vertices.count;
    candidate.vertices.items[candidate.vertices.count++] = (Vertex){
        .position = {0.0f, -1.0f, 0.0f},
        .color = {1.0f, 1.0f, 1.0f},
        .normal = {0.0f, -1.0f, 0.0f},
    };

    // === Indices ===

    // Top cap
    for (u32 i = 0; i < longitude; ++i) {
        u32 next = (i + 1) % longitude;
        uint32_t triangle[] = {
            top_index,
            1 + next,
            1 + i,
        };

        if (IndexArray_push_many(
                &candidate.indices, triangle, ARRAY_COUNT(triangle)
            ) != RETURN_OK) {
            goto failure;
        }
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

            uint32_t triangles[] = {a, b, c, b, d, c};
            if (IndexArray_push_many(
                    &candidate.indices, triangles, ARRAY_COUNT(triangles)
                ) != RETURN_OK) {
                goto failure;
            }
        }
    }

    // Bottom cap
    u32 last_ring = 1 + (latitude - 2) * longitude;
    for (u32 j = 0; j < longitude; ++j) {
        u32 next = (j + 1) % longitude;
        uint32_t triangle[] = {
            last_ring + j,
            last_ring + next,
            bottom_index,
        };
        if (IndexArray_push_many(
                &candidate.indices, triangle, ARRAY_COUNT(triangle)
            ) != RETURN_OK) {
            goto failure;
        }
    }

    // === Edge Indices ===

    // Vertical edges
    for (u32 j = 0; j < longitude; ++j) {
        // Top pole to first ring
        uint32_t top_edge[] = {
            top_index,
            1 + j,
        };
        if (IndexArray_push_many(
                &candidate.edge_indices, top_edge, ARRAY_COUNT(top_edge)
            ) != RETURN_OK) {
            goto failure;
        }

        // Connect rings vertically
        for (u32 i = 0; i < (latitude - 2); ++i) {
            u32 current_ring = 1 + i * longitude;
            u32 next_ring = current_ring + longitude;
            uint32_t edge[] = {
                current_ring + j,
                next_ring + j,
            };

            if (IndexArray_push_many(
                    &candidate.edge_indices, edge, ARRAY_COUNT(edge)
                ) != RETURN_OK) {
                goto failure;
            }
        }

        // Last ring to bottom pole
        uint32_t bottom_edge[] = {
            last_ring + j,
            bottom_index,
        };
        if (IndexArray_push_many(
                &candidate.edge_indices, bottom_edge, ARRAY_COUNT(bottom_edge)
            ) != RETURN_OK) {
            goto failure;
        }
    }

    // Latitude rings (horizontal)
    for (u32 i = 1; i < latitude; ++i) {
        u32 ring_start = 1 + (i - 1) * longitude;
        for (u32 j = 0; j < longitude; ++j) {
            u32 next = (j + 1) % longitude;
            uint32_t edge[] = {
                ring_start + j,
                ring_start + next,
            };
            if (IndexArray_push_many(
                    &candidate.edge_indices, edge, ARRAY_COUNT(edge)
                ) != RETURN_OK) {
                goto failure;
            }
        }
    }

    geometry->vertices = candidate.vertices;
    geometry->indices = candidate.indices;
    geometry->edge_indices = candidate.edge_indices;
    return RETURN_SUCCESS;

failure:
    MeshGeometry_release(&candidate);
    return RETURN_FAILURE;
}

ReturnStatus MeshGeometry_create_cylinder(
    MeshGeometry* geometry, u32 divisions
) {
    if (!MeshGeometry_is_empty(geometry)) return RETURN_FAILURE;
    if (divisions < 4 || divisions > UINT32_MAX / 12) {
        log_error("Invalid cylinder division count: %u\n", divisions);
        return RETURN_FAILURE;
    }

    const f32 radius = 1.0f;
    const f32 height = 1.0f;
    const f32 theta = 2.0f * GLM_PI / divisions;
    const uint32_t vertical_step = divisions / 4;
    const uint32_t vertical_edge_count =
        (uint32_t)(((uint64_t)divisions + vertical_step - 1) / vertical_step);

    // vertices: divisions * 2 (sides) + 2 (cap centers)
    u32 n_vertices = divisions * 2 + 2;
    // indices: divisions * 6 (sides) + divisions * 3 * 2 (two caps)
    u32 n_indices = divisions * 12;
    u32 n_edge_indices = divisions * 4 + vertical_edge_count * 2;

    MeshGeometry candidate = {0};
    if (VertexArray_reserve(&candidate.vertices, n_vertices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.indices, n_indices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.edge_indices, n_edge_indices) !=
        RETURN_OK) {
        goto failure;
    }

    // Generate side vertices
    for (u32 i = 0; i < divisions; ++i) {
        f32 angle = (f32)i * theta;
        f32 x = radius * cosf(angle);
        f32 z = radius * sinf(angle);
        // Bottom vertex (at y=0)
        candidate.vertices.items[candidate.vertices.count++] = (Vertex){
            .position = {x, 0.0f, z},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {x, 0.0f, z},
        };
        // Top vertex (at y=height)
        candidate.vertices.items[candidate.vertices.count++] = (Vertex){
            .position = {x, height, z},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {x, 0.0f, z},
        };
    }

    // Add center vertices for caps
    u32 bottom_center = candidate.vertices.count;
    candidate.vertices.items[candidate.vertices.count++] = (Vertex){
        .position = {0.0f, 0.0f, 0.0f},
        .color = {1.0f, 1.0f, 1.0f},
        .normal = {0.0f, -1.0f, 0.0f},
    };
    u32 top_center = candidate.vertices.count;
    candidate.vertices.items[candidate.vertices.count++] = (Vertex){
        .position = {0.0f, height, 0.0f},
        .color = {1.0f, 1.0f, 1.0f},
        .normal = {0.0f, 1.0f, 0.0f},
    };

    // Generate indices for side faces
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 bottom_current = i * 2;
        u32 top_current = i * 2 + 1;
        u32 bottom_next = next * 2;
        u32 top_next = next * 2 + 1;
        uint32_t triangles[] = {
            bottom_current,
            top_current,
            bottom_next,
            bottom_next,
            top_current,
            top_next
        };
        if (IndexArray_push_many(
                &candidate.indices, triangles, ARRAY_COUNT(triangles)
            ) != RETURN_OK) {
            goto failure;
        }
    }

    // Generate indices for bottom cap (winding order matters for culling)
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 bottom_current = i * 2;
        u32 bottom_next = next * 2;
        u32 triangle[] = {
            bottom_center,
            bottom_next,
            bottom_current,
        };

        if (IndexArray_push_many(
                &candidate.indices, triangle, ARRAY_COUNT(triangle)
            ) != RETURN_OK) {
            goto failure;
        }
    }

    // Generate indices for top cap
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 top_current = i * 2 + 1;
        u32 top_next = next * 2 + 1;
        uint32_t triangle[] = {
            top_center,
            top_current,
            top_next,
        };

        if (IndexArray_push_many(
                &candidate.indices, triangle, ARRAY_COUNT(triangle)
            ) != RETURN_OK) {
            goto failure;
        }
    }

    // Generate edge indices
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        u32 bottom_current = i * 2;
        u32 top_current = i * 2 + 1;
        u32 bottom_next = next * 2;
        u32 top_next = next * 2 + 1;
        uint32_t ring_edges[] = {
            bottom_current,
            bottom_next,
            top_current,
            top_next,
        };

        if (IndexArray_push_many(
                &candidate.edge_indices, ring_edges, ARRAY_COUNT(ring_edges)
            ) != RETURN_OK) {
            goto failure;
        }

        // ADD THIS: Vertical lines (only every few divisions to avoid clutter)
        if (i % vertical_step == 0) {
            uint32_t vertical_edge[] = {
                bottom_current,
                top_current,
            };
            if (IndexArray_push_many(
                    &candidate.edge_indices,
                    vertical_edge,
                    ARRAY_COUNT(vertical_edge)
                ) != RETURN_OK) {
                goto failure;
            }
        }
    }

    geometry->vertices = candidate.vertices;
    geometry->indices = candidate.indices;
    geometry->edge_indices = candidate.edge_indices;
    return RETURN_SUCCESS;

failure:
    MeshGeometry_release(&candidate);
    return RETURN_FAILURE;
}

ReturnStatus MeshGeometry_create_cone(MeshGeometry* geometry, u32 divisions) {
    if (!MeshGeometry_is_empty(geometry)) return RETURN_FAILURE;
    if (divisions < 4 || divisions > UINT32_MAX / 6) {
        log_error("Invalid cone division count: %u\n", divisions);
        return RETURN_FAILURE;
    }

    const float radius = 1.0f;
    const float height = 1.0f;
    const float theta = 2.0f * GLM_PI / divisions;

    const uint32_t vertical_step = divisions / 4;
    const uint32_t vertical_edge_count =
        (uint32_t)((uint64_t)divisions + vertical_step - 1) / vertical_step;

    // vertices: divisions (base ring) + 1 (tip) + 1 (base center)
    u32 n_vertices = divisions + 2;
    // indices: divisions * 3 (side triangles) + divisions * 3 (base cap)
    u32 n_indices = divisions * 6;
    // base ring + edges to tip
    u32 n_edge_indices = divisions * 2 + vertical_edge_count * 2;

    MeshGeometry candidate = {0};

    if (VertexArray_reserve(&candidate.vertices, n_vertices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.indices, n_indices) != RETURN_OK) {
        goto failure;
    }
    if (IndexArray_reserve(&candidate.edge_indices, n_edge_indices) !=
        RETURN_OK) {
        goto failure;
    }

    // Generate base ring vertices
    for (u32 i = 0; i < divisions; ++i) {
        f32 angle = (f32)i * theta;
        f32 x = radius * cosf(angle);
        f32 z = radius * sinf(angle);
        // Normal for cone side - points outward and up
        // Slant height calculation for proper normal
        f32 slant = sqrtf(radius * radius + height * height);
        candidate.vertices.items[candidate.vertices.count++] = (Vertex){
            .position = {x, 0.0f, z},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {x * height / slant, radius / slant, z * height / slant},
        };
    }

    // Add tip vertex
    // Tip normal - average of all side normals (points up and out)
    u32 tip_index = candidate.vertices.count;
    candidate.vertices.items[candidate.vertices.count++] = (Vertex){
        .position = {0.0f, height, 0.0f},
        .color = {1.0f, 1.0f, 1.0f},
        .normal = {0.0f, 1.0f, 0.0f},
    };
    // Add base center vertex
    u32 base_center = candidate.vertices.count;
    candidate.vertices.items[candidate.vertices.count++] = (Vertex){
        .position = {0.0f, 0.0f, 0.0f},
        .color = {1.0f, 1.0f, 1.0f},
        .normal = {0.0f, -1.0f, 0.0f},
    };

    // Generate indices for side triangles
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        uint32_t triangle[] = {
            i,
            tip_index,
            next,
        };

        if (IndexArray_push_many(
                &candidate.indices, triangle, ARRAY_COUNT(triangle)
            ) != RETURN_OK) {
            goto failure;
        }
    }
    // Generate indices for base cap
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        uint32_t triangle[] = {
            base_center,
            next,
            i,
        };

        if (IndexArray_push_many(
                &candidate.indices, triangle, ARRAY_COUNT(triangle)
            ) != RETURN_OK) {
            goto failure;
        }
    }
    // Generate edge indices
    for (u32 i = 0; i < divisions; ++i) {
        u32 next = (i + 1) % divisions;
        uint32_t edge[] = {i, next};

        // Base ring
        if (IndexArray_push_many(
                &candidate.edge_indices, edge, ARRAY_COUNT(edge)
            ) != RETURN_OK) {
            goto failure;
        }

        // Edge to tip (only every few divisions to avoid clutter)
        if (i % (divisions / 4) == 0) {
            uint32_t tip_edge[] = {i, tip_index};

            if (IndexArray_push_many(
                    &candidate.edge_indices, tip_edge, ARRAY_COUNT(tip_edge)
                ) != RETURN_OK) {
                goto failure;
            }
        }
    }

    geometry->vertices = candidate.vertices;
    geometry->indices = candidate.indices;
    geometry->edge_indices = candidate.edge_indices;
    return RETURN_SUCCESS;

failure:
    MeshGeometry_release(&candidate);
    return RETURN_FAILURE;
}
