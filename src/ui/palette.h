#ifndef CE_UI_PALETTE_H
#define CE_UI_PALETTE_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "modules/project/workspace.h"
#include "modules/buffer/document.h"
#include <stdbool.h>

// Initializes the command palette with references to the workspace and active document
void Palette_Init(Workspace *ws, Document *doc);

// Frees all allocated memory in the palette candidate pools
void Palette_Free(void);

// Modal state controls
void Palette_Open(void);
void Palette_Close(void);
void Palette_Toggle(void);
bool Palette_IsOpen(void);

// Updates keyboard input, fuzzy filtering, and selection execution
// Returns true if mouse or keyboard events were consumed
bool Palette_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale);

// Renders the center-anchored floating modal dialog and dark backdrop
void Palette_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body);

#endif // CE_UI_PALETTE_H