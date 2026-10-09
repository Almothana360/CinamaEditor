#include "ui/completion.h"
#include "buffer/syntax.h"
#include "buffer/grammar.h"
#include "buffer/line.h"
#include "core/event.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

typedef struct {
    CompCandidate candidates[CE_MAX_COMP_CANDIDATES];
    int candidate_count;
    int selected_idx;

    char prefix[64];
    size_t prefix_start_col;
    size_t prefix_row;

    bool is_open;
} CompletionState;

static CompletionState g_comp = {0};

static float Completion_GetEffectiveScale(int screen_w, float user_scale) {
    int screen_h = GetScreenHeight();
    float sw = (screen_w > 0) ? (float)screen_w : 1280.0f;
    float sh = (screen_h > 0) ? (float)screen_h : 720.0f;
    float u = (user_scale > 0.1f) ? user_scale : 1.0f;
    float res_scale = fminf(sw / 1280.0f, sh / 720.0f);
    if (res_scale < 0.75f) res_scale = 0.75f;
    return u * res_scale;
}

static bool MatchesPrefix(const char *word, const char *prefix) {
    if (!word || !prefix) return false;
    size_t plen = strlen(prefix);
    size_t wlen = strlen(word);
    if (wlen < plen) return false;

    // Don't autocomplete if word is already completely typed identically
    if (strcmp(word, prefix) == 0) return false;

    for (size_t i = 0; i < plen; ++i) {
        if (tolower((unsigned char)word[i]) != tolower((unsigned char)prefix[i])) {
            return false;
        }
    }
    return true;
}

static bool HasCandidate(const char *word) {
    for (int i = 0; i < g_comp.candidate_count; ++i) {
        if (strcmp(g_comp.candidates[i].word, word) == 0) return true;
    }
    return false;
}

static int CalculateScore(const char *word, const char *prefix, CompKind kind) {
    int score = 0;
    size_t plen = strlen(prefix);

    // Exact casing match receives bonus
    if (strncmp(word, prefix, plen) == 0) {
        score += 100;
    } else {
        score += 50;
    }

    if (kind == COMP_KIND_KEYWORD) score += 20;
    else if (kind == COMP_KIND_TYPE) score += 15;
    else score += 10;

    int len_diff = (int)strlen(word) - (int)plen;
    if (len_diff < 10) score += (10 - len_diff);

    return score;
}

static void AddCandidate(const char *word, CompKind kind, const char *prefix) {
    if (g_comp.candidate_count >= CE_MAX_COMP_CANDIDATES) return;
    CompCandidate *c = &g_comp.candidates[g_comp.candidate_count++];
    strncpy(c->word, word, sizeof(c->word) - 1);
    c->word[sizeof(c->word) - 1] = '\0';
    c->kind = kind;
    c->score = CalculateScore(word, prefix, kind);
}

static int Comp_CandidateCompare(const void *a, const void *b) {
    const CompCandidate *ca = (const CompCandidate *)a;
    const CompCandidate *cb = (const CompCandidate *)b;
    if (ca->score != cb->score) {
        return cb->score - ca->score; // Highest score first
    }
    return strcmp(ca->word, cb->word);
}

static void HarvestGrammarWords(const char *prefix) {
    const LanguageDef *lang = Syntax_GetLanguage();
    if (!lang) return;

    if (lang->keywords) {
        for (int i = 0; i < lang->keyword_count; ++i) {
            if (lang->keywords[i] && MatchesPrefix(lang->keywords[i], prefix) && !HasCandidate(lang->keywords[i])) {
                AddCandidate(lang->keywords[i], COMP_KIND_KEYWORD, prefix);
                if (g_comp.candidate_count >= CE_MAX_COMP_CANDIDATES) return;
            }
        }
    }

    if (lang->types) {
        for (int i = 0; i < lang->type_count; ++i) {
            if (lang->types[i] && MatchesPrefix(lang->types[i], prefix) && !HasCandidate(lang->types[i])) {
                AddCandidate(lang->types[i], COMP_KIND_TYPE, prefix);
                if (g_comp.candidate_count >= CE_MAX_COMP_CANDIDATES) return;
            }
        }
    }
}

static void HarvestDocumentWords(const Document *doc, const char *prefix) {
    if (!doc || doc->line_count == 0) return;

    for (size_t r = 0; r < doc->line_count; ++r) {
        const Line *l = &doc->lines[r];
        if (l->size == 0 || !l->chars) continue;

        size_t c = 0;
        while (c < l->size) {
            while (c < l->size && !isalpha((unsigned char)l->chars[c]) && l->chars[c] != '_') {
                c++;
            }
            if (c >= l->size) break;

            size_t start = c;
            while (c < l->size && (isalnum((unsigned char)l->chars[c]) || l->chars[c] == '_')) {
                c++;
            }
            size_t wlen = c - start;
            if (wlen >= 2 && wlen < 64) {
                char word[64];
                memcpy(word, &l->chars[start], wlen);
                word[wlen] = '\0';

                if (MatchesPrefix(word, prefix) && !HasCandidate(word)) {
                    AddCandidate(word, COMP_KIND_IDENTIFIER, prefix);
                    if (g_comp.candidate_count >= CE_MAX_COMP_CANDIDATES) return;
                }
            }
        }
    }
}

static bool Completion_ExtractPrefix(const Document *doc, char *out_prefix, size_t *out_start_col) {
    if (!doc || doc->line_count == 0 || doc->cursor_row >= doc->line_count) return false;
    const Line *l = &doc->lines[doc->cursor_row];
    if (l->size == 0 || doc->cursor_col == 0 || !l->chars) return false;

    size_t end = doc->cursor_col;
    if (end > l->size) end = l->size;

    // Check if character before cursor is an identifier character
    if (!isalnum((unsigned char)l->chars[end - 1]) && l->chars[end - 1] != '_') {
        return false;
    }

    size_t start = end;
    while (start > 0 && (isalnum((unsigned char)l->chars[start - 1]) || l->chars[start - 1] == '_')) {
        start--;
    }

    size_t len = end - start;
    if (len < 2 || len >= 63) return false;

    memcpy(out_prefix, &l->chars[start], len);
    out_prefix[len] = '\0';
    if (out_start_col) *out_start_col = start;
    return true;
}

static Rectangle Completion_GetPopupRect(Camera2D camera, const Document *doc, Font font_syntax, int screen_w, int screen_h, float s) {
    if (!doc || doc->line_count == 0 || doc->cursor_row >= doc->line_count) {
        return (Rectangle){ 0, 0, 0, 0 };
    }

    float line_height = CE_FONT_SIZE + 8.0f;
    float cur_world_x = Line_GetColX(font_syntax, &doc->lines[doc->cursor_row], doc->cursor_col, CE_FONT_SIZE, CE_FONT_SPACING);
    float cur_world_y = (float)doc->cursor_row * line_height;

    Vector2 cur_screen = GetWorldToScreen2D((Vector2){ cur_world_x, cur_world_y }, camera);

    float popup_w = 210.0f * s;
    float item_h = 24.0f * s;
    int max_visible = 6;
    int visible_count = (g_comp.candidate_count < max_visible) ? g_comp.candidate_count : max_visible;
    float header_h = 4.0f * s;
    float footer_h = 18.0f * s;
    float popup_h = header_h + (float)visible_count * item_h + footer_h;

    float popup_x = cur_screen.x;
    float popup_y = cur_screen.y + (line_height * camera.zoom) + 4.0f;

    // Flip above cursor if approaching bottom of viewport
    if (popup_y + popup_h > (float)screen_h - 38.0f * s) {
        popup_y = cur_screen.y - popup_h - 4.0f;
    }
    if (popup_x + popup_w > (float)screen_w - 12.0f * s) {
        popup_x = (float)screen_w - popup_w - 12.0f * s;
    }
    if (popup_x < 12.0f * s) {
        popup_x = 12.0f * s;
    }
    if (popup_y < 36.0f * s) {
        popup_y = 36.0f * s;
    }

    return (Rectangle){ popup_x, popup_y, popup_w, popup_h };
}

void Completion_Init(Document *doc, Font font_syntax) {
    (void)doc;
    (void)font_syntax;
    memset(&g_comp, 0, sizeof(CompletionState));
}

void Completion_Free(void) {
    memset(&g_comp, 0, sizeof(CompletionState));
}

void Completion_Close(void) {
    g_comp.is_open = false;
    g_comp.candidate_count = 0;
    g_comp.selected_idx = 0;
}

bool Completion_IsOpen(void) {
    return (g_comp.is_open && g_comp.candidate_count > 0);
}

void Completion_CheckTrigger(const Document *doc, Font font_syntax) {
    (void)font_syntax;
    if (!doc) {
        Completion_Close();
        return;
    }

    char prefix[64];
    size_t start_col = 0;
    if (Completion_ExtractPrefix(doc, prefix, &start_col)) {
        strncpy(g_comp.prefix, prefix, sizeof(g_comp.prefix) - 1);
        g_comp.prefix[sizeof(g_comp.prefix) - 1] = '\0';
        g_comp.prefix_start_col = start_col;
        g_comp.prefix_row = doc->cursor_row;

        g_comp.candidate_count = 0;
        HarvestGrammarWords(prefix);
        HarvestDocumentWords(doc, prefix);

        if (g_comp.candidate_count > 0) {
            if (g_comp.candidate_count > 1) {
                qsort(g_comp.candidates, g_comp.candidate_count, sizeof(CompCandidate), Comp_CandidateCompare);
            }
            if (g_comp.selected_idx >= g_comp.candidate_count) {
                g_comp.selected_idx = 0;
            }
            g_comp.is_open = true;
        } else {
            Completion_Close();
        }
    } else {
        Completion_Close();
    }
}

void Completion_Commit(Document *doc) {
    if (!g_comp.is_open || g_comp.candidate_count == 0 || !doc) return;
    if (g_comp.selected_idx < 0 || g_comp.selected_idx >= g_comp.candidate_count) return;

    const char *replacement = g_comp.candidates[g_comp.selected_idx].word;
    size_t prefix_len = strlen(g_comp.prefix);

    // Delete prefix characters
    for (size_t i = 0; i < prefix_len; ++i) {
        Document_Backspace(doc);
    }
    // Insert full replacement token
    for (const char *p = replacement; *p; ++p) {
        Document_InsertChar(doc, *p);
    }

    TextChangedPayload tc = { doc->line_count, false };
    Event_Emit(EV_TEXT_CHANGED, &tc);
    CursorMovedPayload cm = { doc->cursor_row, doc->cursor_col };
    Event_Emit(EV_CURSOR_MOVED, &cm);

    Completion_Close();
}

bool Completion_ContainsPoint(Vector2 point, Camera2D camera, const Document *doc, Font font_syntax, int screen_w, int screen_h, float scale) {
    if (!g_comp.is_open || g_comp.candidate_count == 0) return false;
    float s = Completion_GetEffectiveScale(screen_w, scale);
    Rectangle popup = Completion_GetPopupRect(camera, doc, font_syntax, screen_w, screen_h, s);
    return CheckCollisionPointRec(point, popup);
}

bool Completion_Update(Document *doc, float scale) {
    (void)scale;
    if (!g_comp.is_open || g_comp.candidate_count == 0 || !doc) return false;

    // Up arrow navigation
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        g_comp.selected_idx = (g_comp.selected_idx - 1 + g_comp.candidate_count) % g_comp.candidate_count;
        return true;
    }

    // Down arrow navigation
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        g_comp.selected_idx = (g_comp.selected_idx + 1) % g_comp.candidate_count;
        return true;
    }

    // Tab or Enter commits completion
    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ENTER)) {
        Completion_Commit(doc);
        return true;
    }

    // Escape dismisses
    if (IsKeyPressed(KEY_ESCAPE)) {
        Completion_Close();
        return true;
    }

    return false;
}

void Completion_Draw(Camera2D camera, const Document *doc, Font font_syntax, Font font_body, int screen_w, int screen_h, float scale, const Theme *theme) {
    if (!g_comp.is_open || g_comp.candidate_count == 0 || !doc || !theme) return;
    if (doc->line_count == 0 || doc->cursor_row >= doc->line_count) return;

    float s = Completion_GetEffectiveScale(screen_w, scale);
    Rectangle popup = Completion_GetPopupRect(camera, doc, font_syntax, screen_w, screen_h, s);

    if (popup.width <= 0 || popup.height <= 0) return;

    // Drop shadow & background container (Rounded Rectangle)
    DrawRectangleRounded((Rectangle){ popup.x + 2.0f * s, popup.y + 3.0f * s, popup.width, popup.height }, 0.08f, 4, (Color){ 0, 0, 0, 130 });
    DrawRectangleRounded(popup, 0.08f, 4, theme->menu_bg);
    DrawRectangleRoundedLines(popup, 0.08f, 4, theme->menu_border);

    float item_h = 24.0f * s;
    int max_visible = 6;
    int visible_count = (g_comp.candidate_count < max_visible) ? g_comp.candidate_count : max_visible;

    int start_idx = 0;
    if (g_comp.selected_idx >= max_visible) {
        start_idx = g_comp.selected_idx - (max_visible - 1);
    }
    if (start_idx + max_visible > g_comp.candidate_count) {
        start_idx = g_comp.candidate_count - max_visible;
    }
    if (start_idx < 0) start_idx = 0;

    Vector2 mpos = GetMousePosition();
    float cur_y = popup.y + 4.0f * s;

    for (int i = 0; i < visible_count; ++i) {
        int idx = start_idx + i;
        if (idx >= g_comp.candidate_count) break;
        const CompCandidate *cand = &g_comp.candidates[idx];

        Rectangle row_rect = { popup.x + 3.0f * s, cur_y, popup.width - 6.0f * s, item_h };
        bool is_selected = (idx == g_comp.selected_idx);
        bool is_hovered = CheckCollisionPointRec(mpos, row_rect);

        // Click on suggestion row directly commits
        if (is_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_comp.selected_idx = idx;
            Completion_Commit((Document *)doc);
            return;
        }

        if (is_selected) {
            DrawRectangleRounded(row_rect, 0.20f, 4, theme->menu_hl);
            DrawRectangle((int)row_rect.x, (int)row_rect.y, (int)(3.0f * s), (int)row_rect.height, theme->cursor);
        } else if (is_hovered) {
            DrawRectangleRounded(row_rect, 0.20f, 4, ColorAlpha(theme->menu_hl, 0.55f));
        }

        // Badge
        const char *badge = "ID";
        Color badge_col = theme->syn_default;
        if (cand->kind == COMP_KIND_KEYWORD) {
            badge = "KW";
            badge_col = theme->syn_keyword;
        } else if (cand->kind == COMP_KIND_TYPE) {
            badge = "TYPE";
            badge_col = theme->syn_type;
        }

        Rectangle badge_rect = { row_rect.x + 6.0f * s, row_rect.y + 4.0f * s, 34.0f * s, 16.0f * s };
        DrawRectangleRounded(badge_rect, 0.25f, 4, ColorAlpha(badge_col, 0.18f));
        DrawTextEx(font_body, badge, (Vector2){ badge_rect.x + 4.0f * s, badge_rect.y + 2.5f * s }, 9.5f * s, 1.0f, badge_col);

        Color text_col = is_selected ? theme->cursor : theme->syn_default;
        DrawTextEx(font_body, cand->word, (Vector2){ row_rect.x + 46.0f * s, row_rect.y + 5.0f * s }, 12.0f * s, 1.0f, text_col);

        cur_y += item_h;
    }

    // Footer hint bar
    DrawLine((int)(popup.x + 6.0f * s), (int)cur_y, (int)(popup.x + popup.width - 6.0f * s), (int)cur_y, ColorAlpha(theme->gutter_num, 0.30f));
    DrawTextEx(font_body, "Tab: Select | Esc: Close", (Vector2){ popup.x + 8.0f * s, cur_y + 3.0f * s }, 9.5f * s, 1.0f, theme->gutter_num);
}