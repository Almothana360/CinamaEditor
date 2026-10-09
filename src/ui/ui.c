#include "ui/ui.h"
#include "render/canvas.h"
#include "core/event.h"
#include "raymath.h"
#include <stdio.h>

static const ContextMenuItem MENU_ITEMS[] = {
    [CTX_COPY]             = { "Copy", "Ctrl+C", false },
    [CTX_CUT]              = { "Cut", "Ctrl+X", false },
    [CTX_PASTE]            = { "Paste", "Ctrl+V", false },
    [CTX_SELECT_ALL]       = { "Select All", "Ctrl+A", false },
    [CTX_SEP1]             = { "", "", true },
    [CTX_CAM_BOUNDS]       = { "Cam: Script Fit", "F5", false },
    [CTX_CAM_CURSOR]       = { "Cam: Cursor Focus", "F6", false },
    [CTX_CAM_LINE]         = { "Cam: Line Focus", "F7", false },
    [CTX_SEP2]             = { "", "", true },
    [CTX_ZOOM_IN]          = { "Zoom In", "Ctrl++", false },
    [CTX_ZOOM_OUT]         = { "Zoom Out", "Ctrl+-", false },
    [CTX_ZOOM_RESET]       = { "Reset Zoom", "Ctrl+0", false },
    [CTX_SEP3]             = { "", "", true },
    [CTX_UI_SCALE]         = { "UI Scale", "F8", false },
    [CTX_THEME_CYCLE]      = { "Next Theme", "F4", false },
    [CTX_SPOTLIGHT_TOGGLE] = { "Toggle Spotlight", "F3", false },
    [CTX_CRT_TOGGLE]       = { "Toggle CRT FX", "F2", false },
};

static ComboSystem *g_combo = NULL;

static void UI_OnAction(EventType type, const void *payload) {
    if (type != EV_ACTION || !payload || !g_combo) return;
    const ActionPayload *p = (const ActionPayload *)payload;

    if (p->action == ACTION_INSERT_CHAR || p->action == ACTION_INSERT_NEWLINE) {
        g_combo->streak++;
        g_combo->decay_timer = g_combo->max_timer;
        g_combo->title_scale = 1.35f;

        ComboHitPayload cp = { g_combo->streak, p->action, p->char_data };
        Event_Emit(EV_COMBO_HIT, &cp);
    }
}

void Combo_Init(ComboSystem *combo) {
    if (!combo) return;
    combo->streak = 0;
    combo->decay_timer = 0.0f;
    combo->max_timer = 1.35f;
    combo->title_scale = 1.0f;

    g_combo = combo;
    Event_Subscribe(EV_ACTION, UI_OnAction);
}

void Combo_Update(ComboSystem *combo, float dt) {
    if (!combo) return;
    if (combo->streak > 0) {
        combo->decay_timer -= dt;
        if (combo->decay_timer <= 0.0f) {
            combo->streak = 0;
            combo->decay_timer = 0.0f;
        }
    }
    combo->title_scale = Lerp(combo->title_scale, 1.0f, 8.0f * dt);
}

void ContextMenu_Init(ContextMenu *menu) {
    if (!menu) return;
    menu->active = false;
    menu->pos = (Vector2){ 0, 0 };
    menu->hovered_idx = -1;
}

void ContextMenu_Open(ContextMenu *menu, Vector2 screen_pos) {
    if (!menu) return;
    menu->active = true;
    menu->pos = screen_pos;
    menu->hovered_idx = -1;
}

void ContextMenu_Close(ContextMenu *menu) {
    if (!menu) return;
    menu->active = false;
    menu->hovered_idx = -1;
}

ContextAction ContextMenu_GetHoveredAction(const ContextMenu *menu) {
    if (!menu || !menu->active || menu->hovered_idx < 0 || menu->hovered_idx >= CTX_COUNT) {
        return (ContextAction)-1;
    }
    return (ContextAction)menu->hovered_idx;
}

void UI_DrawMinimap(const Document *doc, Camera2D camera, int screen_w, int screen_h, float line_height, float scale, const Theme *theme) {
    if (!doc || !theme) return;
    UICanvas canvas = Canvas_Create(screen_w, screen_h, scale);
    float s = canvas.scale;

    // Minimap floats on the right with a 10px top margin matching the top menu bar, and clears the status bar
    UIMargins mm_margins = Canvas_Margins(0.0f, 10.0f, 16.0f, 46.0f);
    Rectangle mm_rect = Canvas_GetRect(&canvas, ANCHOR_RIGHT_FILL, 120.0f, 0.0f, mm_margins);
    float mm_x = mm_rect.x;
    float mm_y = mm_rect.y;
    float mm_w = mm_rect.width;
    float mm_h = mm_rect.height;

    DrawRectangleRounded(mm_rect, 0.08f, 4, ColorAlpha(theme->gutter_bg, 0.78f));
    DrawRectangleRoundedLines(mm_rect, 0.08f, 4, ColorAlpha(theme->gutter_num, 0.45f));

    if (doc->line_count == 0) return;
    float line_scale = fminf((mm_h - (20.0f * s)) / (float)doc->line_count, 3.5f * s);

    for (size_t r = 0; r < doc->line_count; ++r) {
        float line_w = fminf((float)doc->lines[r].size * 1.5f * s, mm_w - (18.0f * s));
        if (line_w <= 0.0f) continue;
        float ly = mm_y + (10.0f * s) + (float)r * line_scale;
        Color c = (r == doc->cursor_row) ? theme->cursor : ColorAlpha(theme->syn_default, 0.35f);
        DrawRectangle((int)(mm_x + (8.0f * s)), (int)ly, (int)line_w, (int)fmaxf(line_scale - 1.0f, 1.0f), c);
    }

    float total_code_h = (float)doc->line_count * line_height;
    if (total_code_h > 0.0f) {
        float vp_top = (camera.target.y - ((float)screen_h * 0.5f) / camera.zoom) / total_code_h;
        float vp_h = ((float)screen_h / camera.zoom) / total_code_h;
        float box_y = mm_y + (10.0f * s) + vp_top * (doc->line_count * line_scale);
        float box_h = vp_h * (doc->line_count * line_scale);
        DrawRectangleLinesEx((Rectangle){ mm_x + (4.0f * s), box_y, mm_w - (8.0f * s), fmaxf(box_h, 8.0f) }, 1.0f, ColorAlpha(theme->cursor, 0.65f));
    }
}

void UI_DrawContextMenu(ContextMenu *menu, int screen_w, int screen_h, float scale, CCameraMode cam_mode, const Theme *theme, Font font_body) {
    if (!menu || !menu->active || !theme) return;
    UICanvas canvas = Canvas_Create(screen_w, screen_h, scale);
    float s = canvas.scale;

    float item_height = 28.0f * s;
    float sep_height = 8.0f * s;
    float width = 270.0f * s;
    float total_h = 0.0f;

    for (int i = 0; i < CTX_COUNT; ++i) {
        total_h += MENU_ITEMS[i].is_separator ? sep_height : item_height;
    }

    Rectangle bg_rect = { menu->pos.x, menu->pos.y, width, total_h };
    bg_rect = Canvas_ClampRect(&canvas, bg_rect);
    menu->pos.x = bg_rect.x;
    menu->pos.y = bg_rect.y;

    DrawRectangleRounded(bg_rect, 0.06f, 4, theme->menu_bg);
    DrawRectangleRoundedLines(bg_rect, 0.06f, 4, theme->menu_border);

    Vector2 mpos = GetMousePosition();
    float cur_y = menu->pos.y;
    menu->hovered_idx = -1;

    for (int i = 0; i < CTX_COUNT; ++i) {
        if (MENU_ITEMS[i].is_separator) {
            DrawLine((int)(menu->pos.x + 8.0f * s), (int)(cur_y + sep_height * 0.5f),
                     (int)(menu->pos.x + width - 8.0f * s), (int)(cur_y + sep_height * 0.5f),
                     ColorAlpha(theme->gutter_num, 0.4f));
            cur_y += sep_height;
            continue;
        }

        Rectangle item_rect = { menu->pos.x + 4.0f * s, cur_y + 2.0f * s, width - 8.0f * s, item_height - 4.0f * s };
        bool hovered = Canvas_ContainsPoint(item_rect, mpos);

        if (hovered) {
            menu->hovered_idx = i;
            DrawRectangleRounded(item_rect, 0.15f, 4, theme->menu_hl);
            DrawRectangle((int)item_rect.x, (int)item_rect.y, (int)(3.0f * s), (int)item_rect.height, theme->cursor);
        }

        char label_buf[64];
        if (i == CTX_CAM_BOUNDS) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_BOUNDS_FIT ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_CAM_CURSOR) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_CURSOR_FOCUS ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_CAM_LINE) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_LINE_FOCUS ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_UI_SCALE) {
            snprintf(label_buf, sizeof(label_buf), "UI Scale: %.0f%%", scale * 100.0f);
        } else {
            snprintf(label_buf, sizeof(label_buf), "%s", MENU_ITEMS[i].label);
        }

        Color text_col = hovered ? theme->cursor : theme->syn_default;
        DrawTextEx(font_body, label_buf, (Vector2){ item_rect.x + 10.0f * s, item_rect.y + 4.0f * s }, 14.0f * s, 1.0f, text_col);

        Vector2 sz = MeasureTextEx(font_body, MENU_ITEMS[i].shortcut, 12.0f * s, 1.0f);
        DrawTextEx(font_body, MENU_ITEMS[i].shortcut, (Vector2){ item_rect.x + item_rect.width - sz.x - 8.0f * s, item_rect.y + 6.0f * s }, 12.0f * s, 1.0f, theme->gutter_num);

        cur_y += item_height;
    }
}

void UI_DrawComboHUD(const ComboSystem *combo, int screen_w, float scale, const Theme *theme, Font font_body) {
    if (!combo || combo->streak <= 2 || !theme) return;
    int screen_h = GetScreenHeight();
    UICanvas canvas = Canvas_Create(screen_w, screen_h, scale);
    float s = canvas.scale;

    char combo_text[64];
    snprintf(combo_text, sizeof(combo_text), "%d COMBO!", combo->streak);

    Color combo_color = theme->cursor;
    if (combo->streak > 35) combo_color = (Color){ 255, 60, 60, 255 };
    else if (combo->streak > 20) combo_color = (Color){ 255, 130, 40, 255 };

    float combo_font_size = (CE_FONT_SIZE + 12.0f) * combo->title_scale * s;
    Vector2 sz = MeasureTextEx(font_body, combo_text, combo_font_size, 2.0f);

    Rectangle hud_rect = Canvas_GetRect(&canvas, ANCHOR_TOP_RIGHT, sz.x / s, (sz.y + 10.0f) / s, Canvas_Margins(0.0f, 50.0f, 160.0f, 0.0f));
    DrawTextEx(font_body, combo_text, (Vector2){ hud_rect.x, hud_rect.y }, combo_font_size, 2.0f, combo_color);

    float bar_w = 120.0f * s;
    float bar_ratio = combo->decay_timer / combo->max_timer;
    DrawRectangle((int)hud_rect.x, (int)(hud_rect.y + sz.y + 4.0f * s), (int)bar_w, (int)(4.0f * s), ColorAlpha(theme->gutter_num, 0.5f));
    DrawRectangle((int)hud_rect.x, (int)(hud_rect.y + sz.y + 4.0f * s), (int)(bar_w * bar_ratio), (int)(4.0f * s), combo_color);
}

void UI_DrawHelp(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    if (!theme) return;
    UICanvas canvas = Canvas_Create(screen_w, screen_h, scale);
    float s = canvas.scale;

    DrawRectangle(0, 0, screen_w, screen_h, (Color){ 0, 0, 0, 195 });

    Rectangle h_rect = Canvas_GetRect(&canvas, ANCHOR_CENTER, 660.0f, 470.0f, Canvas_MarginZero());
    h_rect = Canvas_ClampRect(&canvas, h_rect);

    DrawRectangleRounded(h_rect, 0.05f, 6, theme->menu_bg);
    DrawRectangleRoundedLines(h_rect, 0.05f, 6, theme->menu_border);

    DrawTextEx(font_body, "Cinema Editor (CE) - Controls", (Vector2){ h_rect.x + 25.0f * s, h_rect.y + 20.0f * s }, (CE_FONT_SIZE + 3.0f) * s, 1.5f, theme->cursor);

    const char *help_lines[] = {
        "Auto-Indent      : Preserves tabs/spaces automatically upon Enter",
        "Undo / Redo      : Ctrl+Z / Ctrl+Y (or Ctrl+Shift+Z)",
        "Camera Modes     : F5 (Script Fit) | F6 (Cursor Focus) | F7 (Line Focus)",
        "UI Scaling       : F8 (Cycles 50% -> 100% -> 120% -> 150% ... 400%)",
        "Select Text      : Click & Drag with mouse or Shift + Navigation",
        "Context Menu     : Right click anywhere on screen",
        "Copy / Cut / Paste: Ctrl+C / Ctrl+X / Ctrl+V",
        "Zoom In/Out/Reset: Ctrl++ / Ctrl+- / Ctrl+0 or Ctrl + Mouse Wheel",
        "Spotlight Mode   : F3 (darkens scene, illuminates mouse)",
        "Switch Theme     : F4 (Cyber Dark, Synthwave, Solarized, Paper)",
        "Word Navigation  : Ctrl + Left / Right",
        "Word Deletion    : Ctrl + Backspace / Ctrl + Delete",
        "Duplicate Line   : Ctrl + D",
        "Select All       : Ctrl + A",
        "Toggle CRT FX    : F2",
        NULL
    };

    float ly = h_rect.y + 65.0f * s;
    for (int i = 0; help_lines[i]; ++i) {
        DrawTextEx(font_body, help_lines[i], (Vector2){ h_rect.x + 25.0f * s, ly }, 14.0f * s, 1.0f, theme->syn_default);
        ly += 24.0f * s;
    }
}

void UI_DrawStatusBar(int screen_h, float scale, CCameraMode cam_mode, float zoom, float user_zoom_mult, const Theme *theme, Font font_body) {
    if (!theme) return;
    int screen_w = GetScreenWidth();
    UICanvas canvas = Canvas_Create(screen_w, screen_h, scale);
    float s = canvas.scale;

    // Floating rounded rectangle status bar with margins from bottom and sides
    UIMargins sb_margins = Canvas_Margins(16.0f, 0.0f, 16.0f, 10.0f);
    Rectangle bar_rect = Canvas_GetRect(&canvas, ANCHOR_BOTTOM_FILL, 0.0f, 26.0f, sb_margins);

    DrawRectangleRounded(bar_rect, 0.25f, 6, theme->status_bg);
    DrawRectangleRoundedLines(bar_rect, 0.25f, 6, ColorAlpha(theme->gutter_num, 0.45f));

    const char *cam_names[] = { "Script Fit", "Cursor Focus", "Line Focus" };
    char stats[256];
    snprintf(stats, sizeof(stats),
             "%s | Cam: %s | UI: %.0f%% | Theme: %s | Zoom: %.2fx (User: %.0f%%) | F1: Help",
             CE_APP_NAME_SHORT, cam_names[cam_mode], scale * 100.0f, theme->name, zoom, user_zoom_mult * 100.0f);

    float font_size = 13.0f * s;
    float text_y = bar_rect.y + (bar_rect.height - font_size) * 0.5f;
    DrawTextEx(font_body, stats, (Vector2){ bar_rect.x + 16.0f * s, text_y }, font_size, 1.0f, theme->status_text);
}