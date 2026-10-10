#include "buffer/syntax.h"
#include "buffer/grammar.h"
#include "core/event.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const Document *g_syn_doc = NULL;
static const Theme *g_syn_theme = NULL;
static Font g_syn_font = {0};
static const LanguageDef *g_active_lang = NULL;

const LanguageDef *Syntax_GetLanguage(void) {
    if (!g_active_lang) {
        g_active_lang = Grammar_GetDefault();
    }
    return g_active_lang;
}

void Syntax_SetLanguage(const LanguageDef *lang) {
    g_active_lang = lang ? lang : Grammar_GetDefault();
}

void Syntax_SetLanguageByFilename(const char *filepath) {
    g_active_lang = Grammar_GetByFilename(filepath);
}

bool Syntax_IsKeyword(const char *word) {
    return Grammar_IsKeyword(Syntax_GetLanguage(), word);
}

bool Syntax_IsType(const char *word) {
    return Grammar_IsType(Syntax_GetLanguage(), word);
}

Color Syntax_GetHighlightColor(const char *word, const Theme *theme) {
    if (!theme || !word) return BLANK;
    const LanguageDef *lang = Syntax_GetLanguage();
    if (Grammar_IsKeyword(lang, word)) return theme->syn_keyword;
    if (Grammar_IsType(lang, word))    return theme->syn_type;
    return BLANK;
}

static void Syntax_OnEvent(EventType type, const void *payload) {
    if (type == EV_THEME_CHANGED) {
        const ThemeChangedPayload *p = (const ThemeChangedPayload *)payload;
        g_syn_theme = (const Theme *)p->theme;
    } else if (type == EV_FILE_MODIFIED) {
        if (g_syn_doc && g_syn_doc->file_path[0] != '\0') {
            Syntax_SetLanguageByFilename(g_syn_doc->file_path);
            Syntax_UpdateMultilineComments(g_syn_doc->lines, g_syn_doc->line_count);
        }
    } else if (type == EV_ACTION) {
        const ActionPayload *p = (const ActionPayload *)payload;
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

    Grammar_Init();
    if (doc && doc->file_path[0] != '\0') {
        Syntax_SetLanguageByFilename(doc->file_path);
    } else {
        Syntax_SetLanguage(Grammar_GetDefault());
    }

    Event_Subscribe(EV_THEME_CHANGED, Syntax_OnEvent);
    Event_Subscribe(EV_ACTION, Syntax_OnEvent);
    Event_Subscribe(EV_FILE_MODIFIED, Syntax_OnEvent);
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
    const LanguageDef *lang = Syntax_GetLanguage();

    const char *sc = (lang && lang->single_comment[0]) ? lang->single_comment : "//";
    size_t sc_len = strlen(sc);
    const char *mc_start = (lang && lang->multi_comment_start[0]) ? lang->multi_comment_start : "/*";
    size_t mc_s_len = strlen(mc_start);
    const char *mc_end = (lang && lang->multi_comment_end[0]) ? lang->multi_comment_end : "*/";
    size_t mc_e_len = strlen(mc_end);
    bool has_mc = (mc_s_len > 0 && mc_e_len > 0);

    bool in_comment = false;
    for (size_t i = 0; i < line_count; ++i) {
        lines[i].starts_in_comment = in_comment;
        const char *p = lines[i].chars;
        if (!p) continue;
        size_t len = lines[i].size;
        size_t j = 0;
        while (j < len) {
            if (in_comment) {
                if (has_mc && j + mc_e_len <= len && strncmp(&p[j], mc_end, mc_e_len) == 0) {
                    in_comment = false;
                    j += mc_e_len;
                } else {
                    j++;
                }
            } else {
                if (has_mc && j + mc_s_len <= len && strncmp(&p[j], mc_start, mc_s_len) == 0) {
                    in_comment = true;
                    j += mc_s_len;
                } else if (sc_len > 0 && j + sc_len <= len && strncmp(&p[j], sc, sc_len) == 0) {
                    break;
                } else if (p[j] == '"' || p[j] == '\'' || p[j] == '`') {
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
                      float start_x, float start_y, Color color, const Theme *theme, float decrypt_ratio) {
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
    size_t out_len = len;

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
        out_len = 0;
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

    // Phase 9: Boot Sequence Decryption Micro-Animation
    if (decrypt_ratio < 1.0f) {
        for (size_t i = 0; i < out_len; ++i) {
            if (str[i] > 32) { // Ignore whitespace so indentation doesn't glitch
                // Staggered falloff: left characters decrypt faster than right characters
                float threshold = (decrypt_ratio * 1.5f) - ((float)i / (float)out_len) * 0.5f;
                if (threshold < 1.0f) {
                    if ((float)GetRandomValue(0, 100) / 100.0f > threshold) {
                        str[i] = (char)GetRandomValue(33, 126); // Random printable ASCII
                    }
                }
            }
        }
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

void Syntax_DrawLine(Font font, const Line *line, float start_x, float start_y, const Theme *theme, float decrypt_ratio) {
    if (!line || !line->chars || line->size == 0 || !theme) return;
    const char *text = line->chars;
    size_t len = line->size;
    size_t i = 0;
    bool in_comment = line->starts_in_comment;
    const LanguageDef *lang = Syntax_GetLanguage();

    const char *sc = (lang && lang->single_comment[0]) ? lang->single_comment : "//";
    size_t sc_len = strlen(sc);
    const char *mc_start = (lang && lang->multi_comment_start[0]) ? lang->multi_comment_start : "/*";
    size_t mc_s_len = strlen(mc_start);
    const char *mc_end = (lang && lang->multi_comment_end[0]) ? lang->multi_comment_end : "*/";
    size_t mc_e_len = strlen(mc_end);
    bool has_mc = (mc_s_len > 0 && mc_e_len > 0);

    while (i < len) {
        if (in_comment) {
            size_t token_start = i;
            while (i < len) {
                if (has_mc && i + mc_e_len <= len && strncmp(&text[i], mc_end, mc_e_len) == 0) {
                    i += mc_e_len;
                    in_comment = false;
                    break;
                }
                i++;
            }
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_comment, theme, decrypt_ratio);
            continue;
        }

        if (text[i] == ' ' || text[i] == '\t') {
            i++;
            continue;
        }

        if (sc_len > 0 && i + sc_len <= len && strncmp(&text[i], sc, sc_len) == 0) {
            Syntax_DrawToken(font, line, i, len - i, start_x, start_y, theme->syn_comment, theme, decrypt_ratio);
            break;
        }

        if (has_mc && i + mc_s_len <= len && strncmp(&text[i], mc_start, mc_s_len) == 0) {
            size_t token_start = i;
            i += mc_s_len;
            in_comment = true;
            while (i < len) {
                if (i + mc_e_len <= len && strncmp(&text[i], mc_end, mc_e_len) == 0) {
                    i += mc_e_len;
                    in_comment = false;
                    break;
                }
                i++;
            }
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_comment, theme, decrypt_ratio);
            continue;
        }

        if (text[i] == '#' && strcmp(sc, "#") != 0) {
            size_t token_start = i;
            while (i < len && !isspace((unsigned char)text[i])) i++;
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_preproc, theme, decrypt_ratio);
            continue;
        }

        bool is_delim = false;
        if (lang && lang->string_delim_count > 0) {
            for (int d = 0; d < lang->string_delim_count; ++d) {
                if (text[i] == lang->string_delims[d]) {
                    is_delim = true;
                    break;
                }
            }
        } else {
            is_delim = (text[i] == '"' || text[i] == '\'');
        }

        if (is_delim) {
            char quote = text[i];
            size_t token_start = i++;
            while (i < len && text[i] != quote) {
                if (text[i] == '\\' && i + 1 < len) i++;
                i++;
            }
            if (i < len) i++;
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y,
                             (quote == '"' || quote == '`') ? theme->syn_string : theme->syn_number, theme, decrypt_ratio);
            continue;
        }

        if (isdigit((unsigned char)text[i])) {
            size_t token_start = i;
            while (i < len && (isalnum((unsigned char)text[i]) || text[i] == '.' || text[i] == 'x' || text[i] == 'X')) i++;
            Syntax_DrawToken(font, line, token_start, i - token_start, start_x, start_y, theme->syn_number, theme, decrypt_ratio);
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
                if (Grammar_IsKeyword(lang, word)) col = theme->syn_keyword;
                else if (Grammar_IsType(lang, word)) col = theme->syn_type;
                Syntax_DrawToken(font, line, token_start, wlen, start_x, start_y, col, theme, decrypt_ratio);
            } else {
                Syntax_DrawToken(font, line, token_start, wlen, start_x, start_y, theme->syn_default, theme, decrypt_ratio);
            }
            continue;
        }

        Syntax_DrawToken(font, line, i, 1, start_x, start_y, theme->syn_default, theme, decrypt_ratio);
        i++;
    }
}