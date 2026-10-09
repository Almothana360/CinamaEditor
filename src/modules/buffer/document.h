#ifndef CE_MODULES_BUFFER_DOCUMENT_H
#define CE_MODULES_BUFFER_DOCUMENT_H

#include "raylib.h"
#include "core/types.h"
#include "buffer/line.h"
#include <stddef.h>
#include <stdbool.h>

// Core text document state.
typedef struct {
    Line *lines;
    size_t line_count;
    size_t line_capacity;
    size_t cursor_row;
    size_t cursor_col;

    bool has_selection;
    size_t anchor_row;
    size_t anchor_col;

    char file_path[512];
    bool modified;
} Document;

// Binds the active document to the Event Bus and starts listening for commands.
void Document_Init(Document *doc);

// Initializes the document with a single empty line.
void Document_InitEmpty(Document *doc);

// Frees all allocated lines and buffers.
void Document_Free(Document *doc);

// Appends a new line buffer to the document collection.
void Document_AddLine(Document *doc, Line line);

void Document_GetSelectionBounds(const Document *doc, size_t *sr, size_t *sc, size_t *er, size_t *ec);
void Document_ClearSelection(Document *doc);
void Document_SelectAll(Document *doc);
void Document_DeleteSelection(Document *doc);

void Document_CopySelection(const Document *doc);
void Document_CutSelection(Document *doc);
void Document_PasteClipboard(Document *doc);

void Document_InsertChar(Document *doc, char c);
void Document_InsertNewline(Document *doc);
void Document_Backspace(Document *doc);
void Document_Delete(Document *doc);
void Document_DuplicateLine(Document *doc);

void Document_MoveWordLeft(Document *doc);
void Document_MoveWordRight(Document *doc);
void Document_DeleteWordBackward(Document *doc);
void Document_DeleteWordForward(Document *doc);

bool Document_GetCompletedToken(const Document *doc, char *out_token, size_t max_len, size_t *out_start_col, size_t *out_end_col);

#endif // CE_MODULES_BUFFER_DOCUMENT_H