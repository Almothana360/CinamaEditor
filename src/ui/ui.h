#ifndef CE_UI_UI_H
#define CE_UI_UI_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "editor/editor.h"

/**
 * Keystroke combo counter and title pulse manager.
 */
typedef struct {
    int streak;
    float decay_timer;
    float max_timer;
    float title_scale;
} ComboSystem;

/**
 * Context menu actions.
 */
typedef enum {
    CTX_COPY = 0,
    CTX_CUT,
    CTX_PASTE,
    CTX_SELECT_ALL,
    CTX_SEP1,
    CTX_CAM_BOUNDS,
    CTX_CAM_CURSOR,
    CTX_CAM_LINE,
    CTX_SEP2,
    CTX_ZOOM_IN,
    CTX_ZOOM_OUT,
    CTX_ZOOM_RESET,
    CTX_SEP3,
    CTX_UI_SCALE,
    CTX_THEME_CYCLE,
    CTX_SPOTLIGHT_TOGGLE,
    CTX_CRT_TOGGLE,
    CTX_COUNT
} ContextAction;

/**
 * Visual metadata for individual context menu entries.
 */
typedef struct {
    const char *label;
    const char *shortcut;
    bool is_separator;
} ContextMenuItem;

/**
 * Right-click context menu state.
 */
typedef struct {
    bool active;
    Vector2 pos;
    int hovered_idx;
} ContextMenu;

/* --- Combo Lifecycle --- */

void Combo_Init(ComboSystem *combo);
void Combo_Update(ComboSystem *combo, float dt);
void Combo_RegisterHit(ComboSystem *combo);

/* --- Context Menu Lifecycle --- */

void ContextMenu_Init(ContextMenu *menu);
void ContextMenu_Open(ContextMenu *menu, Vector2 screen_pos);
void ContextMenu_Close(ContextMenu *menu);
ContextAction ContextMenu_GetHoveredAction(const ContextMenu *menu);

/* --- UI Rendering Passes --- */

void UI_DrawMinimap(const Editor *ed, Camera2D camera, int screen_w, int screen_h, float line_height, float scale, const Theme *theme);
void UI_DrawContextMenu(ContextMenu *menu, int screen_w, int screen_h, float scale, CCameraMode cam_mode, const Theme *theme, Font font_body);
void UI_DrawComboHUD(const ComboSystem *combo, int screen_w, float scale, const Theme *theme, Font font_body);
void UI_DrawHelp(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body);
void UI_DrawStatusBar(int screen_h, float scale, CCameraMode cam_mode, float zoom, float user_zoom_mult, const Theme *theme, Font font_body);

#endif // CE_UI_UI_H