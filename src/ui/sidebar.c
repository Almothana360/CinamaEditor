#include "ui/sidebar.h"
#include "core/event.h"
#include "fx/audio.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static Workspace *g_sb_ws = NULL;
static Document *g_sb_doc = NULL;

static bool g_sb_open = false;
static bool g_sb_active = false;
static float g_sb_anim = 0.0f;
static float g_sb_vel = 0.0f;

// Filter and Navigation State
static char g_sb_search[64] = {0};
static int g_sb_search_len = 0;
static int g_sb_selected_sector = -1; // -1 = ALL SECTORS
static int g_sb_hovered_file = -1;
static float g_sb_file_scroll = 0.0f;
static float g_sb_sector_scroll = 0.0f;

// Inline creation mode: 0 = none, 1 = new datacore (file), 2 = new sector (folder)
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

static const char *GetLanguageBadge(const char *filename) {
    if (!filename) return "[DATA]";
    const char *dot = strrchr(filename, '.');
    if (!dot) return "[RAW]";
    if (strcmp(dot, ".c") == 0) return "[C-SRC]";
    if (strcmp(dot, ".h") == 0) return "[HEADER]";
    if (strcmp(dot, ".cpp") == 0 || strcmp(dot, ".hpp") == 0 || strcmp(dot, ".cc") == 0) return "[C++]";
    if (strcmp(dot, ".py") == 0) return "[PYTHON]";
    if (strcmp(dot, ".js") == 0 || strcmp(dot, ".ts") == 0) return "[SCRIPT]";
    if (strcmp(dot, ".json") == 0) return "[CONFIG]";
    if (strcmp(dot, ".glsl") == 0 || strcmp(dot, ".vs") == 0 || strcmp(dot, ".fs") == 0) return "[SHADER]";
    if (strcmp(dot, ".txt") == 0 || strcmp(dot, ".md") == 0) return "[LOG]";
    return "[DATA]";
}

static bool MatchesFilter(const char *name, const char *path, const char *query) {
    if (!query || query[0] == '\0') return true;
    char q_lower[64];
    size_t q_len = strlen(query);
    if (q_len >= sizeof(q_lower)) q_len = sizeof(q_lower) - 1;
    for (size_t i = 0; i < q_len; ++i) {
        q_lower[i] = (char)tolower((unsigned char)query[i]);
    }
    q_lower[q_len] = '\0';

    char target[256];
    snprintf(target, sizeof(target), "%s %s", name ? name : "", path ? path : "");
    for (char *p = target; *p; ++p) {
        *p = (char)tolower((unsigned char)*p);
    }

    return (strstr(target, q_lower) != NULL);
}

void Sidebar_Init(Workspace *ws, Document *doc) {
    g_sb_ws = ws;
    g_sb_doc = doc;
    g_sb_open = false;
    g_sb_active = false;
    g_sb_anim = 0.0f;
    g_sb_vel = 0.0f;
    g_sb_search[0] = '\0';
    g_sb_search_len = 0;
    g_sb_selected_sector = -1;
    g_sb_hovered_file = -1;
    g_sb_file_scroll = 0.0f;
    g_sb_sector_scroll = 0.0f;
    g_sb_create_mode = 0;
    g_sb_input_buf[0] = '\0';
    g_sb_input_len = 0;
}

void Sidebar_Open(void) {
    if (!g_sb_open) {
        g_sb_open = true;
        g_sb_active = true;
        g_sb_search[0] = '\0';
        g_sb_search_len = 0;
        g_sb_create_mode = 0;
        g_sb_file_scroll = 0.0f;
        Audio_PlayUI(SND_UI_OPEN);
    }
}

void Sidebar_Close(void) {
    if (g_sb_open) {
        g_sb_open = false;
        g_sb_create_mode = 0;
        g_sb_input_len = 0;
        g_sb_input_buf[0] = '\0';
        Audio_PlayUI(SND_UI_CLOSE);
    }
}

void Sidebar_Toggle(void) {
    if (g_sb_open) Sidebar_Close();
    else Sidebar_Open();
}

bool Sidebar_IsOpen(void) {
    return g_sb_active;
}

void Sidebar_SetOpen(bool open) {
    if (open) Sidebar_Open();
    else Sidebar_Close();
}

float Sidebar_GetWidth(float scale) {
    (void)scale;
    return 0.0f;
}

static Rectangle Sidebar_GetDatapadRect(int screen_w, int screen_h, float s) {
    float w = fminf((float)screen_w * 0.86f, 1080.0f * s);
    float h = fminf((float)screen_h * 0.82f, 680.0f * s);
    float x = ((float)screen_w - w) * 0.5f;
    float y = ((float)screen_h - h) * 0.5f - 10.0f * s;
    return (Rectangle){ x, y, w, h };
}

bool Sidebar_ContainsPoint(Vector2 point, int screen_w, int screen_h, float scale) {
    if (!g_sb_active) return false;
    float s = Sidebar_GetEffectiveScale(screen_w, scale);
    Rectangle rect = Sidebar_GetDatapadRect(screen_w, screen_h, s);
    return CheckCollisionPointRec(point, rect);
}

bool Sidebar_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale) {
    // Phase 5: Datapad Spring Physics
    float dt = GetFrameTime();
    float target = g_sb_open ? 1.0f : 0.0f;
    float tension = 280.0f;
    float damping = 2.0f * sqrtf(tension) * 0.85f;

    g_sb_vel += (target - g_sb_anim) * tension * dt;
    g_sb_vel -= g_sb_vel * damping * dt;
    g_sb_anim += g_sb_vel * dt;

    if (!g_sb_open && g_sb_anim < 0.005f) {
        g_sb_active = false;
        g_sb_anim = 0.0f;
    }

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    if (ctrl && IsKeyPressed(KEY_B)) {
        Sidebar_Toggle();
        return true;
    }

    if (!g_sb_active) return false;
    if (!g_sb_open) return true;

    float s = Sidebar_GetEffectiveScale(screen_w, scale);
    Rectangle pad = Sidebar_GetDatapadRect(screen_w, screen_h, s);

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
            if (g_sb_input_len > 0) g_sb_input_buf[--g_sb_input_len] = '\0';
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (g_sb_input_len > 0) {
                if (g_sb_create_mode == 1) {
                    if (Workspace_CreateFile(g_sb_ws, g_sb_input_buf)) {
                        Audio_PlayUI(SND_UI_SELECT);
                        Workspace_OpenFile(g_sb_ws, g_sb_doc, g_sb_input_buf);
                        Sidebar_Close();
                        return true;
                    }
                } else if (g_sb_create_mode == 2) {
                    Audio_PlayUI(SND_UI_SELECT);
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
        return true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        Sidebar_Close();
        return true;
    }

    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126) {
            if (g_sb_search_len < (int)sizeof(g_sb_search) - 2) {
                g_sb_search[g_sb_search_len++] = (char)ch;
                g_sb_search[g_sb_search_len] = '\0';
                g_sb_file_scroll = 0.0f;
            }
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (g_sb_search_len > 0) {
            g_sb_search[--g_sb_search_len] = '\0';
            g_sb_file_scroll = 0.0f;
        }
    }

    float header_h = 75.0f * s;
    float footer_h = 32.0f * s;
    float content_y = pad.y + header_h;
    float content_h = pad.height - header_h - footer_h;
    float sector_col_w = pad.width * 0.30f;
    float file_col_w = pad.width - sector_col_w;

    Rectangle sector_deck = { pad.x, content_y, sector_col_w, content_h };
    Rectangle file_deck = { pad.x + sector_col_w, content_y, file_col_w, content_h };

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        if (CheckCollisionPointRec(mouse_screen, sector_deck)) {
            g_sb_sector_scroll += wheel * 32.0f * s;
            if (g_sb_sector_scroll > 0.0f) g_sb_sector_scroll = 0.0f;
        } else if (CheckCollisionPointRec(mouse_screen, file_deck)) {
            g_sb_file_scroll += wheel * 36.0f * s;
            if (g_sb_file_scroll > 0.0f) g_sb_file_scroll = 0.0f;
        }
    }

    float btn_w = 88.0f * s;
    float btn_h = 24.0f * s;
    float btn_y = pad.y + 44.0f * s;

    Rectangle btn_file = { pad.x + pad.width - (btn_w * 2.0f + 40.0f * s), btn_y, btn_w, btn_h };
    Rectangle btn_folder = { btn_file.x + btn_w + 6.0f * s, btn_y, btn_w, btn_h };
    Rectangle btn_ref = { btn_folder.x + btn_w + 6.0f * s, btn_y, 26.0f * s, btn_h };

    static int last_hovered_btn = 0;
    int hovered_btn = 0;

    if (CheckCollisionPointRec(mouse_screen, btn_file)) hovered_btn = 1;
    if (CheckCollisionPointRec(mouse_screen, btn_folder)) hovered_btn = 2;
    if (CheckCollisionPointRec(mouse_screen, btn_ref)) hovered_btn = 3;

    if (hovered_btn != last_hovered_btn && hovered_btn != 0) {
        Audio_PlayUI(SND_UI_HOVER);
    }
    last_hovered_btn = hovered_btn;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hovered_btn == 1) {
            Audio_PlayUI(SND_UI_SELECT);
            g_sb_create_mode = 1;
            g_sb_input_len = 0;
            g_sb_input_buf[0] = '\0';
            return true;
        }
        if (hovered_btn == 2) {
            Audio_PlayUI(SND_UI_SELECT);
            g_sb_create_mode = 2;
            g_sb_input_len = 0;
            g_sb_input_buf[0] = '\0';
            return true;
        }
        if (hovered_btn == 3) {
            Audio_PlayUI(SND_UI_SELECT);
            Workspace_Refresh(g_sb_ws);
            return true;
        }
    }

    int count = 0;
    const WorkspaceEntry *entries = Workspace_GetEntries(g_sb_ws, &count);

    int last_hovered_file = g_sb_hovered_file;
    g_sb_hovered_file = -1;

    static int last_hovered_sector = -2;
    int hovered_sector = -2;

    if (CheckCollisionPointRec(mouse_screen, sector_deck)) {
        float row_h = 28.0f * s;
        float cur_y = content_y + 10.0f * s + g_sb_sector_scroll;

        Rectangle all_row = { pad.x + 8.0f * s, cur_y, sector_col_w - 16.0f * s, row_h };
        if (CheckCollisionPointRec(mouse_screen, all_row)) {
            hovered_sector = -1;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Audio_PlayUI(SND_UI_SELECT);
                g_sb_selected_sector = -1;
                g_sb_file_scroll = 0.0f;
                return true;
            }
        }
        cur_y += row_h + 4.0f * s;

        for (int i = 0; i < count; ++i) {
            if (!entries[i].is_directory) continue;
            Rectangle row = { pad.x + 8.0f * s, cur_y, sector_col_w - 16.0f * s, row_h };
            if (CheckCollisionPointRec(mouse_screen, row)) {
                hovered_sector = i;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Audio_PlayUI(SND_UI_SELECT);
                    g_sb_selected_sector = i;
                    g_sb_file_scroll = 0.0f;
                    return true;
                }
            }
            cur_y += row_h + 3.0f * s;
        }
    }

    if (hovered_sector != last_hovered_sector && hovered_sector != -2) {
        Audio_PlayUI(SND_UI_HOVER);
    }
    last_hovered_sector = hovered_sector;

    if (CheckCollisionPointRec(mouse_screen, file_deck)) {
        float card_h = 38.0f * s;
        float cur_y = content_y + 10.0f * s + g_sb_file_scroll;

        const char *selected_prefix = (g_sb_selected_sector >= 0 && g_sb_selected_sector < count) ? entries[g_sb_selected_sector].path : NULL;

        for (int i = 0; i < count; ++i) {
            if (entries[i].is_directory) continue;
            if (selected_prefix && strncmp(entries[i].path, selected_prefix, strlen(selected_prefix)) != 0) continue;
            if (!MatchesFilter(entries[i].name, entries[i].rel_path, g_sb_search)) continue;

            Rectangle card = { file_deck.x + 10.0f * s, cur_y, file_deck.width - 20.0f * s, card_h };
            if (CheckCollisionPointRec(mouse_screen, card)) {
                g_sb_hovered_file = i;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Audio_PlayUI(SND_UI_SELECT);
                    Workspace_OpenFileIndex(g_sb_ws, g_sb_doc, i);
                    Sidebar_Close();
                    return true;
                }
            }
            cur_y += card_h + 6.0f * s;
        }
    }

    if (g_sb_hovered_file != last_hovered_file && g_sb_hovered_file != -1) {
        Audio_PlayUI(SND_UI_HOVER);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(mouse_screen, pad)) {
        Sidebar_Close();
        return true;
    }

    return true;
}

void Sidebar_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    if (!g_sb_active || !theme || !g_sb_ws) return;

    float s = Sidebar_GetEffectiveScale(screen_w, scale);
    float alpha = g_sb_anim;

    DrawRectangle(0, 0, screen_w, screen_h, ColorAlpha((Color){ 4, 6, 12, 225 }, alpha));
    for (int y = 0; y < screen_h; y += 4) {
        DrawRectangle(0, y, screen_w, 1, ColorAlpha(theme->gutter_num, 0.08f * alpha));
    }

    Rectangle pad = Sidebar_GetDatapadRect(screen_w, screen_h, s);

    // Phase 5: Spring Scale Animation
    float anim_scale = 0.85f + (0.15f * g_sb_anim);
    float w_diff = pad.width * (1.0f - anim_scale);
    float h_diff = pad.height * (1.0f - anim_scale);
    pad.x += w_diff * 0.5f;
    pad.y += h_diff * 0.5f;
    pad.width *= anim_scale;
    pad.height *= anim_scale;

    // Scale everything relative to anim_scale so inner contents scale properly
    s *= anim_scale;

    DrawRectangleRounded((Rectangle){ pad.x + 4.0f * s, pad.y + 6.0f * s, pad.width, pad.height }, 0.03f, 4, ColorAlpha((Color){ 0, 0, 0, 160 }, alpha));
    DrawRectangleRounded(pad, 0.03f, 4, ColorAlpha(theme->menu_bg, alpha));
    DrawRectangleRoundedLines(pad, 0.03f, 4, ColorAlpha(theme->menu_border, alpha));

    float tick_len = 16.0f * s;
    float tick_thk = 3.0f * s;
    Color c_alpha = ColorAlpha(theme->cursor, alpha);
    DrawRectangle((int)pad.x, (int)pad.y, (int)tick_len, (int)tick_thk, c_alpha);
    DrawRectangle((int)pad.x, (int)pad.y, (int)tick_thk, (int)tick_len, c_alpha);
    DrawRectangle((int)(pad.x + pad.width - tick_len), (int)pad.y, (int)tick_len, (int)tick_thk, c_alpha);
    DrawRectangle((int)(pad.x + pad.width - tick_thk), (int)pad.y, (int)tick_thk, (int)tick_len, c_alpha);
    DrawRectangle((int)pad.x, (int)(pad.y + pad.height - tick_thk), (int)tick_len, (int)tick_thk, c_alpha);
    DrawRectangle((int)pad.x, (int)(pad.y + pad.height - tick_len), (int)tick_thk, (int)tick_len, c_alpha);
    DrawRectangle((int)(pad.x + pad.width - tick_len), (int)(pad.y + pad.height - tick_thk), (int)tick_len, (int)tick_thk, c_alpha);
    DrawRectangle((int)(pad.x + pad.width - tick_thk), (int)(pad.y + pad.height - tick_len), (int)tick_thk, (int)tick_len, c_alpha);

    float title_sz = 14.0f * s;
    const char *ws_name = Workspace_GetName(g_sb_ws);
    char title_str[128];
    snprintf(title_str, sizeof(title_str), "/// MISSION SELECT // DATA ARCHIVE // SECTOR: %s", ws_name);
    DrawTextEx(font_body, title_str, (Vector2){ pad.x + 18.0f * s, pad.y + 16.0f * s }, title_sz, 2.0f, c_alpha);

    float search_w = 260.0f * s;
    float search_h = 24.0f * s;
    Rectangle search_box = { pad.x + 18.0f * s, pad.y + 44.0f * s, search_w, search_h };
    DrawRectangleRounded(search_box, 0.20f, 4, ColorAlpha(theme->line_hl, alpha));
    DrawRectangleRoundedLines(search_box, 0.20f, 4, ColorAlpha(theme->gutter_num, 0.5f * alpha));

    char search_prompt[128];
    if (g_sb_search_len == 0) {
        snprintf(search_prompt, sizeof(search_prompt), "FILTER DATACORES: _");
        DrawTextEx(font_body, search_prompt, (Vector2){ search_box.x + 8.0f * s, search_box.y + 5.0f * s }, 11.5f * s, 1.0f, ColorAlpha(theme->gutter_num, 0.75f * alpha));
    } else {
        snprintf(search_prompt, sizeof(search_prompt), "> %s", g_sb_search);
        DrawTextEx(font_body, search_prompt, (Vector2){ search_box.x + 8.0f * s, search_box.y + 5.0f * s }, 11.5f * s, 1.0f, c_alpha);
    }

    float btn_w = 88.0f * s;
    float btn_h = 24.0f * s;
    float btn_y = pad.y + 44.0f * s;

    Rectangle btn_file = { pad.x + pad.width - (btn_w * 2.0f + 40.0f * s), btn_y, btn_w, btn_h };
    Rectangle btn_folder = { btn_file.x + btn_w + 6.0f * s, btn_y, btn_w, btn_h };
    Rectangle btn_ref = { btn_folder.x + btn_w + 6.0f * s, btn_y, 26.0f * s, btn_h };

    Vector2 mpos = GetMousePosition();

    bool h_f = CheckCollisionPointRec(mpos, btn_file);
    DrawRectangleRounded(btn_file, 0.22f, 4, h_f ? ColorAlpha(theme->menu_hl, alpha) : ColorAlpha(theme->gutter_bg, 0.85f * alpha));
    DrawRectangleRoundedLines(btn_file, 0.22f, 4, ColorAlpha(theme->gutter_num, 0.45f * alpha));
    DrawTextEx(font_body, "+ Datacore", (Vector2){ btn_file.x + 8.0f * s, btn_file.y + 5.0f * s }, 11.0f * s, 1.0f, h_f ? c_alpha : ColorAlpha(theme->syn_default, alpha));

    bool h_d = CheckCollisionPointRec(mpos, btn_folder);
    DrawRectangleRounded(btn_folder, 0.22f, 4, h_d ? ColorAlpha(theme->menu_hl, alpha) : ColorAlpha(theme->gutter_bg, 0.85f * alpha));
    DrawRectangleRoundedLines(btn_folder, 0.22f, 4, ColorAlpha(theme->gutter_num, 0.45f * alpha));
    DrawTextEx(font_body, "+ Sector", (Vector2){ btn_folder.x + 12.0f * s, btn_folder.y + 5.0f * s }, 11.0f * s, 1.0f, h_d ? c_alpha : ColorAlpha(theme->syn_default, alpha));

    bool h_r = CheckCollisionPointRec(mpos, btn_ref);
    DrawRectangleRounded(btn_ref, 0.22f, 4, h_r ? ColorAlpha(theme->menu_hl, alpha) : ColorAlpha(theme->gutter_bg, 0.85f * alpha));
    DrawRectangleRoundedLines(btn_ref, 0.22f, 4, ColorAlpha(theme->gutter_num, 0.45f * alpha));
    DrawTextEx(font_body, "*", (Vector2){ btn_ref.x + 9.0f * s, btn_ref.y + 4.0f * s }, 13.0f * s, 1.0f, h_r ? c_alpha : ColorAlpha(theme->gutter_num, alpha));

    float header_div_y = pad.y + 75.0f * s;
    DrawLine((int)(pad.x + 10.0f * s), (int)header_div_y, (int)(pad.x + pad.width - 10.0f * s), (int)header_div_y, ColorAlpha(theme->gutter_num, 0.35f * alpha));

    float footer_h = 32.0f * s;
    float content_y = header_div_y + 1.0f;
    float content_h = pad.height - 75.0f * s - footer_h;
    float sector_col_w = pad.width * 0.30f;
    float file_col_w = pad.width - sector_col_w;

    DrawLine((int)(pad.x + sector_col_w), (int)content_y, (int)(pad.x + sector_col_w), (int)(content_y + content_h), ColorAlpha(theme->gutter_num, 0.25f * alpha));

    int count = 0;
    const WorkspaceEntry *entries = Workspace_GetEntries(g_sb_ws, &count);

    // LEFT DECK: SECTORS
    Rectangle sec_scissor = { pad.x + 4.0f * s, content_y + 4.0f * s, sector_col_w - 8.0f * s, content_h - 8.0f * s };
    if (sec_scissor.width > 0 && sec_scissor.height > 0) {
        BeginScissorMode((int)sec_scissor.x, (int)sec_scissor.y, (int)sec_scissor.width, (int)sec_scissor.height);
        float row_h = 28.0f * s;
        float cur_y = content_y + 10.0f * s + g_sb_sector_scroll;

        Rectangle all_row = { pad.x + 8.0f * s, cur_y, sector_col_w - 16.0f * s, row_h };
        bool is_all_sel = (g_sb_selected_sector == -1);
        bool is_all_hov = CheckCollisionPointRec(mpos, all_row);

        if (is_all_sel) {
            DrawRectangleRounded(all_row, 0.18f, 4, ColorAlpha(theme->menu_hl, alpha));
            DrawRectangle((int)all_row.x, (int)all_row.y, (int)(3.0f * s), (int)all_row.height, c_alpha);
        } else if (is_all_hov) {
            DrawRectangleRounded(all_row, 0.18f, 4, ColorAlpha(theme->menu_hl, 0.50f * alpha));
        }
        DrawTextEx(font_body, ">> ALL SECTORS", (Vector2){ all_row.x + 10.0f * s, all_row.y + 7.0f * s }, 11.5f * s, 1.0f, is_all_sel ? c_alpha : ColorAlpha(theme->syn_default, alpha));
        cur_y += row_h + 4.0f * s;

        for (int i = 0; i < count; ++i) {
            if (!entries[i].is_directory) continue;
            Rectangle row = { pad.x + 8.0f * s, cur_y, sector_col_w - 16.0f * s, row_h };
            bool is_sel = (g_sb_selected_sector == i);
            bool is_hov = CheckCollisionPointRec(mpos, row);

            if (is_sel) {
                DrawRectangleRounded(row, 0.18f, 4, ColorAlpha(theme->menu_hl, alpha));
                DrawRectangle((int)row.x, (int)row.y, (int)(3.0f * s), (int)row.height, c_alpha);
            } else if (is_hov) {
                DrawRectangleRounded(row, 0.18f, 4, ColorAlpha(theme->menu_hl, 0.50f * alpha));
            }

            char sec_label[128];
            snprintf(sec_label, sizeof(sec_label), ":: %s", entries[i].name);
            DrawTextEx(font_body, sec_label, (Vector2){ row.x + 10.0f * s, row.y + 7.0f * s }, 11.5f * s, 1.0f, is_sel ? c_alpha : ColorAlpha(theme->syn_default, 0.85f * alpha));
            cur_y += row_h + 3.0f * s;
        }
        EndScissorMode();
    }

    // RIGHT DECK: DATACORES
    Rectangle file_deck = { pad.x + sector_col_w, content_y, file_col_w, content_h };
    Rectangle file_scissor = { file_deck.x + 4.0f * s, file_deck.y + 4.0f * s, file_deck.width - 8.0f * s, file_deck.height - 8.0f * s };
    if (file_scissor.width > 0 && file_scissor.height > 0) {
        BeginScissorMode((int)file_scissor.x, (int)file_scissor.y, (int)file_scissor.width, (int)file_scissor.height);

        float card_h = 38.0f * s;
        float cur_y = content_y + 10.0f * s + g_sb_file_scroll;

        if (g_sb_create_mode != 0) {
            Rectangle in_card = { file_deck.x + 10.0f * s, cur_y, file_deck.width - 20.0f * s, card_h };
            DrawRectangleRounded(in_card, 0.15f, 4, ColorAlpha(theme->menu_hl, alpha));
            DrawRectangleRoundedLines(in_card, 0.15f, 4, c_alpha);

            char in_title[128];
            snprintf(in_title, sizeof(in_title), "ENTER NEW %s NAME: %s_", (g_sb_create_mode == 1 ? "DATACORE" : "SECTOR"), g_sb_input_buf);
            DrawTextEx(font_body, in_title, (Vector2){ in_card.x + 12.0f * s, in_card.y + 11.0f * s }, 12.0f * s, 1.0f, c_alpha);
            cur_y += card_h + 8.0f * s;
        }

        const char *selected_prefix = (g_sb_selected_sector >= 0 && g_sb_selected_sector < count) ? entries[g_sb_selected_sector].path : NULL;
        int matching_files = 0;

        for (int i = 0; i < count; ++i) {
            if (entries[i].is_directory) continue;
            if (selected_prefix && strncmp(entries[i].path, selected_prefix, strlen(selected_prefix)) != 0) continue;
            if (!MatchesFilter(entries[i].name, entries[i].rel_path, g_sb_search)) continue;

            matching_files++;

            Rectangle card = { file_deck.x + 10.0f * s, cur_y, file_deck.width - 20.0f * s, card_h };
            bool is_active_file = (g_sb_ws->active_file_idx == i);
            bool is_hov = (g_sb_hovered_file == i);

            if (is_active_file) {
                DrawRectangleRounded(card, 0.16f, 4, ColorAlpha(theme->menu_hl, alpha));
                DrawRectangleRoundedLines(card, 0.16f, 4, c_alpha);
            } else if (is_hov) {
                DrawRectangleRounded(card, 0.16f, 4, ColorAlpha(theme->menu_hl, 0.65f * alpha));
                DrawRectangleRoundedLines(card, 0.16f, 4, ColorAlpha(theme->cursor, 0.5f * alpha));
            } else {
                DrawRectangleRounded(card, 0.16f, 4, ColorAlpha(theme->gutter_bg, 0.70f * alpha));
                DrawRectangleRoundedLines(card, 0.16f, 4, ColorAlpha(theme->gutter_num, 0.28f * alpha));
            }

            const char *badge = GetLanguageBadge(entries[i].name);
            Rectangle b_rect = { card.x + 8.0f * s, card.y + 9.0f * s, 54.0f * s, 19.0f * s };
            DrawRectangleRounded(b_rect, 0.20f, 4, ColorAlpha(theme->syn_keyword, 0.18f * alpha));
            DrawTextEx(font_body, badge, (Vector2){ b_rect.x + 5.0f * s, b_rect.y + 3.0f * s }, 10.0f * s, 1.0f, ColorAlpha(theme->syn_keyword, alpha));

            Color name_col = is_active_file ? c_alpha : (is_hov ? ColorAlpha(theme->syn_default, alpha) : ColorAlpha(theme->syn_default, 0.9f * alpha));
            DrawTextEx(font_body, entries[i].name, (Vector2){ card.x + 68.0f * s, card.y + 7.0f * s }, 13.0f * s, 1.0f, name_col);

            DrawTextEx(font_body, entries[i].rel_path, (Vector2){ card.x + 70.0f * s, card.y + 22.0f * s }, 9.5f * s, 1.0f, ColorAlpha(theme->gutter_num, alpha));

            if (is_hov) {
                const char *act_callout = ">> DEPLOY DATACORE";
                Vector2 act_sz = MeasureTextEx(font_body, act_callout, 11.5f * s, 1.0f);
                DrawTextEx(font_body, act_callout, (Vector2){ card.x + card.width - act_sz.x - 12.0f * s, card.y + 12.0f * s }, 11.5f * s, 1.0f, c_alpha);
            } else if (is_active_file) {
                const char *status_str = "[ACTIVE / DEPLOYED]";
                Vector2 st_sz = MeasureTextEx(font_body, status_str, 10.0f * s, 1.0f);
                DrawTextEx(font_body, status_str, (Vector2){ card.x + card.width - st_sz.x - 12.0f * s, card.y + 12.0f * s }, 10.0f * s, 1.0f, c_alpha);
            }

            cur_y += card_h + 6.0f * s;
        }

        if (matching_files == 0 && g_sb_create_mode == 0) {
            DrawTextEx(font_body, "NO MATCHING DATACORES IN THIS SECTOR.", (Vector2){ file_deck.x + 24.0f * s, content_y + 24.0f * s }, 12.0f * s, 1.0f, ColorAlpha(theme->gutter_num, alpha));
        }

        EndScissorMode();
    }

    float footer_y = pad.y + pad.height - footer_h;
    DrawLine((int)(pad.x + 10.0f * s), (int)footer_y, (int)(pad.x + pad.width - 10.0f * s), (int)footer_y, ColorAlpha(theme->gutter_num, 0.35f * alpha));

    DrawTextEx(font_body, "[ESC] DISENGAGE  |  [CLICK] DEPLOY DATACORE  |  [WHEEL] SCROLL  |  [CTRL+B] TOGGLE",
               (Vector2){ pad.x + 16.0f * s, footer_y + 9.0f * s }, 10.5f * s, 1.0f, ColorAlpha(theme->gutter_num, alpha));
}