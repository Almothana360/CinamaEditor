#ifndef CE_BUFFER_LINE_H
#define CE_BUFFER_LINE_H

#include "raylib.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * Dynamic text line buffer representation.
 */
typedef struct {
    char *chars;
    size_t size;
    size_t capacity;
    bool starts_in_comment;
} Line;

/**
 * Initializes a new empty Line buffer.
 */
void Line_Init(Line *line);

/**
 * Frees heap-allocated characters and resets Line state.
 */
void Line_Free(Line *line);

/**
 * Ensures the internal line capacity has enough space for at least `needed` characters.
 */
void Line_Reserve(Line *line, size_t needed);

/**
 * Inserts a single character at the specified column offset.
 */
void Line_InsertChar(Line *line, size_t col, char c);

/**
 * Deletes a character at the specified column offset.
 */
void Line_DeleteChar(Line *line, size_t col);

/**
 * Appends a string slice of length `len` to the end of the line.
 */
void Line_AppendStr(Line *line, const char *str, size_t len);

/**
 * Calculates the horizontal screen offset (x-coordinate) for a given character column
 * using the specified font metrics.
 */
float Line_GetColX(Font font, const Line *line, size_t col, float font_size, float spacing);

#endif // CE_BUFFER_LINE_H