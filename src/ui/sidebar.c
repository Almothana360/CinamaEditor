#include "ui/sidebar.h"
#include "ui/menu_bar.h"
#include "render/canvas.h"
#include "core/event.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static Workspace *g_sb_ws = NULL;
static Document *g_sb_doc = NULL;

static bool g_sb_open = true;
static float g_sb_scroll_y = 0.0f;
static int g_sb_hovered_idx = -1;

// Inline creation state: 0 = none, 1 = new file, 2 = new folder
static int g_sb_create_mode = 0;
static char g_sb_input_buf[64] = {0};
static int g_sb_input_len = 0;

static float Sidebar_GetEffectiveScale(int screen_w, float user_scale) {
    int screen_h = GetScreenHeight();
    float sw = (screen_w > 0) ? (float)screen_w : 1280.0f;
    float sh = (screen_h > 0) ? (float)screen_h : 720.0f;
    float u = (user_scale > 0.1f) ? user_scale : 1.0f;
    float res_scale = fminf(sw / 1280.0f, sh / 720.0f);
    if (res_scale < 0.75f) res_scale = 0.75f;
    return u * res_scale;
}

void Sidebar_Init(Workspace *ws, Document *doc) {
    g_sb_ws = ws;
    g_sb_doc = doc;
    g_sb_open = true;
    g_sb_scroll_y = 0.0f;
    g_sb_hovered_idx = -1;
    g_sb_create_mode = 0;
    g_sb_input_buf[0] = '\0';
    g_sb_input_len = 0;
}

void Sidebar_Toggle(void) {
    g_sb_open = !g_sb_open;
    if (!g_sb_open) {
        g_sb_create_mode = 0;
    }
}

bool Sidebar_IsOpen(void) {
    return g_sb_open;
}

void Sidebar_SetOpen(bool open) {
    g_sb_open = open;
    if (!g_sb_open) {
        g_sb_create_mode = 0;
    }
}

float Sidebar_GetWidth(float scale) {
    if (!g_sb_open) return 0.0f;
    int screen_w = GetScreenWidth();
    float s = Sidebar_GetEffectiveScale(screen_w, scale);
    return 240.0f * s + 16.0f * s; // Panel width + left margin
}

static Rectangle Sidebar_GetContainerRect(int screen_w, int screen_h, float s) {
    float margin_left = 16.0f * s;
    float top_y = MenuBar_GetHeight(s) + 10.0f * s;
    float bottom_margin = 10.0f * s + 26.0f * s + 10.0f * s; // Status bar clearance
    float height = (float)screen_h - top_y - bottom_margin;
    float width = 240.0f * s;

    if (height < 100.0f * s) height = 100.0f * s;

    return (Rectangle){
        .x = margin_left,
        .y = top_y,
        .width = width,
        .height = height
    };
}

bool Sidebar_ContainsPoint(Vector2 point, int screen_w, int screen_h, float scale) {
    if (!g_sb_open) return false;
    float s = Sidebar_GetEffectiveScale(screen_w, scale);
    Rectangle rect = Sidebar_GetContainerRect(screen_w, screen_h, s);
    return CheckCollisionPointRec(point, rect);
}

bool Sidebar_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale) {
    // Global hotkey: Ctrl+B to toggle sidebar
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrl && IsKeyPressed(KEY_B)) {
        Sidebar_Toggle();
        return true;
    }

    if (!g_sb_open || !g_sb_ws) return false;

    float s = Sidebar_GetEffectiveScale(screen_w, scale);
    Rectangle panel = Sidebar_GetContainerRect(screen_w, screen_h, s);
    bool mouse_in_panel = CheckCollisionPointRec(mouse_screen, panel);

    // Mouse wheel scrolling
    if (mouse_in_panel) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && g_sb_create_mode == 0) {
            g_sb_scroll_y += wheel * 28.0f * s;
            if (g_sb_scroll_y > 0.0f) g_sb_scroll_y = 0.0f;
        }
    }

    // Inline file/folder creation handling
    if (g_sb_create_mode != 0) {
        int ch = GetCharPressed();
        while (ch > 0) {
            if ((ch >= 32 && ch <= 126) && ch != '/' && ch != '\\' && ch != ':' && ch != '*' && ch != '?' && ch != '"' && ch != '<' && ch != '>' && ch != '|') {
                if (g_sb_input_len < (int)sizeof(g_sb_input_buf) - 2) {
                    g_sb_input_buf[g_sb_input_len++] = (char)ch;
                    g_sb_input_buf[g_sb_input_len] = '\0';
                }
            }
            ch = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (g_sb_input_len > 0) {
                g_sb_input_buf[--g_sb_input_len] = '\0';
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (g_sb_input_len > 0) {
                if (g_sb_create_mode == 1) {
                    if (Workspace_CreateFile(g_sb_ws, g_sb_input_buf)) {
                        Workspace_OpenFile(g_sb_ws, g_sb_doc, g_sb_input_buf);
                    }
                } else if (g_sb_create_mode == 2) {
                    Workspace_CreateFolder(g_sb_ws, g_sb_input_buf);
                }
            }
            g_sb_create_mode = 0;
            g_sb_input_len = 0;
            g_sb_input_buf[0] = '\0';
            return true;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            g_sb_create_mode = 0;
            g_sb_input_len = 0;
            g_sb_input_buf[0] = '\0';
            return true;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mouse_in_panel) {
            g_sb_create_mode = 0;
            return true;
        }

        return mouse_in_panel;
    }

    // Header buttons (New File, New Folder, Refresh)
    float header_h = 56.0f * s;
    float btn_w = 46.0f * s;
    float btn_h = 22.0f * s;
    float btn_y = panel.y + 28.0f * s;

    Rectangle btn_file = { panel.x + 10.0f * s, btn_y, btn_w + 14.0f * s, btn_h };
    Rectangle btn_folder = { btn_file.x + btn_file.width + 6.0f * s, btn_y, btn_w + 18.0f * s, btn_h };
    Rectangle btn_refresh = { btn_folder.x + btn_folder.width + 6.0f * s, btn_y, 26.0f * s, btn_h };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse_screen, btn_file)) {
            g_sb_create_mode = 1; // Create file
            g_sb_input_len = 0;
            g_sb_input_buf[0] = '\0';
            return true;
        }
        if (CheckCollisionPointRec(mouse_screen, btn_folder)) {
            g_sb_create_mode = 2; // Create folder
            g_sb_input_len = 0;
            g_sb_input_buf[0] = '\0';
            return true;
        }
        if (CheckCollisionPointRec(mouse_screen, btn_refresh)) {
            Workspace_Refresh(g_sb_ws);
            return true;
        }
    }

    // Tree entry interaction
    g_sb_hovered_idx = -1;
    float content_y = panel.y + header_h;
    float content_h = panel.height - header_h - 10.0f * s;
    Rectangle content_rect = { panel.x, content_y, panel.width, content_h };

    if (CheckCollisionPointRec(mouse_screen, content_rect)) {
        float row_h = 24.0f * s;
        float cur_y = content_y + g_sb_scroll_y;

        int count = 0;
        const WorkspaceEntry *entries = Workspace_GetEntries(g_sb_ws, &count);

        for (int i = 0; i < count; ++i) {
            if (entries[i].is_hidden) continue;

            Rectangle row_rect = { panel.x + 4.0f * s, cur_y, panel.width - 8.0f * s, row_h };
            if (CheckCollisionPointRec(mouse_screen, row_rect)) {
                g_sb_hovered_idx = i;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (entries[i].is_directory) {
                        Workspace_ToggleFolder(g_sb_ws, i);
                    } else {
                        Workspace_OpenFileIndex(g_sb_ws, g_sb_doc, i);
                    }
                    return true;
                }
                break;
            }
            cur_y += row_h;
        }
    }

    return mouse_in_panel;
}

void Sidebar_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    if (!g_sb_open || !theme || !g_sb_ws) return;

    float s = Sidebar_GetEffectiveScale(screen_w, scale);
    Rectangle panel = Sidebar_GetContainerRect(screen_w, screen_h, s);

    // 1. Draw floating panel background and border (Rounded Rectangle)
    DrawRectangleRounded(panel, 0.04f, 4, theme->menu_bg);
    DrawRectangleRoundedLines(panel, 0.04f, 4, theme->menu_border);

    // 2. Header: Workspace root title
    float title_font_size = 13.0f * s;
    const char *ws_name = Workspace_GetName(g_sb_ws);
    char header_str[128];
    snprintf(header_str, sizeof(header_str), "PROJECT: %s", ws_name);
    DrawTextEx(font_body, header_str, (Vector2){ panel.x + 12.0f * s, panel.y + 10.0f * s }, title_font_size, 1.0f, theme->cursor);

    // 3. Action buttons (+ File, + Folder, ↻ Refresh)
    float btn_w = 46.0f * s;
    float btn_h = 20.0f * s;
    float btn_y = panel.y + 28.0f * s;

    Rectangle btn_file = { panel.x + 10.0f * s, btn_y, btn_w + 14.0f * s, btn_h };
    Rectangle btn_folder = { btn_file.x + btn_file.width + 5.0f * s, btn_y, btn_w + 18.0f * s, btn_h };
    Rectangle btn_refresh = { btn_folder.x + btn_folder.width + 5.0f * s, btn_y, 24.0f * s, btn_h };

    Vector2 mpos = GetMousePosition();

    // Button: + File
    bool h_file = CheckCollisionPointRec(mpos, btn_file);
    DrawRectangleRounded(btn_file, 0.20f, 4, h_file ? theme->menu_hl : ColorAlpha(theme->gutter_bg, 0.85f));
    DrawRectangleRoundedLines(btn_file, 0.20f, 4, ColorAlpha(theme->gutter_num, 0.45f));
    DrawTextEx(font_body, "+ File", (Vector2){ btn_file.x + 8.0f * s, btn_file.y + 4.0f * s }, 11.0f * s, 1.0f, h_file ? theme->cursor : theme->syn_default);

    // Button: + Folder
    bool h_folder = CheckCollisionPointRec(mpos, btn_folder);
    DrawRectangleRounded(btn_folder, 0.20f, 4, h_folder ? theme->menu_hl : ColorAlpha(theme->gutter_bg, 0.85f));
    DrawRectangleRoundedLines(btn_folder, 0.20f, 4, ColorAlpha(theme->gutter_num, 0.45f));
    DrawTextEx(font_body, "+ Folder", (Vector2){ btn_folder.x + 6.0f * s, btn_folder.y + 4.0f * s }, 11.0f * s, 1.0f, h_folder ? theme->cursor : theme->syn_default);

    // Button: Refresh
    bool h_ref = CheckCollisionPointRec(mpos, btn_refresh);
    DrawRectangleRounded(btn_refresh, 0.20f, 4, h_ref ? theme->menu_hl : ColorAlpha(theme->gutter_bg, 0.85f));
    DrawRectangleRoundedLines(btn_refresh, 0.20f, 4, ColorAlpha(theme->gutter_num, 0.45f));
    DrawTextEx(font_body, "*", (Vector2){ btn_refresh.x + 8.0f * s, btn_refresh.y + 2.0f * s }, 13.0f * s, 1.0f, h_ref ? theme->cursor : theme->gutter_num);

    // Divider line below header
    float header_divider_y = panel.y + 54.0f * s;
    DrawLine((int)(panel.x + 8.0f * s), (int)header_divider_y, (int)(panel.x + panel.width - 8.0f * s), (int)header_divider_y, ColorAlpha(theme->gutter_num, 0.35f));

    // 4. File tree list with scissor clipping
    float content_y = header_divider_y + 4.0f * s;
    float content_h = panel.y + panel.height - content_y - 8.0f * s;
    Rectangle scissor_rect = { panel.x + 2.0f * s, content_y, panel.width - 4.0f * s, content_h };

    if (scissor_rect.width > 0 && scissor_rect.height > 0) {
        BeginScissorMode((int)scissor_rect.x, (int)scissor_rect.y, (int)scissor_rect.width, (int)scissor_rect.height);

        float row_h = 24.0f * s;
        float cur_y = content_y + g_sb_scroll_y;

        // Draw inline creation row if active
        if (g_sb_create_mode != 0) {
            Rectangle input_rect = { panel.x + 8.0f * s, cur_y, panel.width - 16.0f * s, 24.0f * s };
            DrawRectangleRounded(input_rect, 0.15f, 4, theme->menu_hl);
            DrawRectangleRoundedLines(input_rect, 0.15f, 4, theme->cursor);

            char prompt_text[128];
            snprintf(prompt_text, sizeof(prompt_text), "%s %s_", (g_sb_create_mode == 1 ? "FILE:" : "DIR:"), g_sb_input_buf);
            DrawTextEx(font_body, prompt_text, (Vector2){ input_rect.x + 6.0f * s, input_rect.y + 5.0f * s }, 12.0f * s, 1.0f, theme->cursor);

            cur_y += row_h + 4.0f * s;
        }

        int count = 0;
        const WorkspaceEntry *entries = Workspace_GetEntries(g_sb_ws, &count);

        for (int i = 0; i < count; ++i) {
            if (entries[i].is_hidden) continue;

            Rectangle row_rect = { panel.x + 6.0f * s, cur_y, panel.width - 12.0f * s, row_h };
            bool is_hovered = (g_sb_hovered_idx == i);
            bool is_active = (g_sb_ws->active_file_idx == i);

            if (is_active) {
                DrawRectangleRounded(row_rect, 0.18f, 4, theme->menu_hl);
                DrawRectangle((int)row_rect.x, (int)row_rect.y, (int)(3.0f * s), (int)row_rect.height, theme->cursor);
            } else if (is_hovered) {
                DrawRectangleRounded(row_rect, 0.18f, 4, ColorAlpha(theme->menu_hl, 0.60f));
            }

            float indent = (float)entries[i].depth * 13.0f * s;
            float text_x = row_rect.x + 8.0f * s + indent;
            float text_y = cur_y + 5.0f * s;

            char display_line[128];
            Color text_color = is_active ? theme->cursor : (is_hovered ? theme->syn_default : ColorAlpha(theme->syn_default, 0.85f));

            if (entries[i].is_directory) {
                const char *folder_icon = entries[i].is_expanded ? "[-] " : "[+] ";
                snprintf(display_line, sizeof(display_line), "%s%s", folder_icon, entries[i].name);
                text_color = is_active ? theme->cursor : (entries[i].is_expanded ? theme->syn_type : theme->syn_keyword);
            } else {
                snprintf(display_line, sizeof(display_line), "- %s", entries[i].name);
            }

            DrawTextEx(font_body, display_line, (Vector2){ text_x, text_y }, 12.0f * s, 1.0f, text_color);
            cur_y += row_h;
        }

        EndScissorMode();
    }
}