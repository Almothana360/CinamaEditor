#include "editor/editor.h"
#include "buffer/syntax.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void Editor_Free(Editor *ed) {
    if (!ed) return;
    for (size_t i = 0; i < ed->line_count; ++i) {
        Line_Free(&ed->lines[i]);
    }
    free(ed->lines);
    ed->lines = NULL;
    ed->line_count = 0;
    ed->line_capacity = 0;
}

void Editor_AddLine(Editor *ed, Line line) {
    if (!ed) return;
    if (ed->line_count >= ed->line_capacity) {
        size_t new_cap = (ed->line_capacity == 0) ? 32 : ed->line_capacity * 2;
        Line *nl = (Line *)realloc(ed->lines, new_cap * sizeof(Line));
        if (nl) {
            ed->lines = nl;
            ed->line_capacity = new_cap;
        }
    }
    ed->lines[ed->line_count++] = line;
}

void Editor_InitEmpty(Editor *ed) {
    if (!ed) return;
    Editor_Free(ed);
    Line line;
    Line_Init(&line);
    Editor_AddLine(ed, line);
    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->has_selection = false;
    ed->anchor_row = 0;
    ed->anchor_col = 0;
    ed->file_path[0] = '\0';
    ed->modified = false;
}

bool Editor_LoadFile(Editor *ed, const char *filepath) {
    if (!ed || !filepath) return false;
    FILE *f = fopen(filepath, "rb");
    if (!f) return false;

    Editor_Free(ed);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) {
        fclose(f);
        Editor_InitEmpty(ed);
        strncpy(ed->file_path, filepath, sizeof(ed->file_path) - 1);
        ed->file_path[sizeof(ed->file_path) - 1] = '\0';
        ed->modified = false;
        return true;
    }

    char *buf = (char *)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return false;
    }

    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = '\0';
    fclose(f);

    size_t start = 0;
    for (size_t i = 0; i <= rd; ++i) {
        if (buf[i] == '\n' || buf[i] == '\0') {
            size_t len = i - start;
            if (len > 0 && buf[start + len - 1] == '\r') {
                len--;
            }
            Line line;
            Line_Init(&line);
            Line_AppendStr(&line, &buf[start], len);
            Editor_AddLine(ed, line);
            start = i + 1;
        }
    }
    free(buf);

    if (ed->line_count == 0) {
        Editor_InitEmpty(ed);
    }

    strncpy(ed->file_path, filepath, sizeof(ed->file_path) - 1);
    ed->file_path[sizeof(ed->file_path) - 1] = '\0';
    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->has_selection = false;
    ed->modified = false;
    Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
    return true;
}

bool Editor_SaveFile(Editor *ed) {
    if (!ed) return false;
    if (strlen(ed->file_path) == 0) {
        strncpy(ed->file_path, "untitled.c", sizeof(ed->file_path) - 1);
        ed->file_path[sizeof(ed->file_path) - 1] = '\0';
    }

    FILE *f = fopen(ed->file_path, "wb");
    if (!f) return false;

    for (size_t i = 0; i < ed->line_count; ++i) {
        if (ed->lines[i].size > 0) {
            fwrite(ed->lines[i].chars, 1, ed->lines[i].size, f);
        }
        if (i + 1 < ed->line_count) {
            fputc('\n', f);
        }
    }
    fclose(f);
    ed->modified = false;
    return true;
}

void Editor_GetSelectionBounds(const Editor *ed, size_t *sr, size_t *sc, size_t *er, size_t *ec) {
    if (!ed || !sr || !sc || !er || !ec) return;
    if (!ed->has_selection) {
        *sr = *er = ed->cursor_row;
        *sc = *ec = ed->cursor_col;
        return;
    }

    if (ed->anchor_row < ed->cursor_row || (ed->anchor_row == ed->cursor_row && ed->anchor_col <= ed->cursor_col)) {
        *sr = ed->anchor_row; *sc = ed->anchor_col;
        *er = ed->cursor_row; *ec = ed->cursor_col;
    } else {
        *sr = ed->cursor_row; *sc = ed->cursor_col;
        *er = ed->anchor_row; *ec = ed->anchor_col;
    }
}

void Editor_ClearSelection(Editor *ed) {
    if (!ed) return;
    ed->has_selection = false;
    ed->anchor_row = ed->cursor_row;
    ed->anchor_col = ed->cursor_col;
}

void Editor_SelectAll(Editor *ed) {
    if (!ed || ed->line_count == 0) return;
    ed->anchor_row = 0;
    ed->anchor_col = 0;
    ed->cursor_row = ed->line_count - 1;
    ed->cursor_col = ed->lines[ed->cursor_row].size;
    ed->has_selection = true;
}

void Editor_DeleteSelection(Editor *ed) {
    if (!ed || !ed->has_selection) return;

    size_t sr, sc, er, ec;
    Editor_GetSelectionBounds(ed, &sr, &sc, &er, &ec);
    if (sr == er && sc == ec) {
        Editor_ClearSelection(ed);
        return;
    }

    if (sr == er) {
        Line *l = &ed->lines[sr];
        memmove(&l->chars[sc], &l->chars[ec], l->size - ec);
        l->size -= (ec - sc);
        l->chars[l->size] = '\0';
    } else {
        Line *first = &ed->lines[sr];
        Line *last = &ed->lines[er];
        size_t tail_len = last->size - ec;

        first->size = sc;
        if (tail_len > 0) {
            Line_AppendStr(first, &last->chars[ec], tail_len);
        } else {
            first->chars[first->size] = '\0';
        }

        for (size_t r = sr + 1; r <= er; ++r) {
            Line_Free(&ed->lines[r]);
        }
        memmove(&ed->lines[sr + 1], &ed->lines[er + 1], (ed->line_count - (er + 1)) * sizeof(Line));
        ed->line_count -= (er - sr);
    }

    ed->cursor_row = sr;
    ed->cursor_col = sc;
    ed->has_selection = false;
    ed->modified = true;
}

void Editor_CopySelection(const Editor *ed) {
    if (!ed) return;
    size_t sr, sc, er, ec;
    Editor_GetSelectionBounds(ed, &sr, &sc, &er, &ec);

    if (!ed->has_selection || (sr == er && sc == ec)) {
        if (ed->cursor_row < ed->line_count && ed->lines[ed->cursor_row].chars) {
            SetClipboardText(ed->lines[ed->cursor_row].chars);
        }
        return;
    }

    size_t total_len = 0;
    for (size_t r = sr; r <= er; ++r) {
        size_t start = (r == sr) ? sc : 0;
        size_t end = (r == er) ? ec : ed->lines[r].size;
        total_len += (end - start) + (r < er ? 1 : 0);
    }

    char *clip_buf = (char *)malloc(total_len + 1);
    if (!clip_buf) return;

    size_t offset = 0;
    for (size_t r = sr; r <= er; ++r) {
        size_t start = (r == sr) ? sc : 0;
        size_t end = (r == er) ? ec : ed->lines[r].size;
        size_t len = end - start;
        if (len > 0) {
            memcpy(&clip_buf[offset], &ed->lines[r].chars[start], len);
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

void Editor_CutSelection(Editor *ed) {
    if (!ed) return;
    Editor_CopySelection(ed);
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
    } else {
        if (ed->line_count > 1) {
            Line_Free(&ed->lines[ed->cursor_row]);
            memmove(&ed->lines[ed->cursor_row],
                    &ed->lines[ed->cursor_row + 1],
                    (ed->line_count - (ed->cursor_row + 1)) * sizeof(Line));
            ed->line_count--;
            if (ed->cursor_row >= ed->line_count) ed->cursor_row = ed->line_count - 1;
            if (ed->cursor_col > ed->lines[ed->cursor_row].size) ed->cursor_col = ed->lines[ed->cursor_row].size;
        } else {
            ed->lines[0].size = 0;
            if (ed->lines[0].chars) ed->lines[0].chars[0] = '\0';
            ed->cursor_col = 0;
        }
        ed->modified = true;
    }
}

void Editor_PasteClipboard(Editor *ed) {
    if (!ed) return;
    const char *clip = GetClipboardText();
    if (!clip || strlen(clip) == 0) return;

    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
    }

    while (*clip) {
        if (*clip == '\r') {
            clip++;
        } else if (*clip == '\n') {
            Editor_InsertNewline(ed);
            clip++;
        } else if (*clip == '\t') {
            Editor_InsertChar(ed, '\t');
            clip++;
        } else {
            Editor_InsertChar(ed, *clip++);
        }
    }
}

void Editor_InsertChar(Editor *ed, char c) {
    if (!ed) return;
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
    }
    Line_InsertChar(&ed->lines[ed->cursor_row], ed->cursor_col, c);
    ed->cursor_col++;
    ed->modified = true;
    Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
}

void Editor_InsertNewline(Editor *ed) {
    if (!ed) return;
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
    }

    if (ed->line_count >= ed->line_capacity) {
        size_t new_cap = (ed->line_capacity == 0) ? 32 : ed->line_capacity * 2;
        Line *nl = (Line *)realloc(ed->lines, new_cap * sizeof(Line));
        if (!nl) return;
        ed->lines = nl;
        ed->line_capacity = new_cap;
    }

    memmove(&ed->lines[ed->cursor_row + 2],
            &ed->lines[ed->cursor_row + 1],
            (ed->line_count - (ed->cursor_row + 1)) * sizeof(Line));

    Line *curr = &ed->lines[ed->cursor_row];
    Line *next = &ed->lines[ed->cursor_row + 1];
    Line_Init(next);

    size_t indent_len = 0;
    while (indent_len < curr->size && (curr->chars[indent_len] == ' ' || curr->chars[indent_len] == '\t')) {
        indent_len++;
    }
    if (indent_len > ed->cursor_col) {
        indent_len = ed->cursor_col;
    }

    bool extra_indent = (ed->cursor_col > 0 && curr->chars[ed->cursor_col - 1] == '{');
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

    size_t tail_len = curr->size > ed->cursor_col ? curr->size - ed->cursor_col : 0;
    Line_Reserve(next, total_indent + tail_len);
    if (total_indent > 0) {
        memcpy(next->chars, indent_buf, total_indent);
        next->size = total_indent;
    }
    if (tail_len > 0) {
        memcpy(&next->chars[next->size], &curr->chars[ed->cursor_col], tail_len);
        next->size += tail_len;
    }
    next->chars[next->size] = '\0';

    curr->size = ed->cursor_col;
    curr->chars[curr->size] = '\0';

    ed->line_count++;
    ed->cursor_row++;
    ed->cursor_col = total_indent;
    ed->modified = true;
    Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
}

void Editor_Backspace(Editor *ed) {
    if (!ed) return;
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
        Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
        return;
    }

    if (ed->cursor_col > 0) {
        Line_DeleteChar(&ed->lines[ed->cursor_row], ed->cursor_col - 1);
        ed->cursor_col--;
        ed->modified = true;
    } else if (ed->cursor_row > 0) {
        Line *prev = &ed->lines[ed->cursor_row - 1];
        Line *curr = &ed->lines[ed->cursor_row];
        size_t prev_len = prev->size;

        if (curr->size > 0) {
            Line_AppendStr(prev, curr->chars, curr->size);
        }
        Line_Free(curr);
        memmove(&ed->lines[ed->cursor_row],
                &ed->lines[ed->cursor_row + 1],
                (ed->line_count - (ed->cursor_row + 1)) * sizeof(Line));
        ed->line_count--;
        ed->cursor_row--;
        ed->cursor_col = prev_len;
        ed->modified = true;
    }
    Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
}

void Editor_Delete(Editor *ed) {
    if (!ed) return;
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
        Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
        return;
    }

    Line *curr = &ed->lines[ed->cursor_row];
    if (ed->cursor_col < curr->size) {
        Line_DeleteChar(curr, ed->cursor_col);
        ed->modified = true;
        Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
    } else if (ed->cursor_row + 1 < ed->line_count) {
        Line *next = &ed->lines[ed->cursor_row + 1];
        Line_AppendStr(curr, next->chars, next->size);
        Line_Free(next);
        memmove(&ed->lines[ed->cursor_row + 1],
                &ed->lines[ed->cursor_row + 2],
                (ed->line_count - (ed->cursor_row + 2)) * sizeof(Line));
        ed->line_count--;
        ed->modified = true;
        Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
    }
}

void Editor_MoveWordLeft(Editor *ed) {
    if (!ed) return;
    Line *l = &ed->lines[ed->cursor_row];
    if (ed->cursor_col == 0) {
        if (ed->cursor_row > 0) {
            ed->cursor_row--;
            ed->cursor_col = ed->lines[ed->cursor_row].size;
        }
        return;
    }

    size_t col = ed->cursor_col;
    while (col > 0 && isspace((unsigned char)l->chars[col - 1])) col--;
    while (col > 0 && !isspace((unsigned char)l->chars[col - 1])) col--;
    ed->cursor_col = col;
}

void Editor_MoveWordRight(Editor *ed) {
    if (!ed) return;
    Line *l = &ed->lines[ed->cursor_row];
    if (ed->cursor_col >= l->size) {
        if (ed->cursor_row + 1 < ed->line_count) {
            ed->cursor_row++;
            ed->cursor_col = 0;
        }
        return;
    }

    size_t col = ed->cursor_col;
    while (col < l->size && !isspace((unsigned char)l->chars[col])) col++;
    while (col < l->size && isspace((unsigned char)l->chars[col])) col++;
    ed->cursor_col = col;
}

void Editor_DeleteWordBackward(Editor *ed) {
    if (!ed) return;
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
        return;
    }
    if (ed->cursor_col == 0 && ed->cursor_row == 0) return;

    ed->anchor_row = ed->cursor_row;
    ed->anchor_col = ed->cursor_col;
    Editor_MoveWordLeft(ed);
    ed->has_selection = true;
    Editor_DeleteSelection(ed);
}

void Editor_DeleteWordForward(Editor *ed) {
    if (!ed) return;
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
        return;
    }
    Line *l = &ed->lines[ed->cursor_row];
    if (ed->cursor_col >= l->size && ed->cursor_row + 1 >= ed->line_count) return;

    ed->anchor_row = ed->cursor_row;
    ed->anchor_col = ed->cursor_col;
    Editor_MoveWordRight(ed);
    ed->has_selection = true;
    Editor_DeleteSelection(ed);
}

void Editor_DuplicateLine(Editor *ed) {
    if (!ed) return;
    if (ed->has_selection) {
        Editor_CopySelection(ed);
        Editor_PasteClipboard(ed);
        return;
    }

    Line *curr = &ed->lines[ed->cursor_row];
    Line dup;
    Line_Init(&dup);
    Line_AppendStr(&dup, curr->chars, curr->size);

    if (ed->line_count >= ed->line_capacity) {
        size_t new_cap = (ed->line_capacity == 0) ? 32 : ed->line_capacity * 2;
        Line *nl = (Line *)realloc(ed->lines, new_cap * sizeof(Line));
        if (nl) {
            ed->lines = nl;
            ed->line_capacity = new_cap;
        }
    }

    memmove(&ed->lines[ed->cursor_row + 2],
            &ed->lines[ed->cursor_row + 1],
            (ed->line_count - (ed->cursor_row + 1)) * sizeof(Line));
    ed->lines[ed->cursor_row + 1] = dup;
    ed->line_count++;
    ed->cursor_row++;
    ed->modified = true;
    Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
}

bool Editor_GetCompletedToken(const Editor *ed, char *out_token, size_t max_len, size_t *out_start_col, size_t *out_end_col) {
    if (!ed || ed->line_count == 0 || ed->cursor_row >= ed->line_count) return false;
    const Line *l = &ed->lines[ed->cursor_row];
    if (l->size == 0 || ed->cursor_col == 0) return false;

    size_t end = ed->cursor_col;
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

void SmoothCursor_Init(SmoothCursor *cursor, float line_height) {
    if (!cursor) return;
    cursor->current = (Vector2){ 0.0f, 4.0f };
    cursor->target = (Vector2){ 0.0f, 4.0f };
    cursor->trail = (Vector2){ 0.0f, 4.0f };
    cursor->width = 3.0f;
    cursor->height = line_height - 6.0f;
}

void SmoothCursor_Update(SmoothCursor *cursor, Vector2 target, float dt) {
    if (!cursor) return;
    cursor->target = target;
    cursor->trail = Vector2Lerp(cursor->trail, cursor->current, 10.0f * dt);
    cursor->current = Vector2Lerp(cursor->current, cursor->target, 22.0f * dt);
}