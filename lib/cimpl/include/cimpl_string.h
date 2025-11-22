#ifndef CIMPL_STRING_H
#define CIMPL_STRING_H

#include "cimpl_core.h"

#define DEFAULT_STRING_CAPACITY 256
DEFINE_DYNAMIC_ARRAY(char, String)
DEFINE_DYNAMIC_ARRAY(String, StringArray)

typedef struct StringView {
    char* items;
    u32 count;
} StringView;

typedef struct StringRingBuffer {
    char* items;
    // Always pointing at the first valid position
    u32 read_index;
    u32 count;
    u32 capacity;
} StringRingBuffer;

/* String */

CimplReturn String_push_view(String*, StringView*);
CimplReturn String_push_literal(String*, const char*);

/* StringView */

u16 StringView_to_u16(StringView*);
u8 StringView_to_u8(StringView*);

/* StringArray */

StringArray* StringArray_default(void);

/* StringRingBuffer */

CimplReturn StringRingBuffer_reserve(StringRingBuffer*, u32);
CimplReturn StringRingBuffer_push(StringRingBuffer*, StringView*);
void StringRingBuffer_clear(StringRingBuffer*);
void StringRingBuffer_free(StringRingBuffer*);

/*** FUNCTION DEFINITIONS ***/

#ifdef CIMPL_IMPLEMENTATION
//  Concatenates a string view to the end of a string
CimplReturn String_push_view(String* dst, StringView* src) {
    if (String_reserve(dst, dst->count + src->count) != RETURN_OK) {
        return RETURN_ERR;
    }
    memcpy(&dst->items[dst->count], src->items, src->count);
    dst->count += src->count;
    return RETURN_OK;
}

// Pushes a string literal to the end of a string
CimplReturn String_push_literal(String* str, const char* items) {
    if (items == NULL) return RETURN_OK;
    u32 count = strlen(items);
    if (String_reserve(str, str->count + count) != RETURN_OK) {
        return RETURN_ERR;
    }
    memcpy(&str->items[str->count], items, count);
    str->count += count;
    return RETURN_OK;
}

/* StringView */

u16 StringView_to_u16(StringView* str) {
    u32 result = 0;
    for (u32 i = 0; i < str->count; ++i) {
        char c = str->items[i];
        if (c < '0' || c > '9') {
            fprintf(
                stderr,
                "Found invalid char %c in IP address %*.s\n",
                c,
                str->count,
                str->items
            );
            return -1;
        }
        result = result * 10 + (str->items[i] - '0');
        if (result > 65535) {
            fprintf(stderr, "Overflow Error: result = %d", result);
            return -1;
        }
    }
    return (u16)result;
}

u8 StringView_to_u8(StringView* str) {
    u16 result = StringView_to_u16(str);
    if (result > 255) {
        fprintf(stderr, "Overflow Error: result = %d", result);
        return -1;
    }
    return (u8)result;
}

/* StringArray */

// Provides a pointer to a new string array with default capacity
// NOTE: This function allocates.  It is the responsibility of the user to free.
StringArray* StringArray_default(void) {
    String* items = calloc(DEFAULT_ARRAY_CAPACITY, sizeof(String));
    if (items == NULL) {
        fprintf(stderr, "Out of memory");
        return NULL;
    }
    StringArray* arr = malloc(sizeof(StringArray));
    if (arr == NULL) {
        free(items);
        fprintf(stderr, "Out of memory");
        return NULL;
    }
    arr->items = items;
    arr->capacity = DEFAULT_ARRAY_CAPACITY;
    arr->count = 0;
    return arr;
}

/* StringRingBuffer */

// Ensures capacity is at least the requested size
CimplReturn StringRingBuffer_reserve(StringRingBuffer* buf, u32 capacity) {
    if (capacity > buf->capacity) {
        if (buf->capacity == 0) {
            buf->capacity = DEFAULT_STRING_CAPACITY;
        }
        while (capacity > buf->capacity) {
            buf->capacity *= 2;
        }
        buf->items = realloc(buf->items, buf->capacity);
        if (buf->items == NULL) {
            fprintf(stderr, "Out of memory");
            return RETURN_ERR;
        }
    }
    return RETURN_OK;
}

// Copies string view contents to the buffer
CimplReturn StringRingBuffer_push(StringRingBuffer* dst, StringView* src) {
    u32 vacant = dst->capacity - dst->count;
    if (src->count > vacant) {
        CimplReturn err =
            StringRingBuffer_reserve(dst, dst->capacity + src->count);
        if (err != RETURN_OK) return RETURN_ERR;
    }
    // Find index after last valid character
    u32 write_index = dst->read_index + dst->count % dst->capacity;
    memcpy(&dst->items[write_index], src->items, src->count);
    dst->count += src->count;
    return RETURN_OK;
}

// Zeros the memory but retains allocated space
void StringRingBuffer_clear(StringRingBuffer* buf) {
    memset(buf->items, 0, buf->capacity);
    buf->count = 0;
    buf->read_index = 0;
}

// Release allocated memory
void StringRingBuffer_free(StringRingBuffer* buf) {
    free(buf->items);
    buf->items = NULL;
    buf->read_index = 0;
    buf->count = 0;
    buf->capacity = 0;
}
#endif /* CIMPL_IMPLEMENTATION */

#endif /* CIMPL_STRING_H */
