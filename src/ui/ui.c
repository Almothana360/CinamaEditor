#include "ui/ui.h"
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

void Combo_Init(ComboSystem *combo) {
    if (!combo) return;
    combo->streak = 0;
    combo->decay_timer = 0.0f;
    combo->max_timer = 1.35f;
    combo->title_scale = 1.0f;
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

void Combo_RegisterHit(ComboSystem *combo) {
    if (!combo) return;
    combo->streak++;
    combo->decay_timer = combo->max_timer;
    combo->title_scale = 1.35f;
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

void UI_DrawMinimap(const Editor *ed, Camera2D camera, int screen_w, int screen_h, float line_height, float scale, const Theme *theme) {
    if (!ed || !theme) return;

    float mm_w = 120.0f * scale;
    float mm_h = (float)screen_h - (40.0f * scale);
    float mm_x = (float)screen_w - mm_w - (15.0f * scale);
    float mm_y = 20.0f * scale;

    DrawRectangleRounded((Rectangle){ mm_x, mm_y, mm_w, mm_h }, 0.08f, 4, ColorAlpha(theme->gutter_bg, 0.75f));
    DrawRectangleRoundedLines((Rectangle){ mm_x, mm_y, mm_w, mm_h }, 0.08f, 4, ColorAlpha(theme->gutter_num, 0.45f));

    if (ed->line_count == 0) return;

    float line_scale = fminf((mm_h - (20.0f * scale)) / (float)ed->line_count, 3.5f * scale);
    for (size_t r = 0; r < ed->line_count; ++r) {
        float line_w = fminf((float)ed->lines[r].size * 1.5f * scale, mm_w - (18.0f * scale));
        if (line_w <= 0.0f) continue;
        float ly = mm_y + (10.0f * scale) + (float)r * line_scale;
        Color c = (r == ed->cursor_row) ? theme->cursor : ColorAlpha(theme->syn_default, 0.35f);
        DrawRectangle((int)(mm_x + (8.0f * scale)), (int)ly, (int)line_w, (int)fmaxf(line_scale - 1.0f, 1.0f), c);
    }

    float total_code_h = (float)ed->line_count * line_height;
    if (total_code_h > 0.0f) {
        float vp_top = (camera.target.y - ((float)screen_h * 0.5f) / camera.zoom) / total_code_h;
        float vp_h = ((float)screen_h / camera.zoom) / total_code_h;
        float box_y = mm_y + (10.0f * scale) + vp_top * (ed->line_count * line_scale);
        float box_h = vp_h * (ed->line_count * line_scale);
        DrawRectangleLinesEx((Rectangle){ mm_x + (4.0f * scale), box_y, mm_w - (8.0f * scale), fmaxf(box_h, 8.0f) }, 1.0f, ColorAlpha(theme->cursor, 0.65f));
    }
}

void UI_DrawContextMenu(ContextMenu *menu, int screen_w, int screen_h, float scale, CCameraMode cam_mode, const Theme *theme, Font font_body) {
    if (!menu || !menu->active || !theme) return;

    float item_height = 28.0f * scale;
    float sep_height = 8.0f * scale;
    float width = 270.0f * scale;

    float total_h = 0.0f;
    for (int i = 0; i < CTX_COUNT; ++i) {
        total_h += MENU_ITEMS[i].is_separator ? sep_height : item_height;
    }

    if (menu->pos.x + width > screen_w) menu->pos.x = screen_w - width - 8.0f;
    if (menu->pos.y + total_h > screen_h) menu->pos.y = screen_h - total_h - 8.0f;
    if (menu->pos.x < 0) menu->pos.x = 4.0f;
    if (menu->pos.y < 0) menu->pos.y = 4.0f;

    Rectangle bg_rect = { menu->pos.x, menu->pos.y, width, total_h };
    DrawRectangleRounded(bg_rect, 0.04f, 4, theme->menu_bg);
    DrawRectangleRoundedLines(bg_rect, 0.04f, 4, theme->menu_border);

    Vector2 mpos = GetMousePosition();
    float cur_y = menu->pos.y;
    menu->hovered_idx = -1;

    for (int i = 0; i < CTX_COUNT; ++i) {
        if (MENU_ITEMS[i].is_separator) {
            DrawLine((int)(menu->pos.x + 8.0f * scale), (int)(cur_y + sep_height * 0.5f),
                     (int)(menu->pos.x + width - 8.0f * scale), (int)(cur_y + sep_height * 0.5f),
                     ColorAlpha(theme->gutter_num, 0.4f));
            cur_y += sep_height;
            continue;
        }

        Rectangle item_rect = { menu->pos.x + 4.0f * scale, cur_y + 2.0f * scale, width - 8.0f * scale, item_height - 4.0f * scale };
        bool hovered = CheckCollisionPointRec(mpos, item_rect);
        if (hovered) {
            menu->hovered_idx = i;
            DrawRectangleRounded(item_rect, 0.15f, 4, theme->menu_hl);
            DrawRectangle((int)item_rect.x, (int)item_rect.y, (int)(3.0f * scale), (int)item_rect.height, theme->cursor);
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
        DrawTextEx(font_body, label_buf, (Vector2){ item_rect.x + 10.0f * scale, item_rect.y + 4.0f * scale }, 14.0f * scale, 1.0f, text_col);

        Vector2 sz = MeasureTextEx(font_body, MENU_ITEMS[i].shortcut, 12.0f * scale, 1.0f);
        DrawTextEx(font_body, MENU_ITEMS[i].shortcut, (Vector2){ item_rect.x + item_rect.width - sz.x - 8.0f * scale, item_rect.y + 6.0f * scale }, 12.0f * scale, 1.0f, theme->gutter_num);

        cur_y += item_height;
    }
}

void UI_DrawComboHUD(const ComboSystem *combo, int screen_w, float scale, const Theme *theme, Font font_body) {
    if (!combo || combo->streak <= 2 || !theme) return;

    char combo_text[64];
    snprintf(combo_text, sizeof(combo_text), "%d COMBO!", combo->streak);

    Color combo_color = theme->cursor;
    if (combo->streak > 35) combo_color = (Color){ 255, 60, 60, 255 };
    else if (combo->streak > 20) combo_color = (Color){ 255, 130, 40, 255 };

    float combo_font_size = (CE_FONT_SIZE + 12.0f) * combo->title_scale * scale;
    Vector2 sz = MeasureTextEx(font_body, combo_text, combo_font_size, 2.0f);
    Vector2 c_pos = { (float)screen_w - sz.x - (160.0f * scale), 30.0f * scale };

    DrawTextEx(font_body, combo_text, c_pos, combo_font_size, 2.0f, combo_color);

    float bar_w = 120.0f * scale;
    float bar_ratio = combo->decay_timer / combo->max_timer;
    DrawRectangle((int)c_pos.x, (int)(c_pos.y + sz.y + 4.0f * scale), (int)bar_w, (int)(4.0f * scale), ColorAlpha(theme->gutter_num, 0.5f));
    DrawRectangle((int)c_pos.x, (int)(c_pos.y + sz.y + 4.0f * scale), (int)(bar_w * bar_ratio), (int)(4.0f * scale), combo_color);
}

void UI_DrawHelp(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    if (!theme) return;

    DrawRectangle(0, 0, screen_w, screen_h, (Color){ 0, 0, 0, 195 });
    float hw = 660.0f * scale, hh = 470.0f * scale;
    float hx = ((float)screen_w - hw) * 0.5f;
    float hy = ((float)screen_h - hh) * 0.5f;

    DrawRectangleRounded((Rectangle){ hx, hy, hw, hh }, 0.05f, 6, theme->menu_bg);
    DrawRectangleRoundedLines((Rectangle){ hx, hy, hw, hh }, 0.05f, 6, theme->menu_border);

    DrawTextEx(font_body, "Cinema Editor (CE) - Controls", (Vector2){ hx + 25.0f * scale, hy + 20.0f * scale }, (CE_FONT_SIZE + 3.0f) * scale, 1.5f, theme->cursor);

    const char *help_lines[] = {
        "Auto-Indent      : Preserves tabs/spaces automatically upon Enter",
        "Camera Modes     : F5 (Script Fit) | F6 (Cursor Focus) | F7 (Line Focus)",
        "UI Scaling       : F8 (Cycles 50% -> 100% -> 120% -> 150% ... 400%)",
        "Select Text      : Click & Drag with mouse or Shift + Navigation",
        "Context Menu     : Right click anywhere on screen",
        "Copy / Cut / Past: Ctrl+C / Ctrl+X / Ctrl+V",
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

    float ly = hy + 65.0f * scale;
    for (int i = 0; help_lines[i]; ++i) {
        DrawTextEx(font_body, help_lines[i], (Vector2){ hx + 25.0f * scale, ly }, 14.0f * scale, 1.0f, theme->syn_default);
        ly += 24.0f * scale;
    }
}

void UI_DrawStatusBar(int screen_h, float scale, CCameraMode cam_mode, float zoom, float user_zoom_mult, const Theme *theme, Font font_body) {
    if (!theme) return;
    const char *cam_names[] = { "Script Fit", "Cursor Focus", "Line Focus" };

    char stats[256];
    snprintf(stats, sizeof(stats),
             "%s | Cam: %s | UI: %.0f%% | Theme: %s | Zoom: %.2fx (User: %.0f%%) | F1: Help",
             CE_APP_NAME_SHORT, cam_names[cam_mode], scale * 100.0f, theme->name, zoom, user_zoom_mult * 100.0f);

    DrawTextEx(font_body, stats, (Vector2){ 18.0f * scale, (float)screen_h - (24.0f * scale) }, 13.0f * scale, 1.0f, theme->status_text);
}