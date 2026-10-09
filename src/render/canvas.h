#ifndef CE_RENDER_CANVAS_H
#define CE_RENDER_CANVAS_H

#include "raylib.h"
#include <stdbool.h>

// UI Canvas Anchor modes defining how an element binds to the screen edges and centers.
typedef enum {
    ANCHOR_TOP_LEFT = 0,
    ANCHOR_TOP_CENTER,
    ANCHOR_TOP_RIGHT,
    ANCHOR_TOP_FILL,        // Stretches across the full top width

    ANCHOR_BOTTOM_LEFT,
    ANCHOR_BOTTOM_CENTER,
    ANCHOR_BOTTOM_RIGHT,
    ANCHOR_BOTTOM_FILL,     // Stretches across the full bottom width

    ANCHOR_LEFT_FILL,       // Stretches across the full left height
    ANCHOR_RIGHT_FILL,      // Stretches across the full right height

    ANCHOR_CENTER,          // Centered horizontally and vertically
    ANCHOR_CENTER_LEFT,
    ANCHOR_CENTER_RIGHT,

    ANCHOR_FULL_FILL,       // Covers the entire canvas
    ANCHOR_COUNT
} UIAnchor;

// Margin / Padding offsets (expressed in unscaled design units)
typedef struct {
    float left;
    float top;
    float right;
    float bottom;
} UIMargins;

// Resolution-independent Canvas context
typedef struct {
    float width;        // Screen width in pixels
    float height;       // Screen height in pixels
    float scale;        // UI scaling multiplier (1.0f = 100%, 1.5f = 150%, etc.)
} UICanvas;

// Initializes a canvas context for the current frame
UICanvas Canvas_Create(int screen_w, int screen_h, float scale);

// Helper margin constructors
static inline UIMargins Canvas_Margins(float left, float top, float right, float bottom) {
    return (UIMargins){ left, top, right, bottom };
}

static inline UIMargins Canvas_MarginAll(float margin) {
    return (UIMargins){ margin, margin, margin, margin };
}

static inline UIMargins Canvas_MarginZero(void) {
    return (UIMargins){ 0.0f, 0.0f, 0.0f, 0.0f };
}

// Applies the canvas scale multiplier to an unscaled dimension or vector
float Canvas_Scale(const UICanvas *canvas, float value);
Vector2 Canvas_ScaleVec(const UICanvas *canvas, Vector2 vec);

// Computes a scaled, anchored screen-space bounding box
Rectangle Canvas_GetRect(const UICanvas *canvas, UIAnchor anchor, float width, float height, UIMargins margins);

// Clamps a rectangle so it stays strictly within visible canvas boundaries
Rectangle Canvas_ClampRect(const UICanvas *canvas, Rectangle rect);

// Subdivides a parent rectangle horizontally into equal slices (Parent Width / total_slots)
Rectangle Canvas_GetSubRect(Rectangle parent, int slot_index, int total_slots);

// Collision testing
bool Canvas_ContainsPoint(Rectangle rect, Vector2 point);

#endif // CE_RENDER_CANVAS_H