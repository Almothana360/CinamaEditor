#include "modules/buffer/document.h"
#include "core/event.h"
#include "buffer/syntax.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static Document *g_active_doc = NULL;
static float g_wheel_accum = 0.0f;

// Event Bus Listener for pure text & cursor mutations
static void Document_OnAction(EventType type, const void *payload) {
    if (type != EV_ACTION || !payload || !g_active_doc) return;
    const ActionPayload *p = (const ActionPayload *)payload;
    Document *doc = g_active_doc;

    bool shift = p->shift_held;
    bool ctrl = p->ctrl_held;

    // Manage selection boundaries before movement
    bool is_movement = (p->action >= ACTION_MOVE_LEFT && p->action <= ACTION_MOVE_WORD_RIGHT);
    if (is_movement) {
        if (shift && !doc->has_selection) {
            doc->anchor_row = doc->cursor_row;
            doc->anchor_col = doc->cursor_col;
            doc->has_selection = true;
        } else if (!shift && doc->has_selection) {
            Document_ClearSelection(doc);
        }
    }

    size_t old_row = doc->cursor_row;
    size_t old_col = doc->cursor_col;
    bool text_changed = false;

    switch (p->action) {
        case ACTION_COPY: Document_CopySelection(doc); break;
        case ACTION_CUT: Document_CutSelection(doc); text_changed = true; break;
        case ACTION_PASTE: Document_PasteClipboard(doc); text_changed = true; break;
        case ACTION_SELECT_ALL: Document_SelectAll(doc); break;
        case ACTION_DUPLICATE_LINE: Document_DuplicateLine(doc); text_changed = true; break;

        case ACTION_DELETE_WORD_BACKWARD: Document_DeleteWordBackward(doc); text_changed = true; break;
        case ACTION_DELETE_WORD_FORWARD: Document_DeleteWordForward(doc); text_changed = true; break;
        case ACTION_INSERT_NEWLINE: Document_InsertNewline(doc); text_changed = true; break;
        case ACTION_DELETE_BACKWARD: Document_Backspace(doc); text_changed = true; break;
        case ACTION_DELETE_FORWARD: Document_Delete(doc); text_changed = true; break;
        case ACTION_INSERT_CHAR: Document_InsertChar(doc, p->char_data); text_changed = true; break;

        case ACTION_MOVE_LEFT:
            if (doc->cursor_col > 0) doc->cursor_col--;
            else if (doc->cursor_row > 0) {
                doc->cursor_row--;
                doc->cursor_col = doc->lines[doc->cursor_row].size;
            }
            break;
        case ACTION_MOVE_RIGHT:
            if (doc->cursor_col < doc->lines[doc->cursor_row].size) doc->cursor_col++;
            else if (doc->cursor_row + 1 < doc->line_count) {
                doc->cursor_row++;
                doc->cursor_col = 0;
            }
            break;
        case ACTION_MOVE_UP:
            if (doc->cursor_row > 0) {
                doc->cursor_row--;
                if (doc->cursor_col > doc->lines[doc->cursor_row].size)
                    doc->cursor_col = doc->lines[doc->cursor_row].size;
            }
            break;
        case ACTION_MOVE_DOWN:
            if (doc->cursor_row + 1 < doc->line_count) {
                doc->cursor_row++;
                if (doc->cursor_col > doc->lines[doc->cursor_row].size)
                    doc->cursor_col = doc->lines[doc->cursor_row].size;
            }
            break;
        case ACTION_MOVE_HOME:
            if (ctrl) { doc->cursor_row = 0; doc->cursor_col = 0; }
            else { doc->cursor_col = 0; }
            break;
        case ACTION_MOVE_END:
            if (ctrl) { doc->cursor_row = doc->line_count - 1; doc->cursor_col = doc->lines[doc->cursor_row].size; }
            else { doc->cursor_col = doc->lines[doc->cursor_row].size; }
            break;
        case ACTION_MOVE_WORD_LEFT: Document_MoveWordLeft(doc); break;
        case ACTION_MOVE_WORD_RIGHT: Document_MoveWordRight(doc); break;

        case ACTION_SCROLL:
            if (!ctrl && doc->line_count > 0) {
                g_wheel_accum += p->float_data * 3.0f;
                int lines_to_scroll = (int)g_wheel_accum;
                if (lines_to_scroll != 0) {
                    g_wheel_accum -= (float)lines_to_scroll;
                    int new_row = (int)doc->cursor_row - lines_to_scroll;
                    if (new_row < 0) new_row = 0;
                    if (new_row >= (int)doc->line_count) new_row = (int)doc->line_count - 1;

                    if (shift && !doc->has_selection) {
                        doc->anchor_row = doc->cursor_row;
                        doc->anchor_col = doc->cursor_col;
                        doc->has_selection = true;
                    } else if (!shift && doc->has_selection) {
                        Document_ClearSelection(doc);
                    }

                    doc->cursor_row = (size_t)new_row;
                    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
                        doc->cursor_col = doc->lines[doc->cursor_row].size;
                    }
                }
            }
            break;

        default: break;
    }

    if (is_movement || p->action == ACTION_SCROLL) {
        if (shift && doc->cursor_row == doc->anchor_row && doc->cursor_col == doc->anchor_col) {
            doc->has_selection = false;
        }
    }

    if (text_changed) {
        TextChangedPayload tc = { doc->line_count, false };
        Event_Emit(EV_TEXT_CHANGED, &tc);
    }

    if (doc->cursor_row != old_row || doc->cursor_col != old_col) {
        CursorMovedPayload cm = { doc->cursor_row, doc->cursor_col };
        Event_Emit(EV_CURSOR_MOVED, &cm);
    }
}

void Document_Init(Document *doc) {
    g_active_doc = doc;
    Event_Subscribe(EV_ACTION, Document_OnAction);
}

void Document_Free(Document *doc) {
    if (!doc) return;
    for (size_t i = 0; i < doc->line_count; ++i) {
        Line_Free(&doc->lines[i]);
    }
    free(doc->lines);
    doc->lines = NULL;
    doc->line_count = 0;
    doc->line_capacity = 0;
}

void Document_AddLine(Document *doc, Line line) {
    if (!doc) return;
    if (doc->line_count >= doc->line_capacity) {
        size_t new_cap = (doc->line_capacity == 0) ? 32 : doc->line_capacity * 2;
        Line *nl = (Line *)realloc(doc->lines, new_cap * sizeof(Line));
        if (nl) {
            doc->lines = nl;
            doc->line_capacity = new_cap;
        }
    }
    doc->lines[doc->line_count++] = line;
}

void Document_InitEmpty(Document *doc) {
    if (!doc) return;
    Document_Free(doc);
    Line line;
    Line_Init(&line);
    Document_AddLine(doc, line);
    doc->cursor_row = 0;
    doc->cursor_col = 0;
    doc->has_selection = false;
    doc->anchor_row = 0;
    doc->anchor_col = 0;
    doc->file_path[0] = '\0';
    doc->modified = false;
}

void Document_GetSelectionBounds(const Document *doc, size_t *sr, size_t *sc, size_t *er, size_t *ec) {
    if (!doc || !sr || !sc || !er || !ec) return;
    if (!doc->has_selection) {
        *sr = *er = doc->cursor_row;
        *sc = *ec = doc->cursor_col;
        return;
    }
    if (doc->anchor_row < doc->cursor_row || (doc->anchor_row == doc->cursor_row && doc->anchor_col <= doc->cursor_col)) {
        *sr = doc->anchor_row; *sc = doc->anchor_col;
        *er = doc->cursor_row; *ec = doc->cursor_col;
    } else {
        *sr = doc->cursor_row; *sc = doc->cursor_col;
        *er = doc->anchor_row; *ec = doc->anchor_col;
    }
}

void Document_ClearSelection(Document *doc) {
    if (!doc) return;
    doc->has_selection = false;
    doc->anchor_row = doc->cursor_row;
    doc->anchor_col = doc->cursor_col;
}

void Document_SelectAll(Document *doc) {
    if (!doc || doc->line_count == 0) return;
    doc->anchor_row = 0;
    doc->anchor_col = 0;
    doc->cursor_row = doc->line_count - 1;
    doc->cursor_col = doc->lines[doc->cursor_row].size;
    doc->has_selection = true;
}

void Document_DeleteSelection(Document *doc) {
    if (!doc || !doc->has_selection) return;

    size_t sr, sc, er, ec;
    Document_GetSelectionBounds(doc, &sr, &sc, &er, &ec);
    if (sr == er && sc == ec) {
        Document_ClearSelection(doc);
        return;
    }

    if (sr == er) {
        Line *l = &doc->lines[sr];
        memmove(&l->chars[sc], &l->chars[ec], l->size - ec);
        l->size -= (ec - sc);
        l->chars[l->size] = '\0';
    } else {
        Line *first = &doc->lines[sr];
        Line *last = &doc->lines[er];
        size_t tail_len = last->size - ec;

        first->size = sc;
        if (tail_len > 0) {
            Line_AppendStr(first, &last->chars[ec], tail_len);
        } else {
            first->chars[first->size] = '\0';
        }

        for (size_t r = sr + 1; r <= er; ++r) {
            Line_Free(&doc->lines[r]);
        }
        memmove(&doc->lines[sr + 1], &doc->lines[er + 1], (doc->line_count - (er + 1)) * sizeof(Line));
        doc->line_count -= (er - sr);
    }

    doc->cursor_row = sr;
    doc->cursor_col = sc;
    doc->has_selection = false;
    doc->modified = true;
}

void Document_CopySelection(const Document *doc) {
    if (!doc) return;
    size_t sr, sc, er, ec;
    Document_GetSelectionBounds(doc, &sr, &sc, &er, &ec);

    if (!doc->has_selection || (sr == er && sc == ec)) {
        if (doc->cursor_row < doc->line_count && doc->lines[doc->cursor_row].chars) {
            SetClipboardText(doc->lines[doc->cursor_row].chars);
        }
        return;
    }

    size_t total_len = 0;
    for (size_t r = sr; r <= er; ++r) {
        size_t start = (r == sr) ? sc : 0;
        size_t end = (r == er) ? ec : doc->lines[r].size;
        total_len += (end - start) + (r < er ? 1 : 0);
    }

    char *clip_buf = (char *)malloc(total_len + 1);
    if (!clip_buf) return;

    size_t offset = 0;
    for (size_t r = sr; r <= er; ++r) {
        size_t start = (r == sr) ? sc : 0;
        size_t end = (r == er) ? ec : doc->lines[r].size;
        size_t len = end - start;
        if (len > 0) {
            memcpy(&clip_buf[offset], &doc->lines[r].chars[start], len);
            offset += len;
        }
        if (r < er) {
            clip_buf[offset++] = '\n';
        }
    }
    clip_buf[offset] = '\0';
    SetClipboardText(clip_buf);
    free(clip_buf);
}

void Document_CutSelection(Document *doc) {
    if (!doc) return;
    Document_CopySelection(doc);
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
    } else {
        if (doc->line_count > 1) {
            Line_Free(&doc->lines[doc->cursor_row]);
            memmove(&doc->lines[doc->cursor_row],
                    &doc->lines[doc->cursor_row + 1],
                    (doc->line_count - (doc->cursor_row + 1)) * sizeof(Line));
            doc->line_count--;
            if (doc->cursor_row >= doc->line_count) doc->cursor_row = doc->line_count - 1;
            if (doc->cursor_col > doc->lines[doc->cursor_row].size) doc->cursor_col = doc->lines[doc->cursor_row].size;
        } else {
            doc->lines[0].size = 0;
            if (doc->lines[0].chars) doc->lines[0].chars[0] = '\0';
            doc->cursor_col = 0;
        }
        doc->modified = true;
    }
}

void Document_PasteClipboard(Document *doc) {
    if (!doc) return;
    const char *clip = GetClipboardText();
    if (!clip || strlen(clip) == 0) return;

    if (doc->has_selection) {
        Document_DeleteSelection(doc);
    }

    while (*clip) {
        if (*clip == '\r') {
            clip++;
        } else if (*clip == '\n') {
            Document_InsertNewline(doc);
            clip++;
        } else if (*clip == '\t') {
            Document_InsertChar(doc, '\t');
            clip++;
        } else {
            Document_InsertChar(doc, *clip++);
        }
    }
}

void Document_InsertChar(Document *doc, char c) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
    }
    Line_InsertChar(&doc->lines[doc->cursor_row], doc->cursor_col, c);
    doc->cursor_col++;
    doc->modified = true;
    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
}

void Document_InsertNewline(Document *doc) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
    }

    if (doc->line_count >= doc->line_capacity) {
        size_t new_cap = (doc->line_capacity == 0) ? 32 : doc->line_capacity * 2;
        Line *nl = (Line *)realloc(doc->lines, new_cap * sizeof(Line));
        if (!nl) return;
        doc->lines = nl;
        doc->line_capacity = new_cap;
    }

    memmove(&doc->lines[doc->cursor_row + 2],
            &doc->lines[doc->cursor_row + 1],
            (doc->line_count - (doc->cursor_row + 1)) * sizeof(Line));

    Line *curr = &doc->lines[doc->cursor_row];
    Line *next = &doc->lines[doc->cursor_row + 1];
    Line_Init(next);

    size_t indent_len = 0;
    while (indent_len < curr->size && (curr->chars[indent_len] == ' ' || curr->chars[indent_len] == '\t')) {
        indent_len++;
    }
    if (indent_len > doc->cursor_col) {
        indent_len = doc->cursor_col;
    }

    bool extra_indent = (doc->cursor_col > 0 && curr->chars[doc->cursor_col - 1] == '{');
    char indent_buf[512];
    size_t total_indent = 0;

    if (indent_len > 0 && indent_len < sizeof(indent_buf) - CE_TAB_SIZE - 2) {
        memcpy(indent_buf, curr->chars, indent_len);
        total_indent = indent_len;
    }

    if (extra_indent && total_indent + 1 < sizeof(indent_buf) - 1) {
        indent_buf[total_indent++] = '\t';
    }
    indent_buf[total_indent] = '\0';

    size_t tail_len = curr->size > doc->cursor_col ? curr->size - doc->cursor_col : 0;
    Line_Reserve(next, total_indent + tail_len);
    if (total_indent > 0) {
        memcpy(next->chars, indent_buf, total_indent);
        next->size = total_indent;
    }
    if (tail_len > 0) {
        memcpy(&next->chars[next->size], &curr->chars[doc->cursor_col], tail_len);
        next->size += tail_len;
    }
    next->chars[next->size] = '\0';

    curr->size = doc->cursor_col;
    curr->chars[curr->size] = '\0';

    doc->line_count++;
    doc->cursor_row++;
    doc->cursor_col = total_indent;
    doc->modified = true;
    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
}

void Document_Backspace(Document *doc) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
        Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
        return;
    }

    if (doc->cursor_col > 0) {
        Line_DeleteChar(&doc->lines[doc->cursor_row], doc->cursor_col - 1);
        doc->cursor_col--;
        doc->modified = true;
    } else if (doc->cursor_row > 0) {
        Line *prev = &doc->lines[doc->cursor_row - 1];
        Line *curr = &doc->lines[doc->cursor_row];
        size_t prev_len = prev->size;

        if (curr->size > 0) {
            Line_AppendStr(prev, curr->chars, curr->size);
        }
        Line_Free(curr);
        memmove(&doc->lines[doc->cursor_row],
                &doc->lines[doc->cursor_row + 1],
                (doc->line_count - (doc->cursor_row + 1)) * sizeof(Line));
        doc->line_count--;
        doc->cursor_row--;
        doc->cursor_col = prev_len;
        doc->modified = true;
    }
    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
}

void Document_Delete(Document *doc) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
        Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
        return;
    }

    Line *curr = &doc->lines[doc->cursor_row];
    if (doc->cursor_col < curr->size) {
        Line_DeleteChar(curr, doc->cursor_col);
        doc->modified = true;
        Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
    } else if (doc->cursor_row + 1 < doc->line_count) {
        Line *next = &doc->lines[doc->cursor_row + 1];
        Line_AppendStr(curr, next->chars, next->size);
        Line_Free(next);
        memmove(&doc->lines[doc->cursor_row + 1],
                &doc->lines[doc->cursor_row + 2],
                (doc->line_count - (doc->cursor_row + 2)) * sizeof(Line));
        doc->line_count--;
        doc->modified = true;
        Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
    }
}

void Document_MoveWordLeft(Document *doc) {
    if (!doc) return;
    Line *l = &doc->lines[doc->cursor_row];
    if (doc->cursor_col == 0) {
        if (doc->cursor_row > 0) {
            doc->cursor_row--;
            doc->cursor_col = doc->lines[doc->cursor_row].size;
        }
        return;
    }

    size_t col = doc->cursor_col;
    while (col > 0 && isspace((unsigned char)l->chars[col - 1])) col--;
    while (col > 0 && !isspace((unsigned char)l->chars[col - 1])) col--;
    doc->cursor_col = col;
}

void Document_MoveWordRight(Document *doc) {
    if (!doc) return;
    Line *l = &doc->lines[doc->cursor_row];
    if (doc->cursor_col >= l->size) {
        if (doc->cursor_row + 1 < doc->line_count) {
            doc->cursor_row++;
            doc->cursor_col = 0;
        }
        return;
    }

    size_t col = doc->cursor_col;
    while (col < l->size && !isspace((unsigned char)l->chars[col])) col++;
    while (col < l->size && isspace((unsigned char)l->chars[col])) col++;
    doc->cursor_col = col;
}

void Document_DeleteWordBackward(Document *doc) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
        return;
    }
    if (doc->cursor_col == 0 && doc->cursor_row == 0) return;

    doc->anchor_row = doc->cursor_row;
    doc->anchor_col = doc->cursor_col;
    Document_MoveWordLeft(doc);
    doc->has_selection = true;
    Document_DeleteSelection(doc);
}

void Document_DeleteWordForward(Document *doc) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
        return;
    }
    Line *l = &doc->lines[doc->cursor_row];
    if (doc->cursor_col >= l->size && doc->cursor_row + 1 >= doc->line_count) return;

    doc->anchor_row = doc->cursor_row;
    doc->anchor_col = doc->cursor_col;
    Document_MoveWordRight(doc);
    doc->has_selection = true;
    Document_DeleteSelection(doc);
}

void Document_DuplicateLine(Document *doc) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_CopySelection(doc);
        Document_PasteClipboard(doc);
        return;
    }

    Line *curr = &doc->lines[doc->cursor_row];
    Line dup;
    Line_Init(&dup);
    Line_AppendStr(&dup, curr->chars, curr->size);

    if (doc->line_count >= doc->line_capacity) {
        size_t new_cap = (doc->line_capacity == 0) ? 32 : doc->line_capacity * 2;
        Line *nl = (Line *)realloc(doc->lines, new_cap * sizeof(Line));
        if (nl) {
            doc->lines = nl;
            doc->line_capacity = new_cap;
        }
    }

    memmove(&doc->lines[doc->cursor_row + 2],
            &doc->lines[doc->cursor_row + 1],
            (doc->line_count - (doc->cursor_row + 1)) * sizeof(Line));
    doc->lines[doc->cursor_row + 1] = dup;
    doc->line_count++;
    doc->cursor_row++;
    doc->modified = true;
    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
}

bool Document_GetCompletedToken(const Document *doc, char *out_token, size_t max_len, size_t *out_start_col, size_t *out_end_col) {
    if (!doc || doc->line_count == 0 || doc->cursor_row >= doc->line_count) return false;
    const Line *l = &doc->lines[doc->cursor_row];
    if (l->size == 0 || doc->cursor_col == 0) return false;

    size_t end = doc->cursor_col;
    while (end > 0 && isspace((unsigned char)l->chars[end - 1])) end--;
    if (end == 0) return false;

    size_t start = end;
    while (start > 0 && (isalnum((unsigned char)l->chars[start - 1]) || l->chars[start - 1] == '_')) {
        start--;
    }
    if (start == end) return false;

    size_t wlen = end - start;
    if (wlen >= max_len) return false;

    if (out_token) {
        memcpy(out_token, &l->chars[start], wlen);
        out_token[wlen] = '\0';
    }
    if (out_start_col) *out_start_col = start;
    if (out_end_col) *out_end_col = end;
    return true;
}