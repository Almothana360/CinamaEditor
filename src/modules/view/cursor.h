#ifndef CE_MODULES_VIEW_CURSOR_H
#define CE_MODULES_VIEW_CURSOR_H

#include "raylib.h"
#include "modules/buffer/document.h"

// Animated cursor with high-speed spring interpolation and trailing blur.
typedef struct {
    Vector2 current;
    Vector2 target;
    Vector2 trail;
    float width;
    float height;
} SmoothCursor;

// Initializes smooth cursor coordinates and dimensions.
void Cursor_Init(SmoothCursor *cursor, float line_height);

// Calculates new target from the document and interpolates physics.
void Cursor_Update(SmoothCursor *cursor, float dt, const Document *doc, Font font_syntax, float line_height);

#endif // CE_MODULES_VIEW_CURSOR_H