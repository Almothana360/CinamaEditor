#include "ui/menu_bar.h"
#include "core/event.h"
#include "modules/view/camera.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

static const MenuBarItem FILE_ITEMS[] = {
    { "Save File", "Ctrl+S", ACTION_SAVE, false },
    { "", "", ACTION_NONE, true },
    { "Reload / Reset", "", ACTION_LOAD, false },
    { "", "", ACTION_NONE, true },
    { "Help / Controls", "F1", ACTION_TOGGLE_HELP, false },
};

static const MenuBarItem EDIT_ITEMS[] = {
    { "Undo", "Ctrl+Z", ACTION_UNDO, false },
    { "Redo", "Ctrl+Y", ACTION_REDO, false },
    { "", "", ACTION_NONE, true },
    { "Cut", "Ctrl+X", ACTION_CUT, false },
    { "Copy", "Ctrl+C", ACTION_COPY, false },
    { "Paste", "Ctrl+V", ACTION_PASTE, false },
    { "", "", ACTION_NONE, true },
    { "Duplicate Line", "Ctrl+D", ACTION_DUPLICATE_LINE, false },
    { "Select All", "Ctrl+A", ACTION_SELECT_ALL, false },
};

static const MenuBarItem VIEW_ITEMS[] = {
    { "Next Theme", "F4", ACTION_CYCLE_THEME, false },
    { "Cycle UI Scale", "F8", ACTION_CYCLE_UI_SCALE, false },
    { "", "", ACTION_NONE, true },
    { "Toggle CRT FX", "F2", ACTION_TOGGLE_CRT, false },
    { "Toggle Spotlight", "F3", ACTION_TOGGLE_SPOTLIGHT, false },
    { "", "", ACTION_NONE, true },
    { "Zoom In", "Ctrl++", ACTION_ZOOM_IN, false },
    { "Zoom Out", "Ctrl+-", ACTION_ZOOM_OUT, false },
    { "Reset Zoom", "Ctrl+0", ACTION_ZOOM_RESET, false },
};

static const MenuBarItem CAMERA_ITEMS[] = {
    { "Script Fit Mode", "F5", ACTION_CAM_BOUNDS_FIT, false },
    { "Cursor Focus Mode", "F6", ACTION_CAM_CURSOR_FOCUS, false },
    { "Line Focus Mode", "F7", ACTION_CAM_LINE_FOCUS, false },
};

static const MenuBarItem HELP_ITEMS[] = {
    { "Controls & Shortcuts", "F1", ACTION_TOGGLE_HELP, false },
    { "", "", ACTION_NONE, true },
    { "Reset Zoom", "Ctrl+0", ACTION_ZOOM_RESET, false },
    { "Cycle Themes", "F4", ACTION_CYCLE_THEME, false },
};

typedef struct {
    const char *title;
    const MenuBarItem *items;
    int item_count;
} MenuBarCategoryDef;

static const MenuBarCategoryDef CATEGORIES[CE_MENU_CATEGORY_COUNT] = {
    { "File", FILE_ITEMS, sizeof(FILE_ITEMS) / sizeof(FILE_ITEMS[0]) },
    { "Edit", EDIT_ITEMS, sizeof(EDIT_ITEMS) / sizeof(EDIT_ITEMS[0]) },
    { "View", VIEW_ITEMS, sizeof(VIEW_ITEMS) / sizeof(VIEW_ITEMS[0]) },
    { "Camera", CAMERA_ITEMS, sizeof(CAMERA_ITEMS) / sizeof(CAMERA_ITEMS[0]) },
    { "Help", HELP_ITEMS, sizeof(HELP_ITEMS) / sizeof(HELP_ITEMS[0]) },
};

typedef struct {
    int active_category;
    int hovered_category;
    int hovered_item;
} MenuBarState;

static MenuBarState g_menu_bar = {
    .active_category = -1,
    .hovered_category = -1,
    .hovered_item = -1
};

static float MenuBar_GetEffectiveScale(int screen_w, float user_scale) {
    int screen_h = GetScreenHeight();
    float sw = (screen_w > 0) ? (float)screen_w : 1280.0f;
    float sh = (screen_h > 0) ? (float)screen_h : 720.0f;
    float u = (user_scale > 0.1f) ? user_scale : 1.0f;
    float res_scale = fminf(sw / 1280.0f, sh / 720.0f);
    if (res_scale < 0.75f) res_scale = 0.75f;
    return u * res_scale;
}

void MenuBar_Init(void) {
    g_menu_bar.active_category = -1;
    g_menu_bar.hovered_category = -1;
    g_menu_bar.hovered_item = -1;
}

void MenuBar_Close(void) {
    g_menu_bar.active_category = -1;
    g_menu_bar.hovered_category = -1;
    g_menu_bar.hovered_item = -1;
}

bool MenuBar_IsActive(void) {
    return (g_menu_bar.active_category >= 0 || g_menu_bar.hovered_category >= 0);
}

float MenuBar_GetBaseHeight(void) {
    return 32.0f;
}

float MenuBar_GetHeight(float scale) {
    int screen_w = GetScreenWidth();
    float s = MenuBar_GetEffectiveScale(screen_w, scale);
    return 10.0f * s + 32.0f * s; // Top margin + Bar height
}

// Container rectangle for the floating, rounded menu bar
static Rectangle MenuBar_GetContainerRect(int screen_w, float s) {
    float margin_top = 10.0f * s;
    float margin_left = 16.0f * s;
    // Reserve space on the right side for the minimap (120 width + 16 right margin + 18 gap)
    float minimap_reserve = 154.0f * s;
    float bar_w = (float)screen_w - margin_left - minimap_reserve;
    if (bar_w < 320.0f * s) {
        bar_w = (float)screen_w - margin_left * 2.0f;
    }
    float bar_h = 32.0f * s;

    return (Rectangle){
        .x = margin_left,
        .y = margin_top,
        .width = bar_w,
        .height = bar_h
    };
}

// Subdivides the menu bar container equally among the 5 categories
static Rectangle MenuBar_GetCategoryRect(int index, int screen_w, float s) {
    if (index < 0 || index >= CE_MENU_CATEGORY_COUNT || screen_w <= 0) {
        return (Rectangle){ 0, 0, 0, 0 };
    }
    Rectangle bar = MenuBar_GetContainerRect(screen_w, s);
    float slot_w = bar.width / (float)CE_MENU_CATEGORY_COUNT;
    float x0 = bar.x + (float)index * slot_w;
    float x1 = (index == CE_MENU_CATEGORY_COUNT - 1) ? (bar.x + bar.width) : (bar.x + (float)(index + 1) * slot_w);

    return (Rectangle){
        .x = x0,
        .y = bar.y,
        .width = x1 - x0,
        .height = bar.height
    };
}

static Rectangle MenuBar_GetDropdownRect(int cat_index, int screen_w, float s) {
    if (cat_index < 0 || cat_index >= CE_MENU_CATEGORY_COUNT) {
        return (Rectangle){ 0, 0, 0, 0 };
    }
    Rectangle cat_rect = MenuBar_GetCategoryRect(cat_index, screen_w, s);
    const MenuBarCategoryDef *cat = &CATEGORIES[cat_index];

    float item_h = 28.0f * s;
    float sep_h = 8.0f * s;
    float pad_y = 6.0f * s;

    float total_h = pad_y * 2.0f;
    for (int i = 0; i < cat->item_count; ++i) {
        total_h += cat->items[i].is_separator ? sep_h : item_h;
    }

    float min_w = 230.0f * s;
    float dropdown_w = fmaxf(min_w, cat_rect.width);
    float dropdown_x = cat_rect.x;

    if (dropdown_x + dropdown_w > (float)screen_w - 12.0f * s) {
        dropdown_x = (float)screen_w - dropdown_w - 12.0f * s;
    }
    if (dropdown_x < 12.0f * s) {
        dropdown_x = 12.0f * s;
    }

    return (Rectangle){
        .x = dropdown_x,
        .y = cat_rect.y + cat_rect.height + 4.0f * s,
        .width = dropdown_w,
        .height = total_h
    };
}

bool MenuBar_ContainsPoint(Vector2 point, int screen_w, float scale) {
    float s = MenuBar_GetEffectiveScale(screen_w, scale);
    Rectangle bar = MenuBar_GetContainerRect(screen_w, s);
    if (CheckCollisionPointRec(point, bar)) {
        return true;
    }
    if (g_menu_bar.active_category >= 0 && g_menu_bar.active_category < CE_MENU_CATEGORY_COUNT) {
        Rectangle drop = MenuBar_GetDropdownRect(g_menu_bar.active_category, screen_w, s);
        if (CheckCollisionPointRec(point, drop)) {
            return true;
        }
    }
    return false;
}

bool MenuBar_Update(Vector2 mouse_screen, int screen_w, float scale) {
    float s = MenuBar_GetEffectiveScale(screen_w, scale);
    Rectangle bar = MenuBar_GetContainerRect(screen_w, s);

    g_menu_bar.hovered_category = -1;
    g_menu_bar.hovered_item = -1;

    // Check category hover inside the floating menu bar
    if (CheckCollisionPointRec(mouse_screen, bar)) {
        for (int i = 0; i < CE_MENU_CATEGORY_COUNT; ++i) {
            Rectangle cat_rect = MenuBar_GetCategoryRect(i, screen_w, s);
            if (CheckCollisionPointRec(mouse_screen, cat_rect)) {
                g_menu_bar.hovered_category = i;
                break;
            }
        }
    }

    // Switch active dropdown automatically when hovering across categories
    if (g_menu_bar.active_category >= 0 && g_menu_bar.hovered_category >= 0) {
        if (g_menu_bar.hovered_category != g_menu_bar.active_category) {
            g_menu_bar.active_category = g_menu_bar.hovered_category;
        }
    }

    // Check item hover inside the active dropdown
    Rectangle drop_rect = { 0, 0, 0, 0 };
    if (g_menu_bar.active_category >= 0) {
        drop_rect = MenuBar_GetDropdownRect(g_menu_bar.active_category, screen_w, s);
        if (CheckCollisionPointRec(mouse_screen, drop_rect)) {
            const MenuBarCategoryDef *cat = &CATEGORIES[g_menu_bar.active_category];
            float item_h = 28.0f * s;
            float sep_h = 8.0f * s;
            float cur_y = drop_rect.y + 6.0f * s;

            for (int i = 0; i < cat->item_count; ++i) {
                if (cat->items[i].is_separator) {
                    cur_y += sep_h;
                    continue;
                }
                Rectangle item_rect = {
                    drop_rect.x + 4.0f * s,
                    cur_y,
                    drop_rect.width - 8.0f * s,
                    item_h
                };
                if (CheckCollisionPointRec(mouse_screen, item_rect)) {
                    g_menu_bar.hovered_item = i;
                    break;
                }
                cur_y += item_h;
            }
        }
    }

    // Escape closes the active menu
    if (IsKeyPressed(KEY_ESCAPE) && g_menu_bar.active_category >= 0) {
        g_menu_bar.active_category = -1;
        return true;
    }

    // Left click handling
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g_menu_bar.hovered_category >= 0) {
            if (g_menu_bar.active_category == g_menu_bar.hovered_category) {
                g_menu_bar.active_category = -1;
            } else {
                g_menu_bar.active_category = g_menu_bar.hovered_category;
            }
            return true;
        }

        if (g_menu_bar.active_category >= 0) {
            if (CheckCollisionPointRec(mouse_screen, drop_rect)) {
                if (g_menu_bar.hovered_item >= 0) {
                    const MenuBarCategoryDef *cat = &CATEGORIES[g_menu_bar.active_category];
                    const MenuBarItem *item = &cat->items[g_menu_bar.hovered_item];
                    if (!item->is_separator && item->action != ACTION_NONE) {
                        ActionPayload p = {
                            .action = item->action,
                            .char_data = 0,
                            .float_data = 0.0f,
                            .shift_held = false,
                            .ctrl_held = false
                        };
                        Event_Emit(EV_ACTION, &p);
                    }
                }
                g_menu_bar.active_category = -1;
                return true;
            } else {
                g_menu_bar.active_category = -1;
                return true;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && g_menu_bar.active_category >= 0) {
        g_menu_bar.active_category = -1;
    }

    return (g_menu_bar.active_category >= 0 || g_menu_bar.hovered_category >= 0);
}

void MenuBar_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    if (!theme) return;
    (void)screen_h;
    float s = MenuBar_GetEffectiveScale(screen_w, scale);

    // 1. Draw floating rounded menu bar container
    Rectangle bar = MenuBar_GetContainerRect(screen_w, s);
    DrawRectangleRounded(bar, 0.25f, 6, theme->menu_bg);
    DrawRectangleRoundedLines(bar, 0.25f, 6, theme->menu_border);

    // 2. Draw equally distributed category buttons
    float font_size = 13.0f * s;
    for (int i = 0; i < CE_MENU_CATEGORY_COUNT; ++i) {
        Rectangle cat_rect = MenuBar_GetCategoryRect(i, screen_w, s);
        bool is_active = (g_menu_bar.active_category == i);
        bool is_hovered = (g_menu_bar.hovered_category == i);

        Rectangle btn_pill = {
            cat_rect.x + 3.0f * s,
            cat_rect.y + 3.0f * s,
            cat_rect.width - 6.0f * s,
            cat_rect.height - 6.0f * s
        };

        if (is_active) {
            DrawRectangleRounded(btn_pill, 0.22f, 4, theme->menu_hl);
            DrawRectangleRoundedLines(btn_pill, 0.22f, 4, theme->cursor);
        } else if (is_hovered) {
            DrawRectangleRounded(btn_pill, 0.22f, 4, ColorAlpha(theme->menu_hl, 0.65f));
        }

        // Subtle vertical separator between category slots
        if (i < CE_MENU_CATEGORY_COUNT - 1) {
            int div_x = (int)(cat_rect.x + cat_rect.width);
            DrawLine(div_x, (int)(bar.y + 6.0f * s), div_x, (int)(bar.y + bar.height - 6.0f * s), ColorAlpha(theme->gutter_num, 0.28f));
        }

        Color text_col = (is_active || is_hovered) ? theme->cursor : theme->syn_default;
        Vector2 text_sz = MeasureTextEx(font_body, CATEGORIES[i].title, font_size, 1.0f);
        Vector2 text_pos = {
            cat_rect.x + (cat_rect.width - text_sz.x) * 0.5f,
            cat_rect.y + (cat_rect.height - text_sz.y) * 0.5f
        };
        DrawTextEx(font_body, CATEGORIES[i].title, text_pos, font_size, 1.0f, text_col);
    }

    // 3. Draw active dropdown menu (Rounded Rectangle)
    if (g_menu_bar.active_category >= 0 && g_menu_bar.active_category < CE_MENU_CATEGORY_COUNT) {
        Rectangle drop_rect = MenuBar_GetDropdownRect(g_menu_bar.active_category, screen_w, s);
        const MenuBarCategoryDef *cat = &CATEGORIES[g_menu_bar.active_category];

        DrawRectangleRounded((Rectangle){ drop_rect.x + 2.0f * s, drop_rect.y + 2.0f * s, drop_rect.width, drop_rect.height }, 0.06f, 4, (Color){ 0, 0, 0, 120 });
        DrawRectangleRounded(drop_rect, 0.06f, 4, theme->menu_bg);
        DrawRectangleRoundedLines(drop_rect, 0.06f, 4, theme->menu_border);

        float item_h = 28.0f * s;
        float sep_h = 8.0f * s;
        float cur_y = drop_rect.y + 6.0f * s;
        CCameraMode cam_mode = Camera_GetMode();

        for (int i = 0; i < cat->item_count; ++i) {
            const MenuBarItem *item = &cat->items[i];
            if (item->is_separator) {
                DrawLine((int)(drop_rect.x + 10.0f * s), (int)(cur_y + sep_h * 0.5f),
                         (int)(drop_rect.x + drop_rect.width - 10.0f * s), (int)(cur_y + sep_h * 0.5f),
                         ColorAlpha(theme->gutter_num, 0.40f));
                cur_y += sep_h;
                continue;
            }

            Rectangle item_rect = {
                drop_rect.x + 4.0f * s,
                cur_y,
                drop_rect.width - 8.0f * s,
                item_h
            };

            bool is_hovered = (g_menu_bar.hovered_item == i);
            if (is_hovered) {
                DrawRectangleRounded(item_rect, 0.15f, 4, theme->menu_hl);
                DrawRectangle((int)item_rect.x, (int)item_rect.y, (int)(3.0f * s), (int)item_rect.height, theme->cursor);
            }

            char label_buf[96];
            const char *display_label = item->label;
            if (item->action == ACTION_CAM_BOUNDS_FIT) {
                snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_BOUNDS_FIT ? "[*]" : "[ ]"), item->label);
                display_label = label_buf;
            } else if (item->action == ACTION_CAM_CURSOR_FOCUS) {
                snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_CURSOR_FOCUS ? "[*]" : "[ ]"), item->label);
                display_label = label_buf;
            } else if (item->action == ACTION_CAM_LINE_FOCUS) {
                snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_LINE_FOCUS ? "[*]" : "[ ]"), item->label);
                display_label = label_buf;
            }

            Color text_col = is_hovered ? theme->cursor : theme->syn_default;
            DrawTextEx(font_body, display_label, (Vector2){ item_rect.x + 10.0f * s, item_rect.y + 5.0f * s }, 13.0f * s, 1.0f, text_col);

            if (item->shortcut && item->shortcut[0] != '\0') {
                Vector2 sz = MeasureTextEx(font_body, item->shortcut, 12.0f * s, 1.0f);
                DrawTextEx(font_body, item->shortcut, (Vector2){ item_rect.x + item_rect.width - sz.x - 8.0f * s, item_rect.y + 6.0f * s }, 12.0f * s, 1.0f, theme->gutter_num);
            }

            cur_y += item_h;
        }
    }
}