#include <stdio.h>
#include <stdlib.h>
#include "binary_writer.h"

/* Initialize code buffer with dynamic allocation */
void init_code_buffer(CodeBuffer *buffer, size_t initial_capacity)
{
    buffer->bytes = (uint8_t *)malloc(initial_capacity);
    if (!buffer->bytes) {
        perror("malloc for code buffer");
        exit(1);
    }
    buffer->size = 0;
    buffer->capacity = initial_capacity;
}

/* Initialize data buffer with dynamic allocation */
void init_data_buffer(DataBuffer *buffer, size_t initial_capacity)
{
    buffer->bytes = (uint8_t *)malloc(initial_capacity);
    if (!buffer->bytes) {
        perror("malloc for data buffer");
        exit(1);
    }
    buffer->size = 0;
    buffer->capacity = initial_capacity;
}

/* Free allocated memory in code buffer */
void free_code_buffer(CodeBuffer *buffer)
{
    if (buffer->bytes) {
        free(buffer->bytes);
        buffer->bytes = NULL;
    }
    buffer->size = 0;
    buffer->capacity = 0;
}

/* Free allocated memory in data buffer */
void free_data_buffer(DataBuffer *buffer)
{
    if (buffer->bytes) {
        free(buffer->bytes);
        buffer->bytes = NULL;
    }
    buffer->size = 0;
    buffer->capacity = 0;
}

/* Ensure there's enough space in a buffer for additional bytes */
void ensure_buffer_capacity(void *buf, size_t additional_bytes)
{
    CodeBuffer *buffer = (CodeBuffer *)buf;  // Works for both CodeBuffer and DataBuffer

    if (buffer->size + additional_bytes > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        if (new_capacity < buffer->size + additional_bytes) {
            new_capacity = buffer->size + additional_bytes + 1024;  // Add some extra space
        }

        uint8_t *new_buffer = (uint8_t *)realloc(buffer->bytes, new_capacity);
        if (!new_buffer) {
            perror("realloc for buffer");
            exit(1);
        }

        buffer->bytes = new_buffer;
        buffer->capacity = new_capacity;
    }
}