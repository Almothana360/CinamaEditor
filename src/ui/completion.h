#ifndef CE_UI_COMPLETION_H
#define CE_UI_COMPLETION_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "modules/buffer/document.h"
#include <stdbool.h>

#define CE_MAX_COMP_CANDIDATES 128

typedef enum {
    COMP_KIND_KEYWORD = 0,
    COMP_KIND_TYPE,
    COMP_KIND_IDENTIFIER
} CompKind;

typedef struct {
    char word[64];
    CompKind kind;
    int score;
} CompCandidate;

// Lifecycle
void Completion_Init(Document *doc, Font font_syntax);
void Completion_Free(void);

// Visibility state controls
void Completion_Close(void);
bool Completion_IsOpen(void);

// Scans word under cursor and triggers candidate harvesting
void Completion_CheckTrigger(const Document *doc, Font font_syntax);

// Handles Up, Down, Tab, Enter, Esc and mouse clicks on candidates
// Returns true if the input was consumed
bool Completion_Update(Document *doc, float scale);

// Commits the selected candidate token into the document
void Completion_Commit(Document *doc);

// Hit testing
bool Completion_ContainsPoint(Vector2 point, Camera2D camera, const Document *doc, Font font_syntax, int screen_w, int screen_h, float scale);

// Renders the floating popup anchored below the cursor in screen space
void Completion_Draw(Camera2D camera, const Document *doc, Font font_syntax, Font font_body, int screen_w, int screen_h, float scale, const Theme *theme);

#endif // CE_UI_COMPLETION_H