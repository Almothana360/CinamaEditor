#ifndef CE_BUFFER_SYNTAX_H
#define CE_BUFFER_SYNTAX_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "buffer/line.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * Result of searching for matching bracket pairs.
 */
typedef struct {
    bool found;
    size_t row;
    size_t col;
} BracketMatch;

/**
 * Checks whether the given identifier matches a C keyword.
 */
bool Syntax_IsKeyword(const char *word);

/**
 * Checks whether the given identifier matches a standard C or graphics primitive type.
 */
bool Syntax_IsType(const char *word);

/**
 * Returns the theme color for a keyword or type token, or BLANK if neither.
 */
Color Syntax_GetHighlightColor(const char *word, const Theme *theme);

/**
 * Finds the corresponding matching bracket for a bracket character at (cur_row, cur_col).
 */
BracketMatch Syntax_FindMatchingBracket(const Line *lines, size_t line_count, size_t cur_row, size_t cur_col);

/**
 * Propagates multiline comment state (`/* ... * /`) across the entire line collection.
 */
void Syntax_UpdateMultilineComments(Line *lines, size_t line_count);

/**
 * Renders a token span with optional additive glow.
 */
void Syntax_DrawToken(Font font, const Line *line, size_t start, size_t len,
                      float start_x, float start_y, Color color, const Theme *theme);

/**
 * Lexes and draws an entire line of code with full syntax styling and glow effects.
 */
void Syntax_DrawLine(Font font, const Line *line, float start_x, float start_y, const Theme *theme);

#endif // CE_BUFFER_SYNTAX_H