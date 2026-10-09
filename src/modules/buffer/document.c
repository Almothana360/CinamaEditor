#include "modules/buffer/document.h"
#include "modules/buffer/history.h"
#include "core/event.h"
#include "buffer/syntax.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static Document *g_active_doc = NULL;
static History g_history;
static float g_wheel_accum = 0.0f;

// Replaces a slice of lines [start_row ... start_row + count_to_remove - 1]
// with count_to_insert fresh lines from new_data strings.
static void Document_ReplaceLines(Document *doc, size_t start_row, size_t count_to_remove, char **new_data, size_t count_to_insert) {
    if (!doc) return;
    if (start_row > doc->line_count) start_row = doc->line_count;
    if (start_row + count_to_remove > doc->line_count) {
        count_to_remove = doc->line_count - start_row;
    }

    // 1. Free memory of lines being removed
    for (size_t i = 0; i < count_to_remove; ++i) {
        Line_Free(&doc->lines[start_row + i]);
    }

    // 2. Adjust capacity and shift lines if document length changes
    if (count_to_insert > count_to_remove) {
        size_t diff = count_to_insert - count_to_remove;
        size_t needed = doc->line_count + diff;
        if (needed > doc->line_capacity) {
            size_t new_cap = (doc->line_capacity == 0) ? 32 : doc->line_capacity * 2;
            while (new_cap < needed) new_cap *= 2;
            Line *nl = (Line *)realloc(doc->lines, new_cap * sizeof(Line));
            if (!nl) return;
            doc->lines = nl;
            doc->line_capacity = new_cap;
        }
        size_t tail_count = doc->line_count - (start_row + count_to_remove);
        if (tail_count > 0) {
            memmove(&doc->lines[start_row + count_to_insert],
                    &doc->lines[start_row + count_to_remove],
                    tail_count * sizeof(Line));
        }
    } else if (count_to_remove > count_to_insert) {
        size_t tail_count = doc->line_count - (start_row + count_to_remove);
        if (tail_count > 0) {
            memmove(&doc->lines[start_row + count_to_insert],
                    &doc->lines[start_row + count_to_remove],
                    tail_count * sizeof(Line));
        }
    }

    // 3. Initialize and populate inserted lines
    for (size_t i = 0; i < count_to_insert; ++i) {
        Line *l = &doc->lines[start_row + i];
        Line_Init(l);
        if (new_data && new_data[i]) {
            size_t len = strlen(new_data[i]);
            Line_AppendStr(l, new_data[i], len);
        }
    }

    // 4. Update document line count
    doc->line_count = doc->line_count - count_to_remove + count_to_insert;
    if (doc->line_count == 0) {
        Line l;
        Line_Init(&l);
        Document_AddLine(doc, l);
    }

    // 5. Update syntax multiline comment cache
    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
}

// Begins tracking a transactional edit before mutations execute.
static void History_BeginEdit(History *h, const Document *doc, ActionType action, char char_data) {
    if (!h || !doc) return;

    bool is_typing = (action == ACTION_INSERT_CHAR);
    bool is_backspace = (action == ACTION_DELETE_BACKWARD);

    // Coalesce typing characters or continuous backspaces
    if (h->can_coalesce && h->undo_count > 0 && !doc->has_selection) {
        EditRecord *top = &h->undo_stack[h->undo_count - 1];
        double now = GetTime();

        if (is_typing && top->action == ACTION_INSERT_CHAR) {
            if (char_data != ' ' && char_data != '\t' &&
                doc->cursor_row == top->cursor_after_row &&
                doc->cursor_col == top->cursor_after_col &&
                (now - top->timestamp) < 0.85) {
                h->in_transaction = true;
                h->is_coalescing_active = true;
                return;
            }
        } else if (is_backspace && top->action == ACTION_DELETE_BACKWARD) {
            if (doc->cursor_row == top->cursor_after_row &&
                doc->cursor_col == top->cursor_after_col &&
                doc->cursor_col > 0 &&
                (now - top->timestamp) < 0.85) {
                h->in_transaction = true;
                h->is_coalescing_active = true;
                return;
            }
        }
    }

    // Determine the pre-mutation line range affected
    size_t start_r = doc->cursor_row;
    size_t end_r = doc->cursor_row;

    if (doc->has_selection) {
        size_t sr, sc, er, ec;
        Document_GetSelectionBounds(doc, &sr, &sc, &er, &ec);
        start_r = sr;
        end_r = er;
    } else if (action == ACTION_DELETE_BACKWARD && doc->cursor_col == 0 && doc->cursor_row > 0) {
        start_r = doc->cursor_row - 1;
        end_r = doc->cursor_row;
    } else if (action == ACTION_DELETE_FORWARD && doc->cursor_row + 1 < doc->line_count) {
        if (doc->cursor_col >= doc->lines[doc->cursor_row].size) {
            start_r = doc->cursor_row;
            end_r = doc->cursor_row + 1;
        }
    }

    h->pre_doc_line_count = doc->line_count;
    h->current_record.action = action;
    h->current_record.start_row = start_r;
    h->current_record.old_line_count = (end_r >= start_r) ? (end_r - start_r + 1) : 1;
    h->current_record.old_lines = (char **)malloc(h->current_record.old_line_count * sizeof(char *));

    for (size_t i = 0; i < h->current_record.old_line_count; ++i) {
        size_t r = start_r + i;
        const char *chars = (r < doc->line_count && doc->lines[r].chars) ? doc->lines[r].chars : "";
        h->current_record.old_lines[i] = History_StrDup(chars);
    }

    h->current_record.cursor_before_row = doc->cursor_row;
    h->current_record.cursor_before_col = doc->cursor_col;
    h->current_record.had_selection_before = doc->has_selection;
    h->current_record.anchor_before_row = doc->anchor_row;
    h->current_record.anchor_before_col = doc->anchor_col;
    h->current_record.timestamp = GetTime();

    h->in_transaction = true;
    h->is_coalescing_active = false;
}

// Commits the post-mutation delta to the history stack.
static void History_CommitEdit(History *h, const Document *doc) {
    if (!h || !doc || !h->in_transaction) return;

    if (h->is_coalescing_active) {
        EditRecord *top = &h->undo_stack[h->undo_count - 1];
        if (top->new_lines && top->new_lines[0]) {
            free(top->new_lines[0]);
        }
        const char *chars = (doc->cursor_row < doc->line_count && doc->lines[doc->cursor_row].chars)
                            ? doc->lines[doc->cursor_row].chars : "";
        top->new_lines[0] = History_StrDup(chars);
        top->cursor_after_row = doc->cursor_row;
        top->cursor_after_col = doc->cursor_col;
        top->timestamp = GetTime();

        h->in_transaction = false;
        h->is_coalescing_active = false;
        return;
    }

    long delta = (long)doc->line_count - (long)h->pre_doc_line_count;
    long new_count_l = (long)h->current_record.old_line_count + delta;
    if (new_count_l < 1) new_count_l = 1;
    size_t new_count = (size_t)new_count_l;

    h->current_record.new_line_count = new_count;
    h->current_record.new_lines = (char **)malloc(new_count * sizeof(char *));
    for (size_t i = 0; i < new_count; ++i) {
        size_t r = h->current_record.start_row + i;
        const char *chars = (r < doc->line_count && doc->lines[r].chars) ? doc->lines[r].chars : "";
        h->current_record.new_lines[i] = History_StrDup(chars);
    }

    h->current_record.cursor_after_row = doc->cursor_row;
    h->current_record.cursor_after_col = doc->cursor_col;
    h->current_record.had_selection_after = doc->has_selection;
    h->current_record.anchor_after_row = doc->anchor_row;
    h->current_record.anchor_after_col = doc->anchor_col;

    // Discard empty/no-op edits
    if (h->current_record.old_line_count == h->current_record.new_line_count) {
        bool identical = true;
        for (size_t i = 0; i < h->current_record.old_line_count; ++i) {
            if (strcmp(h->current_record.old_lines[i], h->current_record.new_lines[i]) != 0) {
                identical = false;
                break;
            }
        }
        if (identical &&
            h->current_record.cursor_before_row == h->current_record.cursor_after_row &&
            h->current_record.cursor_before_col == h->current_record.cursor_after_col) {
            History_FreeRecord(&h->current_record);
            h->in_transaction = false;
            return;
        }
    }

    History_ClearRedo(h);
    History_PushUndo(h, &h->current_record);

    h->can_coalesce = (h->current_record.action == ACTION_INSERT_CHAR ||
                       h->current_record.action == ACTION_DELETE_BACKWARD);
    h->in_transaction = false;
}

static bool Document_Undo(Document *doc) {
    if (!doc || g_history.undo_count == 0) return false;
    History_BreakCoalesce(&g_history);

    EditRecord rec = g_history.undo_stack[--g_history.undo_count];

    Document_ReplaceLines(doc, rec.start_row, rec.new_line_count, rec.old_lines, rec.old_line_count);

    doc->cursor_row = rec.cursor_before_row;
    doc->cursor_col = rec.cursor_before_col;
    doc->has_selection = rec.had_selection_before;
    doc->anchor_row = rec.anchor_before_row;
    doc->anchor_col = rec.anchor_before_col;

    if (doc->cursor_row >= doc->line_count) {
        doc->cursor_row = doc->line_count - 1;
    }
    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
        doc->cursor_col = doc->lines[doc->cursor_row].size;
    }

    if (g_history.redo_count < CE_MAX_HISTORY) {
        g_history.redo_stack[g_history.redo_count++] = rec;
    } else {
        History_FreeRecord(&rec);
    }

    doc->modified = true;
    return true;
}

static bool Document_Redo(Document *doc) {
    if (!doc || g_history.redo_count == 0) return false;
    History_BreakCoalesce(&g_history);

    EditRecord rec = g_history.redo_stack[--g_history.redo_count];

    Document_ReplaceLines(doc, rec.start_row, rec.old_line_count, rec.new_lines, rec.new_line_count);

    doc->cursor_row = rec.cursor_after_row;
    doc->cursor_col = rec.cursor_after_col;
    doc->has_selection = rec.had_selection_after;
    doc->anchor_row = rec.anchor_after_row;
    doc->anchor_col = rec.anchor_after_col;

    if (doc->cursor_row >= doc->line_count) {
        doc->cursor_row = doc->line_count - 1;
    }
    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
        doc->cursor_col = doc->lines[doc->cursor_row].size;
    }

    History_PushUndo(&g_history, &rec);

    doc->modified = true;
    return true;
}

static void Document_InsertRawNewline(Document *doc) {
    if (!doc) return;

    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
        doc->cursor_col = doc->lines[doc->cursor_row].size;
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

    size_t tail_len = (curr->size > doc->cursor_col) ? (curr->size - doc->cursor_col) : 0;
    if (tail_len > 0 && curr->chars) {
        Line_Reserve(next, tail_len);
        if (next->chars) {
            memcpy(next->chars, &curr->chars[doc->cursor_col], tail_len);
            next->size = tail_len;
            next->chars[next->size] = '\0';
        }
    } else {
        Line_Reserve(next, 0);
        if (next->chars) {
            next->chars[0] = '\0';
        }
    }

    curr->size = doc->cursor_col;
    if (curr->chars) {
        curr->chars[curr->size] = '\0';
    }

    doc->line_count++;
    doc->cursor_row++;
    doc->cursor_col = 0;
    doc->modified = true;
}

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
        History_BreakCoalesce(&g_history);
        if (shift && !doc->has_selection) {
            doc->anchor_row = doc->cursor_row;
            doc->anchor_col = doc->cursor_col;
            doc->has_selection = true;
        } else if (!shift && doc->has_selection) {
            Document_ClearSelection(doc);
        }
    }

    // Check if this action mutates document text
    bool is_mutating = (p->action == ACTION_INSERT_CHAR ||
                         p->action == ACTION_INSERT_NEWLINE ||
                         p->action == ACTION_DELETE_BACKWARD ||
                         p->action == ACTION_DELETE_FORWARD ||
                         p->action == ACTION_DELETE_WORD_BACKWARD ||
                         p->action == ACTION_DELETE_WORD_FORWARD ||
                         p->action == ACTION_CUT ||
                         p->action == ACTION_PASTE ||
                         p->action == ACTION_DUPLICATE_LINE);

    if (is_mutating) {
        History_BeginEdit(&g_history, doc, p->action, p->char_data);
    }

    size_t old_row = doc->cursor_row;
    size_t old_col = doc->cursor_col;
    bool text_changed = false;

    switch (p->action) {
        case ACTION_UNDO:
            if (Document_Undo(doc)) {
                text_changed = true;
            }
            break;
        case ACTION_REDO:
            if (Document_Redo(doc)) {
                text_changed = true;
            }
            break;

        case ACTION_COPY: Document_CopySelection(doc); break;
        case ACTION_CUT: Document_CutSelection(doc); text_changed = true; break;
        case ACTION_PASTE: Document_PasteClipboard(doc); text_changed = true; break;
        case ACTION_SELECT_ALL:
            History_BreakCoalesce(&g_history);
            Document_SelectAll(doc);
            break;
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
                History_BreakCoalesce(&g_history);
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

    if (is_mutating) {
        History_CommitEdit(&g_history, doc);
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
    History_Init(&g_history);
    Event_Subscribe(EV_ACTION, Document_OnAction);
}

void Document_Free(Document *doc) {
    if (!doc) return;
    History_Free(&g_history);
    for (size_t i = 0; i < doc->line_count; ++i) {
        Line_Free(&doc->lines[i]);
    }
    if (doc->lines) {
        free(doc->lines);
    }
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
    History_Clear(&g_history);
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
        if (l->chars) {
            memmove(&l->chars[sc], &l->chars[ec], l->size - ec);
            l->size -= (ec - sc);
            l->chars[l->size] = '\0';
        }
    } else {
        Line *first = &doc->lines[sr];
        Line *last = &doc->lines[er];
        size_t tail_len = last->size - ec;

        first->size = sc;
        if (tail_len > 0 && last->chars) {
            Line_AppendStr(first, &last->chars[ec], tail_len);
        } else if (first->chars) {
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
        if (doc->cursor_row < doc->line_count) {
            const char *src = doc->lines[doc->cursor_row].chars ? doc->lines[doc->cursor_row].chars : "";
            SetClipboardText(src);
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
        if (len > 0 && doc->lines[r].chars) {
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
            if (*clip == '\n') {
                clip++;
            }
            Document_InsertRawNewline(doc);
        } else if (*clip == '\n') {
            Document_InsertRawNewline(doc);
            clip++;
        } else {
            char c = *clip++;
            if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
                doc->cursor_col = doc->lines[doc->cursor_row].size;
            }
            Line_InsertChar(&doc->lines[doc->cursor_row], doc->cursor_col, c);
            doc->cursor_col++;
            doc->modified = true;
        }
    }

    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
}

void Document_InsertChar(Document *doc, char c) {
    if (!doc) return;
    if (doc->has_selection) {
        Document_DeleteSelection(doc);
    }

    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
        doc->cursor_col = doc->lines[doc->cursor_row].size;
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

    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
        doc->cursor_col = doc->lines[doc->cursor_row].size;
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
    while (indent_len < curr->size && curr->chars && (curr->chars[indent_len] == ' ' || curr->chars[indent_len] == '\t')) {
        indent_len++;
    }
    if (indent_len > doc->cursor_col) {
        indent_len = doc->cursor_col;
    }

    bool extra_indent = (doc->cursor_col > 0 && curr->chars && curr->chars[doc->cursor_col - 1] == '{');
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

    if (tail_len > 0 && curr->chars) {
        memcpy(&next->chars[next->size], &curr->chars[doc->cursor_col], tail_len);
        next->size += tail_len;
    }

    if (next->chars) {
        next->chars[next->size] = '\0';
    }

    curr->size = doc->cursor_col;
    if (curr->chars) {
        curr->chars[curr->size] = '\0';
    }

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

    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
        doc->cursor_col = doc->lines[doc->cursor_row].size;
    }

    if (doc->cursor_col > 0) {
        Line_DeleteChar(&doc->lines[doc->cursor_row], doc->cursor_col - 1);
        doc->cursor_col--;
        doc->modified = true;
    } else if (doc->cursor_row > 0) {
        Line *prev = &doc->lines[doc->cursor_row - 1];
        Line *curr = &doc->lines[doc->cursor_row];
        size_t prev_len = prev->size;

        if (curr->size > 0 && curr->chars) {
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

    if (doc->cursor_col > doc->lines[doc->cursor_row].size) {
        doc->cursor_col = doc->lines[doc->cursor_row].size;
    }

    Line *curr = &doc->lines[doc->cursor_row];
    if (doc->cursor_col < curr->size) {
        Line_DeleteChar(curr, doc->cursor_col);
        doc->modified = true;
        Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
    } else if (doc->cursor_row + 1 < doc->line_count) {
        Line *next = &doc->lines[doc->cursor_row + 1];
        if (next->size > 0 && next->chars) {
            Line_AppendStr(curr, next->chars, next->size);
        }
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
    if (l->chars) {
        while (col > 0 && isspace((unsigned char)l->chars[col - 1])) col--;
        while (col > 0 && !isspace((unsigned char)l->chars[col - 1])) col--;
    }
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
    if (l->chars) {
        while (col < l->size && !isspace((unsigned char)l->chars[col])) col++;
        while (col < l->size && isspace((unsigned char)l->chars[col])) col++;
    }
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
    if (curr->chars) {
        Line_AppendStr(&dup, curr->chars, curr->size);
    }

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
    if (l->size == 0 || doc->cursor_col == 0 || !l->chars) return false;

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