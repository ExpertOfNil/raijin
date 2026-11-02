#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "raijin/raijin.h"

_Static_assert(sizeof(float) == 4, "State files require 32-bit floats");
_Static_assert(
    FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MIN_EXP == -125 &&
        FLT_MAX_EXP == 128,
    "State files require binary32 float properties"
);

enum {
    STATE_VERSION = 1,
    STATE_HEADER_SIZE = 12,
    STATE_PAYLOAD_SIZE = 96,
    STATE_FILE_SIZE = STATE_HEADER_SIZE + STATE_PAYLOAD_SIZE
};

static void write_u32_le(uint32_t value, uint8_t* out) {
    out[0] = (uint8_t)(value);
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

static uint32_t read_u32_le(const uint8_t* in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}

static void write_f32_le(const float* value, uint8_t* out) {
    uint32_t bits;
    memcpy(&bits, value, sizeof(bits));
    write_u32_le(bits, out);
}

static void read_f32_le(const uint8_t* in, float* out) {
    uint32_t bits = read_u32_le(in);
    memcpy(out, &bits, sizeof(bits));
}

static void encode_state(
    const RaijinState* state, uint8_t buffer[STATE_FILE_SIZE]
) {
    memcpy(buffer, "RJST", 4);
    buffer += 4;
    write_u32_le(STATE_VERSION, buffer);
    buffer += 4;
    write_u32_le(STATE_PAYLOAD_SIZE, buffer);
    buffer += 4;

    for (size_t i = 0; i < 3; ++i) {
        write_f32_le(&state->camera.target[i], buffer);
        buffer += 4;
    }

    for (size_t i = 0; i < 4; ++i) {
        write_f32_le(&state->camera.orientation[i], buffer);
        buffer += 4;
    }

    write_f32_le(&state->camera.distance, buffer);
    buffer += 4;

    for (size_t i = 0; i < 16; ++i) {
        write_f32_le(&state->camera.projection[i], buffer);
        buffer += 4;
    }
}

static void decode_state(
    const uint8_t buffer[STATE_FILE_SIZE], RaijinState* state
) {
    buffer += STATE_HEADER_SIZE;
    for (size_t i = 0; i < 3; ++i) {
        read_f32_le(buffer, &state->camera.target[i]);
        buffer += 4;
    }

    for (size_t i = 0; i < 4; ++i) {
        read_f32_le(buffer, &state->camera.orientation[i]);
        buffer += 4;
    }

    read_f32_le(buffer, &state->camera.distance);
    buffer += 4;

    for (size_t i = 0; i < 16; ++i) {
        read_f32_le(buffer, &state->camera.projection[i]);
        buffer += 4;
    }
}

static RaijinResult read_exact(FILE* file, uint8_t* buffer, size_t count) {
    size_t received = fread(buffer, 1, count, file);

    if (ferror(file)) {
        return RAIJIN_ERROR_IO;
    }
    if (received != count) {
        return RAIJIN_ERROR_INVALID_FORMAT;
    }

    return RAIJIN_SUCCESS;
}

RaijinResult raijin_state_save(const RaijinState* state, const char* path) {
    if (state == NULL || path == NULL || path[0] == '\0') {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    uint8_t buffer[STATE_FILE_SIZE];
    encode_state(state, buffer);

    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        return RAIJIN_ERROR_IO;
    }

    RaijinResult result = RAIJIN_SUCCESS;

    if (fwrite(buffer, 1, sizeof(buffer), file) != sizeof(buffer)) {
        result = RAIJIN_ERROR_IO;
    }

    if (fclose(file) != 0) {
        result = RAIJIN_ERROR_IO;
    }
    return result;
}

RaijinResult raijin_state_load(const char* path, RaijinState* out_state) {
    if (path == NULL || path[0] == '\0' || out_state == NULL) {
        return RAIJIN_ERROR_INVALID_ARGUMENT;
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return errno == ENOENT ? RAIJIN_ERROR_NOT_FOUND : RAIJIN_ERROR_IO;
    }

    uint8_t buffer[STATE_FILE_SIZE];
    uint8_t* cursor = buffer;
    RaijinResult result = read_exact(file, buffer, STATE_HEADER_SIZE);

    if (result != RAIJIN_SUCCESS) {
        goto close_file;
    }
    if (memcmp(cursor, "RJST", 4) != 0) {
        result = RAIJIN_ERROR_INVALID_FORMAT;
        goto close_file;
    }
    cursor += 4;

    /*
     * Check version before interpreting version-specific payload
     * size or contents.
     */
    if (read_u32_le(cursor) != STATE_VERSION) {
        result = RAIJIN_ERROR_UNSUPPORTED_VERSION;
        goto close_file;
    }
    cursor += 4;

    if (read_u32_le(cursor) != STATE_PAYLOAD_SIZE) {
        result = RAIJIN_ERROR_INVALID_FORMAT;
        goto close_file;
    }
    cursor += 4;

    result = read_exact(file, cursor, STATE_PAYLOAD_SIZE);
    if (result != RAIJIN_SUCCESS) {
        goto close_file;
    }

    /* Version 1 permits exactly one snapshot and no trailing bytes. */
    int trailing = fgetc(file);
    if (ferror(file)) {
        result = RAIJIN_ERROR_IO;
    } else if (trailing != EOF) {
        result = RAIJIN_ERROR_INVALID_FORMAT;
    }

close_file:
    if (fclose(file) != 0) {
        result = RAIJIN_ERROR_IO;
    }

    if (result != RAIJIN_SUCCESS) {
        return result;
    }

    /*
     * Do not modify caller storage until the complete file has been
     * read, checked, and closed successfully.
     */
    RaijinState decoded = {0};
    decode_state(buffer, &decoded);
    *out_state = decoded;
    return RAIJIN_SUCCESS;
}
