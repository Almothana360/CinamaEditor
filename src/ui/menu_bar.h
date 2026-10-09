#ifndef CE_UI_MENU_BAR_H
#define CE_UI_MENU_BAR_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include <stdbool.h>

#include "core/event.h"

#define CE_MENU_CATEGORY_COUNT 5

// Metadata for a dropdown menu entry
typedef struct {
    const char *label;
    const char *shortcut;
    ActionType action;
    bool is_separator;
} MenuBarItem;

// Category grouping in the top menu bar
typedef struct {
    const char *title;
    const MenuBarItem *items;
    int item_count;
} MenuBarCategory;

// Initializes the menu bar system state
void MenuBar_Init(void);

// Closes any active open dropdown
void MenuBar_Close(void);

// Returns true if a dropdown is currently open or mouse is hovering the top bar
bool MenuBar_IsActive(void);

// Returns unscaled base height of the menu bar
float MenuBar_GetBaseHeight(void);

// Returns scaled height of the menu bar in pixels
float MenuBar_GetHeight(float scale);

// Returns true if the screen coordinate lies within the top bar or an active dropdown
bool MenuBar_ContainsPoint(Vector2 point, int screen_w, float scale);

// Updates hover detection, category switching, and item click execution
// Returns true if mouse input was consumed by the menu bar
bool MenuBar_Update(Vector2 mouse_screen, int screen_w, float scale);

// Renders the full-width proportional top bar and any active dropdown overlay
void MenuBar_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body);

#endif // CE_UI_MENU_BAR_H