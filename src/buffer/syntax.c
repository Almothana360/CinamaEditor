#include "buffer/syntax.h"
#include "core/event.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const Document *g_syn_doc = NULL;
static const Theme *g_syn_theme = NULL;
static Font g_syn_font = {0};

static void Syntax_OnEvent(EventType type, const void *payload) {
    if (type == EV_THEME_CHANGED) {
        const ThemeChangedPayload *p = payload;
        g_syn_theme = (const Theme *)p->theme;
    } else if (type == EV_ACTION) {
        const ActionPayload *p = payload;
        if (p->action == ACTION_INSERT_CHAR && p->char_data >= 32 && p->char_data <= 126) {
            if (!g_syn_doc || !g_syn_theme) return;
            char token[128];
            size_t s_col, e_col;
            if (Document_GetCompletedToken(g_syn_doc, token, sizeof(token), &s_col, &e_col)) {
                Color glow_col = Syntax_GetHighlightColor(token, g_syn_theme);
                if (glow_col.a > 0) {
                    TokenCompletedPayload tp = { g_syn_doc->cursor_row, s_col, e_col, glow_col };
                    Event_Emit(EV_TOKEN_COMPLETED, &tp);
                }
            }
        }
    }
}

void Syntax_Init(const Document *doc, Font font) {
    g_syn_doc = doc;
    g_syn_font = font;
    Event_Subscribe(EV_THEME_CHANGED, Syntax_OnEvent);
    Event_Subscribe(EV_ACTION, Syntax_OnEvent);
}

// ... Rest of your existing Syntax functions ...

bool Syntax_IsKeyword(const char *word) {
    if (!word) return false;
    static const char *kw[] = {
        "auto", "break", "case", "const", "continue", "default", "do",
        "else", "enum", "extern", "for", "goto", "if", "inline",
        "register", "restrict", "return", "sizeof", "static", "struct",
        "switch", "typedef", "union", "volatile", "while", NULL
    };
    for (int i = 0; kw[i]; ++i) {
        if (strcmp(word, kw[i]) == 0) return true;
    }
    return false;
}

bool Syntax_IsType(const char *word) {
    if (!word) return false;
    static const char *types[] = {
        "void", "char", "short", "int", "long", "float", "double",
        "signed", "unsigned", "bool", "size_t", "ssize_t", "uint8_t",
        "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t",
        "int32_t", "int64_t", "FILE", "Vector2", "Color", "Font", NULL
    };
    for (int i = 0; types[i]; ++i) {
        if (strcmp(word, types[i]) == 0) return true;
    }
    return false;
}

Color Syntax_GetHighlightColor(const char *word, const Theme *theme) {
    if (!theme || !word) return BLANK;
    if (Syntax_IsKeyword(word)) return theme->syn_keyword;
    if (Syntax_IsType(word))    return theme->syn_type;
    return BLANK;
}

BracketMatch Syntax_FindMatchingBracket(const Line *lines, size_t line_count, size_t cur_row, size_t cur_col) {
    BracketMatch match = { .found = false, .row = 0, .col = 0 };
    if (!lines || line_count == 0 || cur_row >= line_count) return match;
    const Line *cur_line = &lines[cur_row];
    if (cur_line->size == 0) return match;

    size_t c_idx = cur_col;
    if (c_idx >= cur_line->size && c_idx > 0) c_idx--;
    char c = cur_line->chars[c_idx];
    char target = 0;
    int direction = 0;

    if (c == '(') { target = ')'; direction = 1; }
    else if (c == ')') { target = '('; direction = -1; }
    else if (c == '{') { target = '}'; direction = 1; }
    else if (c == '}') { target = '{'; direction = -1; }
    else if (c == '[') { target = ']'; direction = 1; }
    else if (c == ']') { target = '['; direction = -1; }
    else return match;

    int depth = 0;
    if (direction == 1) {
        for (size_t r = cur_row; r < line_count; ++r) {
            const Line *l = &lines[r];
            size_t start_col = (r == cur_row) ? c_idx : 0;
            for (size_t col = start_col; col < l->size; ++col) {
                if (l->chars[col] == c) depth++;
                else if (l->chars[col] == target) {
                    depth--;
                    if (depth == 0) {
                        match.found = true;
                        match.row = r;
                        match.col = col;
                        return match;
                    }
                }
            }
        }
    } else {
        for (long r = (long)cur_row; r >= 0; --r) {
            const Line *l = &lines[r];
            long start_col = (r == (long)cur_row) ? (long)c_idx : (long)l->size - 1;
            for (long col = start_col; col >= 0; --col) {
                if (l->chars[col] == c) depth++;
                else if (l->chars[col] == target) {
                    depth--;
                    if (depth == 0) {
                        match.found = true;
                        match.row = (size_t)r;
                        match.col = (size_t)col;
                        return match;
                    }
                }
            }
        }
    }
    return match;
}

void Syntax_UpdateMultilineComments(Line *lines, size_t line_count) {
    if (!lines || line_count == 0) return;
    bool in_comment = false;
    for (size_t i = 0; i < line_count; ++i) {
        lines[i].starts_in_comment = in_comment;
        const char *p = lines[i].chars;
        if (!p) continue;
        size_t len = lines[i].size;
        size_t j = 0;
        while (j < len) {
            if (in_comment) {
                if (j + 1 < len && p[j] == '*' && p[j + 1] == '/') {
                    in_comment = false;
                    j += 2;
                } else {
                    j++;
                }
            } else {
                if (j + 1 < len && p[j] == '/' && p[j + 1] == '*') {
                    in_comment = true;
                    j += 2;
                } else if (j + 1 < len && p[j] == '/' && p[j + 1] == '/') {
                    break;
                } else if (p[j] == '"' || p[j] == '\'') {
                    char quote = p[j++];
                    while (j < len && p[j] != quote) {
                        if (p[j] == '\\' && j + 1 < len) j++;
                        j++;
                    }
                    if (j < len) j++;
                } else {
                    j++;
                }
            }
        }
    }
}

void Syntax_DrawToken(Font font, const Line *line, size_t start, size_t len,
                      float start_x, float start_y, Color color, const Theme *theme) {
    if (!line || !line->chars || len == 0 || !theme) return;
    float offset_x = Line_GetColX(font, line, start, CE_FONT_SIZE, CE_FONT_SPACING);

    bool has_tab = false;
    for (size_t i = 0; i < len; ++i) {
        if (line->chars[start + i] == '\t') {
            has_tab = true;
            break;
        }
    }

    char stack_buf[512];
    char *str = stack_buf;
    if (!has_tab) {
        if (len >= sizeof(stack_buf)) {
            str = (char *)malloc(len + 1);
            if (!str) return;
        }
        memcpy(str, &line->chars[start], len);
        str[len] = '\0';
    } else {
        size_t vcol = 0;
        for (size_t i = 0; i < start; ++i) {
            if (line->chars[i] == '\t') {
                vcol += CE_TAB_SIZE - (vcol % CE_TAB_SIZE);
            } else {
                vcol++;
            }
        }
        size_t max_exp = len * CE_TAB_SIZE + 1;
        if (max_exp >= sizeof(stack_buf)) {
            str = (char *)malloc(max_exp);
            if (!str) return;
        }
        size_t out_len = 0;
        for (size_t i = 0; i < len; ++i) {
            if (line->chars[start + i] == '\t') {
                size_t num_spaces = CE_TAB_SIZE - (vcol % CE_TAB_SIZE);
                for (size_t s = 0; s < num_spaces; ++s) {
                    str[out_len++] = ' ';
                }
                vcol += num_spaces;
            } else {
                str[out_len++] = line->chars[start + i];
                vcol++;
            }
        }
        str[out_len] = '\0';
    }

    Vector2 pos = { start_x + offset_x, start_y };
    if (!theme->is_light && (color.r != theme->syn_default.r || color.g != theme->syn_default.g || color.b != theme->syn_default.b)) {
        BeginBlendMode(BLEND_ADDITIVE);
        Color glow = color;
        glow.a = 48;
        DrawTextEx(font, str, (Vector2){ pos.x - 1.0f, pos.y }, CE_FONT_SIZE, CE_FONT_SPACING, glow);
        DrawTextEx(font, str, (Vector2){ pos.x + 1.0f, pos.y }, CE_FONT_SIZE, CE_FONT_SPACING, glow);
        DrawTextEx(font, str, (Vector2){ pos.x, pos.y - 1.0f }, CE_FONT_SIZE, CE_FONT_SPACING, glow);
        DrawTextEx(font, str, (Vector2){ pos.x, pos.y + 1.0f }, CE_FONT_SIZE, CE_FONT_SPACING, glow);
        EndBlendMode();
    }
    DrawTextEx(font, str, pos, CE_FONT_SIZE, CE_FONT_SPACING, color);
    if (str != stack_buf) free(str);
}

void Syntax_DrawLine(Font font, const Line *line, float start_x, float start_y, const Theme *theme) {
    if (!line || !line->chars || line->size == 0 || !theme) return;
    const char *text = line->chars;
    size_t len = line->size;
    size_t i = 0;
    bool in_comment = line->starts_in_comment;

    while (i < len) {
        if (in_comment) {
            size_t token_start = i;
            while (i < len) {
                if (i + 1 < len && text[i] == '*' && text[i + 1] == '/') {
                    i += 2;
                    in_comment = false;
                    break;
                }
                i++;
            }
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_comment, theme);
            continue;
        }

        if (text[i] == ' ' || text[i] == '\t') {
            i++;
            continue;
        }

        if (i + 1 < len && text[i] == '/' && text[i] == '/') {
            Syntax_DrawToken(font, line, i, len - i, start_x, start_y, theme->syn_comment, theme);
            break;
        }

        if (i + 1 < len && text[i] == '/' && text[i] == '*') {
            size_t token_start = i;
            i += 2;
            in_comment = true;
            while (i < len) {
                if (i + 1 < len && text[i] == '*' && text[i + 1] == '/') {
                    i += 2;
                    in_comment = false;
                    break;
                }
                i++;
            }
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_comment, theme);
            continue;
        }

        if (text[i] == '#') {
            size_t token_start = i;
            while (i < len && !isspace((unsigned char)text[i])) i++;
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_preproc, theme);
            continue;
        }

        if (text[i] == '"' || text[i] == '\'') {
            char quote = text[i];
            size_t token_start = i++;
            while (i < len && text[i] != quote) {
                if (text[i] == '\\' && i + 1 < len) i++;
                i++;
            }
            if (i < len) i++;
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y,
                             quote == '"' ? theme->syn_string : theme->syn_number, theme);
            continue;
        }

        if (isdigit((unsigned char)text[i])) {
            size_t token_start = i;
            while (i < len && (isalnum((unsigned char)text[i]) || text[i] == '.')) i++;
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_number, theme);
            continue;
        }

        if (isalpha((unsigned char)text[i]) || text[i] == '_') {
            size_t token_start = i;
            while (i < len && (isalnum((unsigned char)text[i]) || text[i] == '_')) i++;
            size_t wlen = i - token_start;
            char word[128];
            if (wlen < sizeof(word)) {
                memcpy(word, &text[token_start], wlen);
                word[wlen] = '\0';
                Color col = theme->syn_default;
                if (Syntax_IsKeyword(word)) col = theme->syn_keyword;
                else if (Syntax_IsType(word)) col = theme->syn_type;
                Syntax_DrawToken(font, line, token_start, wlen, start_x, start_y, col, theme);
            } else {
                Syntax_DrawToken(font, line, token_start, wlen, start_x, start_y, theme->syn_default, theme);
            }
            continue;
        }

        Syntax_DrawToken(font, line, i, 1, start_x, start_y, theme->syn_default, theme);
        i++;
    }
}