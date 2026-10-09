#include "ui/palette.h"
#include "ui/sidebar.h"
#include "core/event.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

typedef enum {
    PAL_ITEM_COMMAND = 0,
    PAL_ITEM_FILE
} PaletteItemType;

typedef struct {
    PaletteItemType type;
    char title[128];
    char subtitle[128];
    ActionType action;
    int file_entry_idx;
    int score;
} PaletteItem;

static Workspace *g_pal_ws = NULL;
static Document *g_pal_doc = NULL;

static bool g_pal_open = false;
static char g_pal_query[64] = {0};
static int g_pal_query_len = 0;

static PaletteItem *g_pal_candidates = NULL;
static int g_pal_candidate_count = 0;
static int g_pal_candidate_cap = 0;

static PaletteItem *g_pal_filtered = NULL;
static int g_pal_filtered_count = 0;
static int g_pal_filtered_cap = 0;

static int g_pal_selected_idx = 0;
static int g_pal_scroll_offset = 0;

static float Palette_GetEffectiveScale(int screen_w, float user_scale) {
    int screen_h = GetScreenHeight();
    float sw = (screen_w > 0) ? (float)screen_w : 1280.0f;
    float sh = (screen_h > 0) ? (float)screen_h : 720.0f;
    float u = (user_scale > 0.1f) ? user_scale : 1.0f;
    float res_scale = fminf(sw / 1280.0f, sh / 720.0f);
    if (res_scale < 0.75f) res_scale = 0.75f;
    return u * res_scale;
}

static void Palette_AddCandidate(PaletteItemType type, const char *title, const char *subtitle, ActionType action, int file_idx) {
    if (g_pal_candidate_count >= g_pal_candidate_cap) {
        int new_cap = (g_pal_candidate_cap == 0) ? 64 : g_pal_candidate_cap * 2;
        PaletteItem *nc = (PaletteItem *)realloc(g_pal_candidates, new_cap * sizeof(PaletteItem));
        if (!nc) return;
        g_pal_candidates = nc;
        g_pal_candidate_cap = new_cap;
    }

    PaletteItem *item = &g_pal_candidates[g_pal_candidate_count++];
    item->type = type;
    strncpy(item->title, title ? title : "", sizeof(item->title) - 1);
    item->title[sizeof(item->title) - 1] = '\0';
    strncpy(item->subtitle, subtitle ? subtitle : "", sizeof(item->subtitle) - 1);
    item->subtitle[sizeof(item->subtitle) - 1] = '\0';
    item->action = action;
    item->file_entry_idx = file_idx;
    item->score = 0;
}

static void Palette_BuildCandidates(void) {
    g_pal_candidate_count = 0;

    // 1. Register Core Application Commands
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Save File", "Ctrl+S", ACTION_SAVE, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Toggle File Explorer", "Ctrl+B", ACTION_NONE, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Undo", "Ctrl+Z", ACTION_UNDO, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Redo", "Ctrl+Y", ACTION_REDO, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Cut Text", "Ctrl+X", ACTION_CUT, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Copy Text", "Ctrl+C", ACTION_COPY, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Paste Text", "Ctrl+V", ACTION_PASTE, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Select All", "Ctrl+A", ACTION_SELECT_ALL, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Duplicate Line", "Ctrl+D", ACTION_DUPLICATE_LINE, -1);

    Palette_AddCandidate(PAL_ITEM_COMMAND, "Next Theme", "F4", ACTION_CYCLE_THEME, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Cycle UI Scale", "F8", ACTION_CYCLE_UI_SCALE, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Toggle CRT Scanlines", "F2", ACTION_TOGGLE_CRT, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Toggle Spotlight", "F3", ACTION_TOGGLE_SPOTLIGHT, -1);

    Palette_AddCandidate(PAL_ITEM_COMMAND, "Camera: Script Fit Mode", "F5", ACTION_CAM_BOUNDS_FIT, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Camera: Cursor Focus Mode", "F6", ACTION_CAM_CURSOR_FOCUS, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Camera: Line Focus Mode", "F7", ACTION_CAM_LINE_FOCUS, -1);

    Palette_AddCandidate(PAL_ITEM_COMMAND, "Zoom In", "Ctrl++", ACTION_ZOOM_IN, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Zoom Out", "Ctrl+-", ACTION_ZOOM_OUT, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Reset Zoom", "Ctrl+0", ACTION_ZOOM_RESET, -1);

    Palette_AddCandidate(PAL_ITEM_COMMAND, "Help & Controls Dialog", "F1", ACTION_TOGGLE_HELP, -1);
    Palette_AddCandidate(PAL_ITEM_COMMAND, "Refresh Workspace Files", "Refresh", ACTION_NONE, -2);

    // 2. Register Workspace Project Files
    if (g_pal_ws) {
        int count = 0;
        const WorkspaceEntry *entries = Workspace_GetEntries(g_pal_ws, &count);
        for (int i = 0; i < count; ++i) {
            if (!entries[i].is_directory) {
                Palette_AddCandidate(PAL_ITEM_FILE, entries[i].name, entries[i].rel_path, ACTION_NONE, i);
            }
        }
    }
}

// Subsequence fuzzy scoring algorithm
static int FuzzyScore(const char *pattern, const char *str) {
    if (!pattern || pattern[0] == '\0') return 1;
    if (!str || str[0] == '\0') return 0;

    int score = 0;
    int p_idx = 0;
    int s_idx = 0;
    int consecutive = 0;

    while (pattern[p_idx] && str[s_idx]) {
        char pc = (char)tolower((unsigned char)pattern[p_idx]);
        char sc = (char)tolower((unsigned char)str[s_idx]);

        if (pc == sc) {
            int match_bonus = 12;
            match_bonus += consecutive * 6;
            consecutive++;

            if (s_idx == 0 || str[s_idx - 1] == ' ' || str[s_idx - 1] == '/' ||
                str[s_idx - 1] == '\\' || str[s_idx - 1] == '_' || str[s_idx - 1] == '.' ||
                str[s_idx - 1] == '-') {
                match_bonus += 18;
            }

            if (pattern[p_idx] == str[s_idx]) {
                match_bonus += 2;
            }

            score += match_bonus;
            p_idx++;
        } else {
            consecutive = 0;
        }
        s_idx++;
    }

    if (pattern[p_idx] != '\0') return 0; // Not all pattern characters matched

    score -= (int)strlen(str) * 2;
    return (score > 1) ? score : 1;
}

static int Palette_ScoreCompare(const void *a, const void *b) {
    const PaletteItem *ia = (const PaletteItem *)a;
    const PaletteItem *ib = (const PaletteItem *)b;
    return ib->score - ia->score;
}

static void Palette_FilterCandidates(void) {
    g_pal_filtered_count = 0;

    for (int i = 0; i < g_pal_candidate_count; ++i) {
        PaletteItem *cand = &g_pal_candidates[i];
        int score = 0;

        if (g_pal_query_len == 0) {
            score = 100 - i;
        } else {
            int score_title = FuzzyScore(g_pal_query, cand->title);
            int score_sub = (cand->type == PAL_ITEM_FILE) ? FuzzyScore(g_pal_query, cand->subtitle) : 0;
            score = (score_title > score_sub) ? score_title : score_sub;
        }

        if (score > 0) {
            if (g_pal_filtered_count >= g_pal_filtered_cap) {
                int new_cap = (g_pal_filtered_cap == 0) ? 64 : g_pal_filtered_cap * 2;
                PaletteItem *nf = (PaletteItem *)realloc(g_pal_filtered, new_cap * sizeof(PaletteItem));
                if (!nf) return;
                g_pal_filtered = nf;
                g_pal_filtered_cap = new_cap;
            }

            PaletteItem *dest = &g_pal_filtered[g_pal_filtered_count++];
            *dest = *cand;
            dest->score = score;
        }
    }

    if (g_pal_filtered_count > 1 && g_pal_query_len > 0) {
        qsort(g_pal_filtered, g_pal_filtered_count, sizeof(PaletteItem), Palette_ScoreCompare);
    }

    g_pal_selected_idx = 0;
    g_pal_scroll_offset = 0;
}

static void Palette_ExecuteSelected(void) {
    if (g_pal_filtered_count == 0 || g_pal_selected_idx < 0 || g_pal_selected_idx >= g_pal_filtered_count) {
        Palette_Close();
        return;
    }

    PaletteItem item = g_pal_filtered[g_pal_selected_idx];
    Palette_Close();

    if (item.type == PAL_ITEM_COMMAND) {
        if (item.action != ACTION_NONE) {
            ActionPayload p = {
                .action = item.action,
                .char_data = 0,
                .float_data = 0.0f,
                .shift_held = false,
                .ctrl_held = false
            };
            Event_Emit(EV_ACTION, &p);
        } else if (item.file_entry_idx == -1) {
            Sidebar_Toggle();
        } else if (item.file_entry_idx == -2 && g_pal_ws) {
            Workspace_Refresh(g_pal_ws);
        }
    } else if (item.type == PAL_ITEM_FILE) {
        if (g_pal_ws && g_pal_doc && item.file_entry_idx >= 0) {
            Workspace_OpenFileIndex(g_pal_ws, g_pal_doc, item.file_entry_idx);
        }
    }
}

void Palette_Init(Workspace *ws, Document *doc) {
    g_pal_ws = ws;
    g_pal_doc = doc;
    g_pal_open = false;
    g_pal_query[0] = '\0';
    g_pal_query_len = 0;
    g_pal_selected_idx = 0;
    g_pal_scroll_offset = 0;
}

void Palette_Free(void) {
    if (g_pal_candidates) {
        free(g_pal_candidates);
        g_pal_candidates = NULL;
    }
    g_pal_candidate_count = 0;
    g_pal_candidate_cap = 0;

    if (g_pal_filtered) {
        free(g_pal_filtered);
        g_pal_filtered = NULL;
    }
    g_pal_filtered_count = 0;
    g_pal_filtered_cap = 0;
}

void Palette_Open(void) {
    g_pal_open = true;
    g_pal_query[0] = '\0';
    g_pal_query_len = 0;
    Palette_BuildCandidates();
    Palette_FilterCandidates();
}

void Palette_Close(void) {
    g_pal_open = false;
    g_pal_query[0] = '\0';
    g_pal_query_len = 0;
}

void Palette_Toggle(void) {
    if (g_pal_open) Palette_Close();
    else Palette_Open();
}

bool Palette_IsOpen(void) {
    return g_pal_open;
}

static Rectangle Palette_GetDialogRect(int screen_w, int screen_h, float s) {
    float width = 560.0f * s;
    if (width > (float)screen_w - 32.0f * s) {
        width = (float)screen_w - 32.0f * s;
    }

    int visible_items = g_pal_filtered_count;
    if (visible_items > 8) visible_items = 8;
    if (visible_items < 1) visible_items = 1;

    float item_h = 32.0f * s;
    float input_h = 42.0f * s;
    float footer_h = 24.0f * s;
    float padding = 16.0f * s;

    float height = input_h + ((float)visible_items * item_h) + footer_h + padding;
    if (height > (float)screen_h - 40.0f * s) {
        height = (float)screen_h - 40.0f * s;
    }

    float x = ((float)screen_w - width) * 0.5f;
    float y = 55.0f * s;

    return (Rectangle){ x, y, width, height };
}

bool Palette_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale) {
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    // Global Hotkey: Ctrl+P toggles the palette
    if (ctrl && IsKeyPressed(KEY_P)) {
        Palette_Toggle();
        return true;
    }

    if (!g_pal_open) return false;

    float s = Palette_GetEffectiveScale(screen_w, scale);
    Rectangle dialog = Palette_GetDialogRect(screen_w, screen_h, s);

    // 1. Text Typing Input
    int ch = GetCharPressed();
    bool query_changed = false;
    while (ch > 0) {
        if (ch >= 32 && ch <= 126) {
            if (g_pal_query_len < (int)sizeof(g_pal_query) - 2) {
                g_pal_query[g_pal_query_len++] = (char)ch;
                g_pal_query[g_pal_query_len] = '\0';
                query_changed = true;
            }
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (g_pal_query_len > 0) {
            g_pal_query[--g_pal_query_len] = '\0';
            query_changed = true;
        }
    }

    if (query_changed) {
        Palette_FilterCandidates();
    }

    // 2. Selection Navigation
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        if (g_pal_filtered_count > 0) {
            g_pal_selected_idx = (g_pal_selected_idx - 1 + g_pal_filtered_count) % g_pal_filtered_count;
        }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        if (g_pal_filtered_count > 0) {
            g_pal_selected_idx = (g_pal_selected_idx + 1) % g_pal_filtered_count;
        }
    }

    // Keep active selection in the scroll view
    if (g_pal_selected_idx < g_pal_scroll_offset) {
        g_pal_scroll_offset = g_pal_selected_idx;
    }
    if (g_pal_selected_idx >= g_pal_scroll_offset + 8) {
        g_pal_scroll_offset = g_pal_selected_idx - 7;
    }

    // 3. Commit / Cancel Keys
    if (IsKeyPressed(KEY_ENTER)) {
        Palette_ExecuteSelected();
        return true;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        Palette_Close();
        return true;
    }

    // 4. Mouse Clicks
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse_screen, dialog)) {
            float item_h = 32.0f * s;
            float list_y = dialog.y + 44.0f * s;

            int visible_items = g_pal_filtered_count - g_pal_scroll_offset;
            if (visible_items > 8) visible_items = 8;

            for (int i = 0; i < visible_items; ++i) {
                Rectangle item_rect = { dialog.x + 8.0f * s, list_y + (float)i * item_h, dialog.width - 16.0f * s, item_h };
                if (CheckCollisionPointRec(mouse_screen, item_rect)) {
                    g_pal_selected_idx = g_pal_scroll_offset + i;
                    Palette_ExecuteSelected();
                    return true;
                }
            }
        } else {
            Palette_Close();
            return true;
        }
    }

    // Mouse Wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        g_pal_scroll_offset -= (int)wheel;
        if (g_pal_scroll_offset < 0) g_pal_scroll_offset = 0;
        if (g_pal_scroll_offset > g_pal_filtered_count - 8) {
            g_pal_scroll_offset = (g_pal_filtered_count > 8) ? (g_pal_filtered_count - 8) : 0;
        }
    }

    return true; // Always consumes input while open
}

void Palette_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    if (!g_pal_open || !theme) return;

    float s = Palette_GetEffectiveScale(screen_w, scale);

    // 1. Semi-transparent backdrop overlay
    DrawRectangle(0, 0, screen_w, screen_h, (Color){ 0, 0, 0, 160 });

    Rectangle dialog = Palette_GetDialogRect(screen_w, screen_h, s);

    // 2. Dialog Shadow & Background (Rounded Rectangle)
    DrawRectangleRounded((Rectangle){ dialog.x + 3.0f * s, dialog.y + 4.0f * s, dialog.width, dialog.height }, 0.04f, 4, (Color){ 0, 0, 0, 140 });
    DrawRectangleRounded(dialog, 0.04f, 4, theme->menu_bg);
    DrawRectangleRoundedLines(dialog, 0.04f, 4, theme->menu_border);

    // 3. Search Query Input Box
    Rectangle input_box = { dialog.x + 8.0f * s, dialog.y + 8.0f * s, dialog.width - 16.0f * s, 34.0f * s };
    DrawRectangleRounded(input_box, 0.18f, 4, theme->line_hl);
    DrawRectangleRoundedLines(input_box, 0.18f, 4, ColorAlpha(theme->gutter_num, 0.45f));

    float font_size = 13.0f * s;
    DrawTextEx(font_body, ">", (Vector2){ input_box.x + 10.0f * s, input_box.y + 10.0f * s }, font_size + 2.0f, 1.0f, theme->cursor);

    float text_x = input_box.x + 28.0f * s;
    if (g_pal_query_len == 0) {
        DrawTextEx(font_body, "Type a command or filename... (Esc to close)", (Vector2){ text_x, input_box.y + 10.0f * s }, font_size, 1.0f, ColorAlpha(theme->gutter_num, 0.70f));
    } else {
        DrawTextEx(font_body, g_pal_query, (Vector2){ text_x, input_box.y + 10.0f * s }, font_size, 1.0f, theme->syn_default);
    }

    // Blinking cursor
    bool blink = ((int)(GetTime() * 2.5)) % 2 == 0;
    if (blink) {
        Vector2 q_sz = MeasureTextEx(font_body, g_pal_query, font_size, 1.0f);
        float cursor_x = (g_pal_query_len == 0) ? text_x : (text_x + q_sz.x + 1.0f);
        DrawRectangle((int)cursor_x, (int)(input_box.y + 8.0f * s), (int)(2.0f * s), (int)(18.0f * s), theme->cursor);
    }

    // 4. Candidate Items List
    float item_h = 32.0f * s;
    float cur_y = dialog.y + 48.0f * s;
    int visible_items = g_pal_filtered_count - g_pal_scroll_offset;
    if (visible_items > 8) visible_items = 8;

    Vector2 mpos = GetMousePosition();

    if (g_pal_filtered_count == 0) {
        DrawTextEx(font_body, "No matching commands or files found.", (Vector2){ dialog.x + 20.0f * s, cur_y + 10.0f * s }, font_size, 1.0f, theme->gutter_num);
    } else {
        for (int i = 0; i < visible_items; ++i) {
            int idx = g_pal_scroll_offset + i;
            const PaletteItem *item = &g_pal_filtered[idx];

            Rectangle row = { dialog.x + 8.0f * s, cur_y, dialog.width - 16.0f * s, item_h - 2.0f * s };
            bool is_selected = (idx == g_pal_selected_idx);
            bool is_hovered = CheckCollisionPointRec(mpos, row);

            if (is_selected) {
                DrawRectangleRounded(row, 0.16f, 4, theme->menu_hl);
                DrawRectangle((int)row.x, (int)row.y, (int)(3.0f * s), (int)row.height, theme->cursor);
            } else if (is_hovered) {
                DrawRectangleRounded(row, 0.16f, 4, ColorAlpha(theme->menu_hl, 0.55f));
            }

            // Category Badge
            const char *badge = (item->type == PAL_ITEM_COMMAND) ? "CMD" : "FILE";
            Color badge_col = (item->type == PAL_ITEM_COMMAND) ? theme->syn_keyword : theme->syn_type;
            Rectangle badge_rect = { row.x + 8.0f * s, row.y + 6.0f * s, 36.0f * s, 18.0f * s };
            DrawRectangleRounded(badge_rect, 0.22f, 4, ColorAlpha(badge_col, 0.18f));
            DrawTextEx(font_body, badge, (Vector2){ badge_rect.x + 5.0f * s, badge_rect.y + 3.0f * s }, 10.0f * s, 1.0f, badge_col);

            // Title
            Color title_col = is_selected ? theme->cursor : theme->syn_default;
            DrawTextEx(font_body, item->title, (Vector2){ row.x + 52.0f * s, row.y + 8.0f * s }, font_size, 1.0f, title_col);

            // Subtitle / Shortcut (right-aligned)
            if (item->subtitle[0] != '\0') {
                Vector2 sub_sz = MeasureTextEx(font_body, item->subtitle, 11.0f * s, 1.0f);
                DrawTextEx(font_body, item->subtitle, (Vector2){ row.x + row.width - sub_sz.x - 10.0f * s, row.y + 9.0f * s }, 11.0f * s, 1.0f, theme->gutter_num);
            }

            cur_y += item_h;
        }
    }

    // 5. Footer Hint Bar
    float footer_y = dialog.y + dialog.height - 20.0f * s;
    DrawLine((int)(dialog.x + 8.0f * s), (int)(footer_y - 4.0f * s), (int)(dialog.x + dialog.width - 8.0f * s), (int)(footer_y - 4.0f * s), ColorAlpha(theme->gutter_num, 0.30f));

    char count_str[64];
    snprintf(count_str, sizeof(count_str), "%d results", g_pal_filtered_count);
    DrawTextEx(font_body, count_str, (Vector2){ dialog.x + 12.0f * s, footer_y }, 10.5f * s, 1.0f, theme->gutter_num);

    const char *hints = "Enter: Select | Up/Down: Navigate | Esc: Close";
    Vector2 hints_sz = MeasureTextEx(font_body, hints, 10.5f * s, 1.0f);
    DrawTextEx(font_body, hints, (Vector2){ dialog.x + dialog.width - hints_sz.x - 12.0f * s, footer_y }, 10.5f * s, 1.0f, theme->gutter_num);
}