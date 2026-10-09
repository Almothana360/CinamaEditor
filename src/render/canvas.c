#include "render/canvas.h"
#include <math.h>

UICanvas Canvas_Create(int screen_w, int screen_h, float scale) {
    float sw = (screen_w > 0) ? (float)screen_w : 1280.0f;
    float sh = (screen_h > 0) ? (float)screen_h : 720.0f;
    float user_s = (scale > 0.1f) ? scale : 1.0f;

    // Dynamically scale UI up or down based on window dimensions relative to 1280x720 base
    float res_scale = fminf(sw / 1280.0f, sh / 720.0f);
    if (res_scale < 0.75f) res_scale = 0.75f;
    float total_scale = user_s * res_scale;

    return (UICanvas){
        .width = sw,
        .height = sh,
        .scale = total_scale
    };
}

float Canvas_Scale(const UICanvas *canvas, float value) {
    if (!canvas) return value;
    return value * canvas->scale;
}

Vector2 Canvas_ScaleVec(const UICanvas *canvas, Vector2 vec) {
    if (!canvas) return vec;
    return (Vector2){ vec.x * canvas->scale, vec.y * canvas->scale };
}

Rectangle Canvas_GetRect(const UICanvas *canvas, UIAnchor anchor, float width, float height, UIMargins margins) {
    if (!canvas) return (Rectangle){ 0, 0, 0, 0 };

    float s = canvas->scale > 0.01f ? canvas->scale : 1.0f;
    float sw = width * s;
    float sh = height * s;
    float ml = margins.left * s;
    float mt = margins.top * s;
    float mr = margins.right * s;
    float mb = margins.bottom * s;

    float cw = canvas->width;
    float ch = canvas->height;

    Rectangle r = { 0, 0, 0, 0 };

    switch (anchor) {
        case ANCHOR_TOP_LEFT:
            r.x = ml;
            r.y = mt;
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_TOP_CENTER:
            r.x = (cw - sw) * 0.5f + (ml - mr);
            r.y = mt;
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_TOP_RIGHT:
            r.x = cw - sw - mr;
            r.y = mt;
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_TOP_FILL:
            r.x = ml;
            r.y = mt;
            r.width = fmaxf(cw - ml - mr, 0.0f);
            r.height = sh;
            break;

        case ANCHOR_BOTTOM_LEFT:
            r.x = ml;
            r.y = ch - sh - mb;
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_BOTTOM_CENTER:
            r.x = (cw - sw) * 0.5f + (ml - mr);
            r.y = ch - sh - mb;
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_BOTTOM_RIGHT:
            r.x = cw - sw - mr;
            r.y = ch - sh - mb;
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_BOTTOM_FILL:
            r.x = ml;
            r.y = ch - sh - mb;
            r.width = fmaxf(cw - ml - mr, 0.0f);
            r.height = sh;
            break;

        case ANCHOR_LEFT_FILL:
            r.x = ml;
            r.y = mt;
            r.width = sw;
            r.height = fmaxf(ch - mt - mb, 0.0f);
            break;

        case ANCHOR_RIGHT_FILL:
            r.x = cw - sw - mr;
            r.y = mt;
            r.width = sw;
            r.height = fmaxf(ch - mt - mb, 0.0f);
            break;

        case ANCHOR_CENTER:
            r.x = (cw - sw) * 0.5f + (ml - mr);
            r.y = (ch - sh) * 0.5f + (mt - mb);
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_CENTER_LEFT:
            r.x = ml;
            r.y = (ch - sh) * 0.5f + (mt - mb);
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_CENTER_RIGHT:
            r.x = cw - sw - mr;
            r.y = (ch - sh) * 0.5f + (mt - mb);
            r.width = sw;
            r.height = sh;
            break;

        case ANCHOR_FULL_FILL:
            r.x = ml;
            r.y = mt;
            r.width = fmaxf(cw - ml - mr, 0.0f);
            r.height = fmaxf(ch - mt - mb, 0.0f);
            break;

        default:
            r = (Rectangle){ ml, mt, sw, sh };
            break;
    }

    return r;
}

Rectangle Canvas_ClampRect(const UICanvas *canvas, Rectangle rect) {
    if (!canvas) return rect;
    float margin = 6.0f * canvas->scale;

    if (rect.width > canvas->width - margin * 2.0f) {
        rect.width = canvas->width - margin * 2.0f;
    }
    if (rect.height > canvas->height - margin * 2.0f) {
        rect.height = canvas->height - margin * 2.0f;
    }

    if (rect.x < margin) rect.x = margin;
    if (rect.y < margin) rect.y = margin;

    if (rect.x + rect.width > canvas->width - margin) {
        rect.x = canvas->width - rect.width - margin;
    }
    if (rect.y + rect.height > canvas->height - margin) {
        rect.y = canvas->height - rect.height - margin;
    }

    return rect;
}

Rectangle Canvas_GetSubRect(Rectangle parent, int slot_index, int total_slots) {
    if (total_slots <= 0 || slot_index < 0 || slot_index >= total_slots) {
        return parent;
    }
    float slot_width = parent.width / (float)total_slots;
    return (Rectangle){
        .x = parent.x + (float)slot_index * slot_width,
        .y = parent.y,
        .width = slot_width,
        .height = parent.height
    };
}

bool Canvas_ContainsPoint(Rectangle rect, Vector2 point) {
    return CheckCollisionPointRec(point, rect);
}