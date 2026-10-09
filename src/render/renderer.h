#ifndef CE_RENDER_RENDERER_H
#define CE_RENDER_RENDERER_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "modules/buffer/document.h"
#include "modules/view/camera.h"
#include "modules/view/cursor.h"
#include "fx/fx.h"
#include "ui/ui.h"

// Aggregates all layout state required for a frame
typedef struct {
    const Document *doc;
    const SmoothCursor *cursor;
    Camera2D camera;
    CCameraMode cam_mode;
    float zoom_mult;
    const FxSystem *fx;
    ContextMenu *menu;
    const ComboSystem *combo;
    bool show_help;
    const Theme *theme;
    float ui_scale;
    Font font_body;
    Font font_syntax;
    int screen_w;
    int screen_h;
    bool is_mouse_dragging;
} RenderContext;

// Orchestrates the exact draw pass layering sequence over all modules
void Renderer_Draw(const RenderContext *ctx);

#endif // CE_RENDER_RENDERER_H