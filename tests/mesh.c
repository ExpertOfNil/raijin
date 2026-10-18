#include "mesh.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CHECK(expression)        \
    do {                         \
        if (!(expression)) {     \
            return EXIT_FAILURE; \
        }                        \
    } while (0)

static bool indices_are_valid(const IndexArray* indices, uint32_t vertex_count) {
    for (uint32_t i = 0; i < indices->count; ++i) {
        if (indices->items[i] >= vertex_count) {
            return false;
        }
    }

    return true;
}

static bool vertices_are_initialized(const VertexArray* vertices) {
    for (uint32_t i = 0; i < vertices->count; ++i) {
        const Vertex* vertex = &vertices->items[i];

        for (uint32_t component = 0; component < 3; ++component) {
            if (!isfinite(vertex->position[component]) ||
                !isfinite(vertex->normal[component]) ||
                vertex->color[component] != 1.0f) {
                return false;
            }
        }
    }

    return true;
}

static int test_cylinder(void) {
    MeshGeometry geometry = {0};

    CHECK(MeshGeometry_create_cylinder(&geometry, 16) == RETURN_SUCCESS);
    CHECK(geometry.vertices.count == 34);
    CHECK(geometry.indices.count == 192);
    CHECK(geometry.edge_indices.count == 72);

    CHECK(indices_are_valid(&geometry.indices, geometry.vertices.count));
    CHECK(indices_are_valid(&geometry.edge_indices, geometry.vertices.count));
    CHECK(vertices_are_initialized(&geometry.vertices));

    /*
     * Layout:
     *   0..31: side vertices
     *   32: bottom center
     *   33: top center
     *   0..95: side indices
     *   96..143: bottom-cap indices
     *   144..191: top-cap indices
     */
    for (uint32_t i = 0; i < 16; ++i) {
        CHECK(geometry.indices.items[96 + i * 3] == 32);
        CHECK(geometry.indices.items[144 + i * 3] == 33);
    }

    /*
     * A constructor must reject a nonempty destination without changing it.
     */
    Vertex* original_vertices = geometry.vertices.items;
    uint32_t original_vertex_count = geometry.vertices.count;

    CHECK(MeshGeometry_create_cone(&geometry, 16) == RETURN_FAILURE);
    CHECK(geometry.vertices.items == original_vertices);
    CHECK(geometry.vertices.count == original_vertex_count);

    MeshGeometry_release(&geometry);
    CHECK(MeshGeometry_is_empty(&geometry));

    return EXIT_SUCCESS;
}

static int test_cone(void) {
    MeshGeometry geometry = {0};

    CHECK(MeshGeometry_create_cone(&geometry, 16) == RETURN_SUCCESS);
    CHECK(geometry.vertices.count == 18);
    CHECK(geometry.indices.count == 96);
    CHECK(geometry.edge_indices.count == 40);

    CHECK(indices_are_valid(&geometry.indices, geometry.vertices.count));
    CHECK(indices_are_valid(&geometry.edge_indices, geometry.vertices.count));
    CHECK(vertices_are_initialized(&geometry.vertices));

    /*
     * Layout:
     *   0..15: base ring
     *   16: tip
     *   17: base center
     */
    for (uint32_t i = 0; i < 16; ++i) {
        uint32_t next = (i + 1) % 16;
        uint32_t side = i * 3;
        uint32_t base = 48 + i * 3;

        CHECK(geometry.indices.items[side] == i);
        CHECK(geometry.indices.items[side + 1] == 16);
        CHECK(geometry.indices.items[side + 2] == next);

        CHECK(geometry.indices.items[base] == 17);
        CHECK(geometry.indices.items[base + 1] == next);
        CHECK(geometry.indices.items[base + 2] == i);
    }

    CHECK(geometry.vertices.items[17].normal[0] == 0.0f);
    CHECK(geometry.vertices.items[17].normal[1] == -1.0f);
    CHECK(geometry.vertices.items[17].normal[2] == 0.0f);

    MeshGeometry_release(&geometry);
    CHECK(MeshGeometry_is_empty(&geometry));

    return EXIT_SUCCESS;
}

static int test_invalid_divisions(void) {
    MeshGeometry geometry = {0};

    CHECK(MeshGeometry_create_cylinder(NULL, 16) == RETURN_FAILURE);
    CHECK(MeshGeometry_create_cone(NULL, 16) == RETURN_FAILURE);

    CHECK(MeshGeometry_create_cylinder(&geometry, 0) == RETURN_FAILURE);
    CHECK(MeshGeometry_create_cylinder(&geometry, 3) == RETURN_FAILURE);
    CHECK(MeshGeometry_create_cylinder(&geometry, UINT32_MAX) == RETURN_FAILURE);

    CHECK(MeshGeometry_create_cone(&geometry, 0) == RETURN_FAILURE);
    CHECK(MeshGeometry_create_cone(&geometry, 3) == RETURN_FAILURE);
    CHECK(MeshGeometry_create_cone(&geometry, UINT32_MAX) == RETURN_FAILURE);

    CHECK(MeshGeometry_is_empty(&geometry));

    return EXIT_SUCCESS;
}

int main(void) {
    CHECK(test_cylinder() == EXIT_SUCCESS);
    CHECK(test_cone() == EXIT_SUCCESS);
    CHECK(test_invalid_divisions() == EXIT_SUCCESS);
    return EXIT_SUCCESS;
}
