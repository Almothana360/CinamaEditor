#ifndef CE_EDITOR_EDITOR_H
#define CE_EDITOR_EDITOR_H

#include "raylib.h"
#include "core/types.h"
#include "buffer/line.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * Core text editor state.
 */
typedef struct {
    Line *lines;
    size_t line_count;
    size_t line_capacity;
    size_t cursor_row;
    size_t cursor_col;

    // Selection range anchor
    bool has_selection;
    size_t anchor_row;
    size_t anchor_col;

    char file_path[512];
    bool modified;
} Editor;

/**
 * Animated cursor with high-speed spring interpolation and trailing blur.
 */
typedef struct {
    Vector2 current;
    Vector2 target;
    Vector2 trail;
    float width;
    float height;
} SmoothCursor;

/* --- Lifecycle & File I/O --- */

/**
 * Initializes the editor with a single empty line.
 */
void Editor_InitEmpty(Editor *ed);

/**
 * Frees all allocated lines and buffers.
 */
void Editor_Free(Editor *ed);

/**
 * Appends a new line buffer to the editor collection.
 */
void Editor_AddLine(Editor *ed, Line line);

/**
 * Loads a file from disk into the editor buffer.
 */
bool Editor_LoadFile(Editor *ed, const char *filepath);

/**
 * Writes the active buffer contents to disk.
 */
bool Editor_SaveFile(Editor *ed);

/* --- Selection Management --- */

/**
 * Normalizes selection bounds into chronological (start_row, start_col) and (end_row, end_col).
 */
void Editor_GetSelectionBounds(const Editor *ed, size_t *sr, size_t *sc, size_t *er, size_t *ec);

/**
 * Cancels the active selection and collapses the anchor to the current cursor position.
 */
void Editor_ClearSelection(Editor *ed);

/**
 * Selects all content across the entire buffer.
 */
void Editor_SelectAll(Editor *ed);

/**
 * Deletes the text currently encompassed by the selection range.
 */
void Editor_DeleteSelection(Editor *ed);

/* --- Clipboard Operations --- */

/**
 * Copies the selected text (or current line if no selection) to the system clipboard.
 */
void Editor_CopySelection(const Editor *ed);

/**
 * Cuts the selected text (or current line) to the system clipboard.
 */
void Editor_CutSelection(Editor *ed);

/**
 * Pastes clipboard contents at the cursor, replacing any active selection.
 */
void Editor_PasteClipboard(Editor *ed);

/* --- Text Mutations & Navigation --- */

/**
 * Inserts a single printable character at the cursor position.
 */
void Editor_InsertChar(Editor *ed, char c);

/**
 * Inserts a newline preserving leading indentation (tabs and spaces) of the current line.
 */
void Editor_InsertNewline(Editor *ed);

/**
 * Deletes the character before the cursor or removes the active selection.
 */
void Editor_Backspace(Editor *ed);

/**
 * Deletes the character under the cursor or joins the next line.
 */
void Editor_Delete(Editor *ed);

/**
 * Duplicates the current line (or selection) immediately below.
 */
void Editor_DuplicateLine(Editor *ed);

/**
 * Moves cursor one word to the left (skipping whitespace and punctuation boundaries).
 */
void Editor_MoveWordLeft(Editor *ed);

/**
 * Moves cursor one word to the right.
 */
void Editor_MoveWordRight(Editor *ed);

/**
 * Deletes the word preceding the cursor.
 */
void Editor_DeleteWordBackward(Editor *ed);

/**
 * Deletes the word following the cursor.
 */
void Editor_DeleteWordForward(Editor *ed);

/**
 * Inspects whether a valid syntax word token was just completed immediately before the cursor.
 * Returns true and writes token info if an identifier/keyword is present.
 */
bool Editor_GetCompletedToken(const Editor *ed, char *out_token, size_t max_len, size_t *out_start_col, size_t *out_end_col);

/* --- Smooth Cursor Physics --- */

/**
 * Initializes smooth cursor coordinates and dimensions.
 */
void SmoothCursor_Init(SmoothCursor *cursor, float line_height);

/**
 * Interpolates smooth cursor and trailing coordinates towards target position.
 */
void SmoothCursor_Update(SmoothCursor *cursor, Vector2 target, float dt);

#endif // CE_EDITOR_EDITOR_H