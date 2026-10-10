#ifndef CE_UI_PAUSE_MENU_H
#define CE_UI_PAUSE_MENU_H

#include "raylib.h"
#include "core/theme.h"
#include <stdbool.h>

// Initializes the pause menu state
void PauseMenu_Init(void);

// Closes the pause menu
void PauseMenu_Close(void);

// Toggles the pause menu open/closed
void PauseMenu_Toggle(void);

// Returns true if the pause menu is currently open
bool PauseMenu_IsOpen(void);

// Handles keyboard navigation and mouse hover/clicks
// Returns true if input was consumed by the menu
bool PauseMenu_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale);

// Renders the cinematic full-screen overlay
void PauseMenu_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body);

#endif // CE_UI_PAUSE_MENU_H