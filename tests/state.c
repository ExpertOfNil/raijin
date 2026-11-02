#define _POSIX_C_SOURCE 200809L

#include <raijin/raijin.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(expression)                                             \
    do {                                                              \
        if (!(expression)) {                                          \
            fprintf(stderr, "%s:%d: %s\n",                             \
                    __FILE__, __LINE__, #expression);                 \
            goto cleanup;                                             \
        }                                                             \
    } while (0)

/* Independent version-one fixture, including a negative-zero component. */
static const uint8_t expected[108] = {
    /* Header: RJST, version 1, payload size 96. */
    0x52, 0x4a, 0x53, 0x54,
    0x01, 0x00, 0x00, 0x00,
    0x60, 0x00, 0x00, 0x00,

    /* Target: 1, -2, 0.5. */
    0x00, 0x00, 0x80, 0x3f,
    0x00, 0x00, 0x00, 0xc0,
    0x00, 0x00, 0x00, 0x3f,

    /* Orientation: 0, -0, 0, 1. */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x3f,

    /* Distance: 7. */
    0x00, 0x00, 0xe0, 0x40,

    /* Projection entries: 1 through 16. */
    0x00, 0x00, 0x80, 0x3f,
    0x00, 0x00, 0x00, 0x40,
    0x00, 0x00, 0x40, 0x40,
    0x00, 0x00, 0x80, 0x40,
    0x00, 0x00, 0xa0, 0x40,
    0x00, 0x00, 0xc0, 0x40,
    0x00, 0x00, 0xe0, 0x40,
    0x00, 0x00, 0x00, 0x41,
    0x00, 0x00, 0x10, 0x41,
    0x00, 0x00, 0x20, 0x41,
    0x00, 0x00, 0x30, 0x41,
    0x00, 0x00, 0x40, 0x41,
    0x00, 0x00, 0x50, 0x41,
    0x00, 0x00, 0x60, 0x41,
    0x00, 0x00, 0x70, 0x41,
    0x00, 0x00, 0x80, 0x41
};

static bool write_bytes(
    const char* path, const uint8_t* bytes, size_t count
) {
    FILE* file = fopen(path, "wb");
    if (file == NULL) return false;

    bool success = fwrite(bytes, 1, count, file) == count;
    if (fclose(file) != 0) success = false;
    return success;
}

static bool file_matches_fixture(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) return false;

    /* One extra byte ensures an oversized file cannot pass. */
    uint8_t bytes[sizeof(expected) + 1];
    size_t count = fread(bytes, 1, sizeof(bytes), file);
    bool success = !ferror(file) &&
                   count == sizeof(expected) &&
                   memcmp(bytes, expected, sizeof(expected)) == 0;

    if (fclose(file) != 0) success = false;
    return success;
}

static bool states_equal_bits(
    const RaijinState* lhs, const RaijinState* rhs
) {
    /*
     * Compare members, not complete structs: struct padding is not
     * part of the state or its serialization contract.
     */
    return memcmp(
               lhs->camera.target,
               rhs->camera.target,
               sizeof(lhs->camera.target)
           ) == 0 &&
           memcmp(
               lhs->camera.orientation,
               rhs->camera.orientation,
               sizeof(lhs->camera.orientation)
           ) == 0 &&
           memcmp(
               &lhs->camera.distance,
               &rhs->camera.distance,
               sizeof(lhs->camera.distance)
           ) == 0 &&
           memcmp(
               lhs->camera.projection,
               rhs->camera.projection,
               sizeof(lhs->camera.projection)
           ) == 0;
}

static bool load_fails_unchanged(
    const char* path, RaijinResult expected_result
) {
    RaijinState output;
    unsigned char before[sizeof(output)];

    /* Inspect these bytes only as object representation on failure. */
    memset(&output, 0xa5, sizeof(output));
    memcpy(before, &output, sizeof(output));

    RaijinResult result = raijin_state_load(path, &output);
    return result == expected_result &&
           memcmp(before, &output, sizeof(output)) == 0;
}

int main(void) {
    int exit_status = EXIT_FAILURE;
    char path[] = "/tmp/raijin-state-XXXXXX";

    int fd = mkstemp(path);
    if (fd < 0) {
        perror("mkstemp");
        return EXIT_FAILURE;
    }

    CHECK(close(fd) == 0);

    RaijinState source = {
        .camera = {
            .target = {1.0f, -2.0f, 0.5f},
            .orientation = {0.0f, -0.0f, 0.0f, 1.0f},
            .distance = 7.0f
        }
    };
    for (size_t i = 0; i < 16; ++i) {
        source.camera.projection[i] = (float)(i + 1);
    }

    /* Verify encoding against independent bytes, not only the decoder. */
    CHECK(raijin_state_save(&source, path) == RAIJIN_SUCCESS);
    CHECK(file_matches_fixture(path));

    RaijinState loaded = {0};
    CHECK(raijin_state_load(path, &loaded) == RAIJIN_SUCCESS);
    CHECK(states_equal_bits(&source, &loaded));

    /* Verify decoding independently of the encoder. */
    CHECK(write_bytes(path, expected, sizeof(expected)));
    CHECK(raijin_state_load(path, &loaded) == RAIJIN_SUCCESS);
    CHECK(states_equal_bits(&source, &loaded));

    /* Every possible truncation must fail without modifying output. */
    for (size_t count = 0; count < sizeof(expected); ++count) {
        CHECK(write_bytes(path, expected, count));
        CHECK(load_fails_unchanged(path, RAIJIN_ERROR_INVALID_FORMAT));
    }

    uint8_t malformed[sizeof(expected) + 1];
    memcpy(malformed, expected, sizeof(expected));

    malformed[0] ^= 0xff;
    CHECK(write_bytes(path, malformed, sizeof(expected)));
    CHECK(load_fails_unchanged(path, RAIJIN_ERROR_INVALID_FORMAT));

    memcpy(malformed, expected, sizeof(expected));
    malformed[4] = 2;
    CHECK(write_bytes(path, malformed, sizeof(expected)));
    CHECK(load_fails_unchanged(path, RAIJIN_ERROR_UNSUPPORTED_VERSION));

    memcpy(malformed, expected, sizeof(expected));
    malformed[8] = 95;
    CHECK(write_bytes(path, malformed, sizeof(expected)));
    CHECK(load_fails_unchanged(path, RAIJIN_ERROR_INVALID_FORMAT));

    memcpy(malformed, expected, sizeof(expected));
    malformed[sizeof(expected)] = 0;
    CHECK(write_bytes(path, malformed, sizeof(malformed)));
    CHECK(load_fails_unchanged(path, RAIJIN_ERROR_INVALID_FORMAT));

    /* Saving must truncate an existing, longer file. */
    CHECK(raijin_state_save(&source, path) == RAIJIN_SUCCESS);
    CHECK(file_matches_fixture(path));

    CHECK(raijin_state_save(NULL, path) == RAIJIN_ERROR_INVALID_ARGUMENT);
    CHECK(raijin_state_save(&source, NULL) == RAIJIN_ERROR_INVALID_ARGUMENT);
    CHECK(raijin_state_save(&source, "") == RAIJIN_ERROR_INVALID_ARGUMENT);
    CHECK(raijin_state_load(path, NULL) == RAIJIN_ERROR_INVALID_ARGUMENT);
    CHECK(load_fails_unchanged(NULL, RAIJIN_ERROR_INVALID_ARGUMENT));
    CHECK(load_fails_unchanged("", RAIJIN_ERROR_INVALID_ARGUMENT));

    CHECK(remove(path) == 0);
    CHECK(load_fails_unchanged(path, RAIJIN_ERROR_NOT_FOUND));

    exit_status = EXIT_SUCCESS;

cleanup:
    if (exit_status != EXIT_SUCCESS) {
        (void)remove(path);
    }
    return exit_status;
}
