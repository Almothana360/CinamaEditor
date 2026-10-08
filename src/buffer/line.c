#include "buffer/line.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

void Line_Init(Line *line) {
    if (!line) return;
    line->chars = NULL;
    line->size = 0;
    line->capacity = 0;
    line->starts_in_comment = false;
}

void Line_Free(Line *line) {
    if (!line) return;
    if (line->chars) {
        free(line->chars);
        line->chars = NULL;
    }
    line->size = 0;
    line->capacity = 0;
    line->starts_in_comment = false;
}

void Line_Reserve(Line *line, size_t needed) {
    if (!line) return;
    if (needed + 1 > line->capacity) {
        size_t new_cap = (line->capacity == 0) ? 16 : line->capacity * 2;
        while (new_cap < needed + 1) {
            new_cap *= 2;
        }
        char *nc = (char *)realloc(line->chars, new_cap);
        if (nc) {
            line->chars = nc;
            line->capacity = new_cap;
        }
    }
}

void Line_InsertChar(Line *line, size_t col, char c) {
    if (!line) return;
    Line_Reserve(line, line->size + 1);
    if (!line->chars) return;
    if (col > line->size) col = line->size;
    memmove(&line->chars[col + 1], &line->chars[col], line->size - col);
    line->chars[col] = c;
    line->size++;
    line->chars[line->size] = '\0';
}

void Line_DeleteChar(Line *line, size_t col) {
    if (!line || !line->chars || col >= line->size) return;
    memmove(&line->chars[col], &line->chars[col + 1], line->size - col);
    line->size--;
    line->chars[line->size] = '\0';
}

void Line_AppendStr(Line *line, const char *str, size_t len) {
    if (!line || !str || len == 0) return;
    Line_Reserve(line, line->size + len);
    if (!line->chars) return;
    memcpy(&line->chars[line->size], str, len);
    line->size += len;
    line->chars[line->size] = '\0';
}

float Line_GetColX(Font font, const Line *line, size_t col, float font_size, float spacing) {
    if (!line || !line->chars || col == 0 || line->size == 0) return 0.0f;
    if (col > line->size) col = line->size;

    bool has_tab = false;
    for (size_t i = 0; i < col; ++i) {
        if (line->chars[i] == '\t') {
            has_tab = true;
            break;
        }
    }

    if (!has_tab) {
        char saved = line->chars[col];
        ((char *)line->chars)[col] = '\0';
        Vector2 sz = MeasureTextEx(font, line->chars, font_size, spacing);
        ((char *)line->chars)[col] = saved;
        return sz.x;
    }

    size_t max_exp = col * CE_TAB_SIZE + 1;
    char stack_buf[512];
    char *expanded = stack_buf;
    if (max_exp > sizeof(stack_buf)) {
        expanded = (char *)malloc(max_exp);
        if (!expanded) return 0.0f;
    }

    size_t vcol = 0;
    for (size_t i = 0; i < col; ++i) {
        if (line->chars[i] == '\t') {
            size_t num_spaces = CE_TAB_SIZE - (vcol % CE_TAB_SIZE);
            for (size_t s = 0; s < num_spaces; ++s) {
                expanded[vcol++] = ' ';
            }
        } else {
            expanded[vcol++] = line->chars[i];
        }
    }
    expanded[vcol] = '\0';

    Vector2 sz = MeasureTextEx(font, expanded, font_size, spacing);
    if (expanded != stack_buf) free(expanded);
    return sz.x;
}