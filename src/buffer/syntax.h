#ifndef CE_BUFFER_SYNTAX_H
#define CE_BUFFER_SYNTAX_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "buffer/line.h"
#include "buffer/grammar.h"
#include "modules/buffer/document.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    bool found;
    size_t row;
    size_t col;
} BracketMatch;

// Lifecycle & Active Language Selection
void Syntax_Init(const Document *doc, Font font);
void Syntax_SetLanguage(const LanguageDef *lang);
const LanguageDef *Syntax_GetLanguage(void);
void Syntax_SetLanguageByFilename(const char *filepath);

// Token classification (uses active LanguageDef or explicit LanguageDef)
bool Syntax_IsKeyword(const char *word);
bool Syntax_IsType(const char *word);
Color Syntax_GetHighlightColor(const char *word, const Theme *theme);

BracketMatch Syntax_FindMatchingBracket(const Line *lines, size_t line_count, size_t cur_row, size_t cur_col);
void Syntax_UpdateMultilineComments(Line *lines, size_t line_count);

void Syntax_DrawToken(Font font, const Line *line, size_t start, size_t len, float start_x, float start_y, Color color, const Theme *theme);
void Syntax_DrawLine(Font font, const Line *line, float start_x, float start_y, const Theme *theme);

#endif // CE_BUFFER_SYNTAX_H