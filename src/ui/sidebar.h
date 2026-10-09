#ifndef CE_UI_SIDEBAR_H
#define CE_UI_SIDEBAR_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "modules/project/workspace.h"
#include "modules/buffer/document.h"
#include <stdbool.h>

// Initializes the sidebar subsystem with references to workspace and document
void Sidebar_Init(Workspace *ws, Document *doc);

// Toggles sidebar visibility (Ctrl+B)
void Sidebar_Toggle(void);

// Visibility state accessors
bool Sidebar_IsOpen(void);
void Sidebar_SetOpen(bool open);

// Returns active rendered width of the sidebar (0 if collapsed)
float Sidebar_GetWidth(float scale);

// Hit testing
bool Sidebar_ContainsPoint(Vector2 point, int screen_w, int screen_h, float scale);

// Input & Interaction handling (returns true if mouse event was consumed)
bool Sidebar_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale);

// Renders the floating rounded sidebar panel and tree hierarchy
void Sidebar_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body);

#endif // CE_UI_SIDEBAR_H