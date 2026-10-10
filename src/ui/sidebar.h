#ifndef CE_UI_SIDEBAR_H
#define CE_UI_SIDEBAR_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "modules/project/workspace.h"
#include "modules/buffer/document.h"
#include <stdbool.h>

// Initializes the Mission Select Datapad subsystem
void Sidebar_Init(Workspace *ws, Document *doc);

// State controls
void Sidebar_Open(void);
void Sidebar_Close(void);
void Sidebar_Toggle(void);

// Visibility state accessors
bool Sidebar_IsOpen(void);
void Sidebar_SetOpen(bool open);

// Returns active camera offset width (0.0f since it is a full cinematic overlay)
float Sidebar_GetWidth(float scale);

// Hit testing
bool Sidebar_ContainsPoint(Vector2 point, int screen_w, int screen_h, float scale);

// Handles keyboard navigation, search filtering, and deployment clicks
// Returns true if the input was consumed by the Datapad
bool Sidebar_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale);

// Renders the full-screen Tactical Datapad / Mission Select overlay
void Sidebar_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body);

#endif // CE_UI_SIDEBAR_H