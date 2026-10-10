#include "ui/ui.h"
#include "render/canvas.h"
#include "core/event.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

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
    combo->overdrive_alpha = 1.0f;

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

    float target_alpha = (combo->streak >= 8) ? 0.0f : 1.0f;
    combo->overdrive_alpha = Lerp(combo->overdrive_alpha, target_alpha, 4.5f * dt);
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

void UI_DrawMinimap(const Document *doc, Camera2D camera, int screen_w, int screen_h, float line_height, float scale, const Theme *theme, Font font_body, float alpha) {
    if (!doc || !theme || alpha <= 0.01f) return;
    UICanvas canvas = Canvas_Create(screen_w, screen_h, scale);
    float s = canvas.scale;

    UIMargins mm_margins = Canvas_Margins(0.0f, 10.0f, 16.0f, 46.0f);
    Rectangle mm_rect = Canvas_GetRect(&canvas, ANCHOR_RIGHT_FILL, 120.0f, 0.0f, mm_margins);
    float mm_x = mm_rect.x;
    float mm_y = mm_rect.y;
    float mm_w = mm_rect.width;
    float mm_h = mm_rect.height;

    // 1. Tactical Frame
    DrawRectangleRounded(mm_rect, 0.08f, 4, ColorAlpha(theme->gutter_bg, 0.85f * alpha));
    DrawRectangleRoundedLines(mm_rect, 0.08f, 4, ColorAlpha(theme->cursor, 0.35f * alpha));

    // 2. Header & Tactical Grid
    DrawTextEx(font_body, "T E L E M E T R Y", (Vector2){ mm_x + 8.0f * s, mm_y + 8.0f * s }, 9.5f * s, 1.0f, ColorAlpha(theme->cursor, 0.8f * alpha));
    DrawLine((int)(mm_x + 8.0f * s), (int)(mm_y + 20.0f * s), (int)(mm_x + mm_w - 8.0f * s), (int)(mm_y + 20.0f * s), ColorAlpha(theme->cursor, 0.2f * alpha));

    float map_content_start_y = mm_y + 26.0f * s;
    float max_map_h = mm_h - 36.0f * s;

    // Draw background grid lines
    for (float gy = map_content_start_y; gy < map_content_start_y + max_map_h; gy += 20.0f * s) {
        DrawLine((int)mm_x, (int)gy, (int)(mm_x + mm_w), (int)gy, ColorAlpha(theme->gutter_num, 0.05f * alpha));
    }
    for (float gx = mm_x; gx < mm_x + mm_w; gx += 20.0f * s) {
        DrawLine((int)gx, (int)map_content_start_y, (int)gx, (int)(map_content_start_y + max_map_h), ColorAlpha(theme->gutter_num, 0.05f * alpha));
    }

    if (doc->line_count == 0) return;
    float line_scale = fminf(max_map_h / (float)doc->line_count, 3.5f * s);
    float map_content_h = (float)doc->line_count * line_scale;

    // 3. Radar Sweep Effect
    float time_sec = (float)GetTime();
    float sweep_speed = 120.0f * s;
    float sweep_y = fmodf(time_sec * sweep_speed, map_content_h + 80.0f * s);
    float actual_sweep_y = map_content_start_y + sweep_y;

    // 4. Data Line Rendering (Illuminated by Radar)
    for (size_t r = 0; r < doc->line_count; ++r) {
        float line_w = fminf((float)doc->lines[r].size * 1.5f * s, mm_w - 16.0f * s);
        if (line_w <= 0.0f) continue;

        float ly = map_content_start_y + (float)r * line_scale;
        Color base_col = (r == doc->cursor_row) ? theme->cursor : ColorAlpha(theme->syn_default, 0.30f);

        float dist_to_sweep = ly - actual_sweep_y;
        if (dist_to_sweep < 0.0f && dist_to_sweep > -60.0f * s) {
            float intensity = 1.0f - (fabsf(dist_to_sweep) / (60.0f * s));

            // Additive radar boost
            base_col.r = (unsigned char)fminf(base_col.r + theme->syn_keyword.r * intensity, 255.0f);
            base_col.g = (unsigned char)fminf(base_col.g + theme->syn_keyword.g * intensity, 255.0f);
            base_col.b = (unsigned char)fminf(base_col.b + theme->syn_keyword.b * intensity, 255.0f);
            base_col.a = (unsigned char)fminf(base_col.a + 255.0f * intensity, 255.0f);
        }

        Color draw_col = ColorAlpha(base_col, (base_col.a / 255.0f) * alpha);
        DrawRectangle((int)(mm_x + 8.0f * s), (int)ly, (int)line_w, (int)fmaxf(line_scale - 1.0f, 1.0f), draw_col);
    }

    // 5. Radar Laser Line & Gradient Trail
    if (actual_sweep_y >= map_content_start_y && actual_sweep_y <= map_content_start_y + map_content_h) {
        DrawLine((int)mm_x + 2, (int)actual_sweep_y, (int)(mm_x + mm_w - 2), (int)actual_sweep_y, ColorAlpha(theme->cursor, 0.9f * alpha));

        Rectangle trail = { mm_x + 2, actual_sweep_y - 25.0f * s, mm_w - 4, 25.0f * s };
        if (trail.y < map_content_start_y) {
            trail.height -= (map_content_start_y - trail.y);
            trail.y = map_content_start_y;
        }
        if (trail.height > 0) {
            DrawRectangleGradientV((int)trail.x, (int)trail.y, (int)trail.width, (int)trail.height, BLANK, ColorAlpha(theme->cursor, 0.2f * alpha));
        }
    }

    // 6. Viewport Reticle
    float total_code_h = (float)doc->line_count * line_height;
    if (total_code_h > 0.0f) {
        float vp_top = (camera.target.y - ((float)screen_h * 0.5f) / camera.zoom) / total_code_h;
        float vp_h = ((float)screen_h / camera.zoom) / total_code_h;

        float box_y = map_content_start_y + vp_top * map_content_h;
        float box_h = vp_h * map_content_h;
        float box_x = mm_x + 4.0f * s;
        float box_w = mm_w - 8.0f * s;

        if (box_y < map_content_start_y) {
            box_h -= (map_content_start_y - box_y);
            box_y = map_content_start_y;
        }
        if (box_y + box_h > map_content_start_y + map_content_h) {
            box_h = (map_content_start_y + map_content_h) - box_y;
        }

        if (box_h > 4.0f * s) {
            float t_len = 8.0f * s;
            float t_thk = 2.0f * s;
            if (t_len > box_w * 0.5f) t_len = box_w * 0.5f;
            if (t_len > box_h * 0.5f) t_len = box_h * 0.5f;

            Color reticle_col = ColorAlpha(theme->cursor, 0.90f * alpha);
            DrawRectangle((int)box_x, (int)box_y, (int)t_len, (int)t_thk, reticle_col);
            DrawRectangle((int)box_x, (int)box_y, (int)t_thk, (int)t_len, reticle_col);
            DrawRectangle((int)(box_x + box_w - t_len), (int)box_y, (int)t_len, (int)t_thk, reticle_col);
            DrawRectangle((int)(box_x + box_w - t_thk), (int)box_y, (int)t_thk, (int)t_len, reticle_col);
            DrawRectangle((int)box_x, (int)(box_y + box_h - t_thk), (int)t_len, (int)t_thk, reticle_col);
            DrawRectangle((int)box_x, (int)(box_y + box_h - t_len), (int)t_thk, (int)t_len, reticle_col);
            DrawRectangle((int)(box_x + box_w - t_len), (int)(box_y + box_h - t_thk), (int)t_len, (int)t_thk, reticle_col);
            DrawRectangle((int)(box_x + box_w - t_thk), (int)(box_y + box_h - t_len), (int)t_thk, (int)t_len, reticle_col);
        }
    }

    // 7. Active Line Execution Laser
    if (doc->cursor_row < doc->line_count) {
        float cursor_y_ratio = (float)doc->cursor_row / (float)doc->line_count;
        float laser_y = map_content_start_y + (cursor_y_ratio * map_content_h);

        float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 8.0f);

        DrawLine((int)mm_x, (int)laser_y, (int)(mm_x + mm_w), (int)laser_y, ColorAlpha(theme->cursor, 0.4f * pulse * alpha));
        DrawLine((int)(mm_x - 14.0f * s), (int)laser_y, (int)mm_x, (int)laser_y, ColorAlpha(theme->cursor, 0.9f * alpha));
        DrawRectangle((int)(mm_x - 16.0f * s), (int)(laser_y - 2.0f * s), (int)(4.0f * s), (int)(5.0f * s), ColorAlpha(theme->cursor, alpha));

        char depth_str[16];
        snprintf(depth_str, sizeof(depth_str), "%04zu", doc->cursor_row + 1);
        Vector2 t_sz = MeasureTextEx(font_body, depth_str, 9.0f * s, 1.0f);
        DrawTextEx(font_body, depth_str, (Vector2){ mm_x - 20.0f * s - t_sz.x, laser_y - 4.5f * s }, 9.0f * s, 1.0f, ColorAlpha(theme->cursor, 0.85f * alpha));
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

    DrawRectangle(0, 0, screen_w, screen_h, ColorAlpha(theme->bg, 0.94f));

    for (int y = 0; y < screen_h; y += (int)(40.0f * s)) {
        DrawLine(0, y, screen_w, y, ColorAlpha(theme->gutter_num, 0.05f));
    }
    for (int x = 0; x < screen_w; x += (int)(40.0f * s)) {
        DrawLine(x, 0, x, screen_h, ColorAlpha(theme->gutter_num, 0.05f));
    }

    Rectangle h_rect = Canvas_GetRect(&canvas, ANCHOR_CENTER, 860.0f, 520.0f, Canvas_MarginZero());
    h_rect = Canvas_ClampRect(&canvas, h_rect);

    DrawRectangleRounded(h_rect, 0.03f, 4, ColorAlpha(theme->menu_bg, 0.90f));
    DrawRectangleRoundedLines(h_rect, 0.03f, 4, ColorAlpha(theme->cursor, 0.6f));

    float t_len = 24.0f * s;
    float t_thk = 3.0f * s;
    DrawRectangle((int)h_rect.x, (int)h_rect.y, (int)t_len, (int)t_thk, theme->cursor);
    DrawRectangle((int)h_rect.x, (int)h_rect.y, (int)t_thk, (int)t_len, theme->cursor);
    DrawRectangle((int)(h_rect.x + h_rect.width - t_len), (int)h_rect.y, (int)t_len, (int)t_thk, theme->cursor);
    DrawRectangle((int)(h_rect.x + h_rect.width - t_thk), (int)h_rect.y, (int)t_thk, (int)t_len, theme->cursor);
    DrawRectangle((int)h_rect.x, (int)(h_rect.y + h_rect.height - t_thk), (int)t_len, (int)t_thk, theme->cursor);
    DrawRectangle((int)h_rect.x, (int)(h_rect.y + h_rect.height - t_len), (int)t_thk, (int)t_len, theme->cursor);
    DrawRectangle((int)(h_rect.x + h_rect.width - t_len), (int)(h_rect.y + h_rect.height - t_thk), (int)t_len, (int)t_thk, theme->cursor);
    DrawRectangle((int)(h_rect.x + h_rect.width - t_thk), (int)(h_rect.y + h_rect.height - t_len), (int)t_thk, (int)t_len, theme->cursor);

    DrawTextEx(font_body, "/// S Y S T E M   C O N T R O L S", (Vector2){ h_rect.x + 30.0f * s, h_rect.y + 24.0f * s }, 18.0f * s, 2.0f, theme->cursor);
    DrawLine((int)(h_rect.x + 30.0f * s), (int)(h_rect.y + 54.0f * s), (int)(h_rect.x + h_rect.width - 30.0f * s), (int)(h_rect.y + 54.0f * s), ColorAlpha(theme->cursor, 0.3f));

    const char *col1_keys[] = {
        "Ctrl + Z", "Ctrl + Y", "Ctrl + C / X / V", "Ctrl + A", "Ctrl + D",
        "Ctrl + Left / Right", "Ctrl + Backspace", "Ctrl + S", "Ctrl + P", "Ctrl + B"
    };
    const char *col1_desc[] = {
        "Undo Action", "Redo Action", "Copy / Cut / Paste", "Select All", "Duplicate Line",
        "Word Navigation", "Delete Entire Word", "Save Datacore", "Terminal Command Override", "Mission Select Datapad"
    };

    const char *col2_keys[] = {
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "Ctrl + Plus/Minus", "Mouse Drag"
    };
    const char *col2_desc[] = {
        "System Controls", "Toggle CRT Overdrive", "Toggle Spotlight", "Cycle Visual Theme",
        "Cam: Script Fit Mode", "Cam: Cursor Focus Mode", "Cam: Line Focus Mode", "Cycle UI Scaling",
        "Adjust Camera Zoom", "Select Code Blocks"
    };

    float col1_x = h_rect.x + 40.0f * s;
    float col2_x = h_rect.x + h_rect.width * 0.5f + 20.0f * s;
    float start_y = h_rect.y + 80.0f * s;
    float row_h = 32.0f * s;

    for (int i = 0; i < 10; ++i) {
        DrawTextEx(font_body, col1_keys[i], (Vector2){ col1_x, start_y + (float)i * row_h }, 13.0f * s, 1.0f, theme->syn_keyword);
        DrawTextEx(font_body, col1_desc[i], (Vector2){ col1_x + 160.0f * s, start_y + (float)i * row_h }, 13.0f * s, 1.0f, theme->syn_default);
        DrawTextEx(font_body, col2_keys[i], (Vector2){ col2_x, start_y + (float)i * row_h }, 13.0f * s, 1.0f, theme->syn_keyword);
        DrawTextEx(font_body, col2_desc[i], (Vector2){ col2_x + 160.0f * s, start_y + (float)i * row_h }, 13.0f * s, 1.0f, theme->syn_default);
    }

    float footer_y = h_rect.y + h_rect.height - 40.0f * s;
    DrawLine((int)(h_rect.x + 30.0f * s), (int)(footer_y - 10.0f * s), (int)(h_rect.x + h_rect.width - 30.0f * s), (int)(footer_y - 10.0f * s), ColorAlpha(theme->cursor, 0.3f));

    bool blink = ((int)(GetTime() * 2.0)) % 2 == 0;
    if (blink) {
        const char *msg = "[ ESC ] TO DISENGAGE";
        Vector2 msg_sz = MeasureTextEx(font_body, msg, 14.0f * s, 2.0f);
        DrawTextEx(font_body, msg, (Vector2){ h_rect.x + (h_rect.width - msg_sz.x) * 0.5f, footer_y + 4.0f * s }, 14.0f * s, 2.0f, theme->cursor);
    }
}

void UI_DrawStatusBar(int screen_h, float scale, CCameraMode cam_mode, float zoom, float user_zoom_mult, const Theme *theme, Font font_body, float alpha) {
    if (!theme || alpha <= 0.01f) return;
    int screen_w = GetScreenWidth();
    UICanvas canvas = Canvas_Create(screen_w, screen_h, scale);
    float s = canvas.scale;

    UIMargins sb_margins = Canvas_Margins(16.0f, 0.0f, 16.0f, 10.0f);
    Rectangle bar_rect = Canvas_GetRect(&canvas, ANCHOR_BOTTOM_FILL, 0.0f, 26.0f, sb_margins);

    DrawRectangleRounded(bar_rect, 0.25f, 6, ColorAlpha(theme->status_bg, alpha));
    DrawRectangleRoundedLines(bar_rect, 0.25f, 6, ColorAlpha(theme->gutter_num, 0.45f * alpha));

    const char *cam_names[] = { "Script Fit", "Cursor Focus", "Line Focus" };
    char stats[256];
    snprintf(stats, sizeof(stats),
             "%s | Cam: %s | UI: %.0f%% | Theme: %s | Zoom: %.2fx (User: %.0f%%) | F1: Controls",
             CE_APP_NAME_SHORT, cam_names[cam_mode], scale * 100.0f, theme->name, zoom, user_zoom_mult * 100.0f);

    float font_size = 13.0f * s;
    float text_y = bar_rect.y + (bar_rect.height - font_size) * 0.5f;
    DrawTextEx(font_body, stats, (Vector2){ bar_rect.x + 16.0f * s, text_y }, font_size, 1.0f, ColorAlpha(theme->status_text, alpha));
}