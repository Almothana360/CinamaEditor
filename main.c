#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================================
 * MODULE 1: CORE CONFIG, ENUMS & THEMES (types.h & theme.h / theme.c)
 * ============================================================================ */

#define TAB_SIZE 4
#define FONT_SIZE 22.0f
#define FONT_SPACING 1.5f
#define MAX_PARTICLES 768
#define MAX_GLOW_FLASHES 64

#define MAX_CAMERA_ZOOM 2.80f
#define MIN_CAMERA_ZOOM 0.40f

typedef enum {
    CAM_MODE_BOUNDS_FIT = 0,
    CAM_MODE_CURSOR_FOCUS,
    CAM_MODE_LINE_FOCUS,
    CAM_MODE_COUNT
} CCameraMode;

typedef struct {
    const char *name;
    Color bg;
    Color gutter_bg;
    Color gutter_num;
    Color gutter_num_curr;
    Color cursor;
    Color cursor_trail;
    Color selection;
    Color line_hl;
    Color bracket_match;
    Color syn_default;
    Color syn_keyword;
    Color syn_type;
    Color syn_preproc;
    Color syn_string;
    Color syn_number;
    Color syn_comment;
    Color status_bg;
    Color status_text;
    Color menu_bg;
    Color menu_border;
    Color menu_hl;
    bool is_light;
} Theme;

static const float UI_SCALES[] = { 0.50f, 1.00f, 1.20f, 1.50f, 2.00f, 2.50f, 3.00f, 4.00f };
#define UI_SCALE_COUNT 8

static const Theme THEMES[] = {
    // 0: Cyber Dark
    {
        .name = "Cyber Dark",
        .bg = (Color){ 14, 15, 20, 255 },
        .gutter_bg = (Color){ 11, 12, 16, 255 },
        .gutter_num = (Color){ 75, 80, 95, 255 },
        .gutter_num_curr = (Color){ 255, 215, 65, 255 },
        .cursor = (Color){ 255, 210, 50, 255 },
        .cursor_trail = (Color){ 255, 170, 30, 90 },
        .selection = (Color){ 60, 110, 190, 110 },
        .line_hl = (Color){ 24, 26, 35, 255 },
        .bracket_match = (Color){ 80, 220, 255, 210 },
        .syn_default = (Color){ 230, 235, 245, 255 },
        .syn_keyword = (Color){ 255, 85, 130, 255 },
        .syn_type = (Color){ 255, 170, 60, 255 },
        .syn_preproc = (Color){ 190, 120, 255, 255 },
        .syn_string = (Color){ 130, 235, 120, 255 },
        .syn_number = (Color){ 90, 200, 255, 255 },
        .syn_comment = (Color){ 105, 115, 130, 255 },
        .status_bg = (Color){ 10, 11, 15, 255 },
        .status_text = (Color){ 140, 145, 160, 255 },
        .menu_bg = (Color){ 20, 22, 30, 248 },
        .menu_border = (Color){ 255, 210, 50, 160 },
        .menu_hl = (Color){ 42, 46, 62, 255 },
        .is_light = false
    },
    // 1: Synthwave Neon
    {
        .name = "Synthwave Neon",
        .bg = (Color){ 24, 16, 42, 255 },
        .gutter_bg = (Color){ 18, 12, 32, 255 },
        .gutter_num = (Color){ 120, 95, 155, 255 },
        .gutter_num_curr = (Color){ 255, 235, 80, 255 },
        .cursor = (Color){ 255, 50, 160, 255 },
        .cursor_trail = (Color){ 180, 40, 240, 100 },
        .selection = (Color){ 170, 50, 180, 120 },
        .line_hl = (Color){ 36, 25, 62, 255 },
        .bracket_match = (Color){ 255, 230, 60, 220 },
        .syn_default = (Color){ 240, 230, 255, 255 },
        .syn_keyword = (Color){ 255, 45, 120, 255 },
        .syn_type = (Color){ 0, 240, 255, 255 },
        .syn_preproc = (Color){ 255, 175, 40, 255 },
        .syn_string = (Color){ 75, 255, 180, 255 },
        .syn_number = (Color){ 255, 210, 70, 255 },
        .syn_comment = (Color){ 135, 110, 165, 255 },
        .status_bg = (Color){ 16, 10, 28, 255 },
        .status_text = (Color){ 185, 160, 215, 255 },
        .menu_bg = (Color){ 32, 22, 56, 248 },
        .menu_border = (Color){ 255, 50, 160, 170 },
        .menu_hl = (Color){ 58, 38, 98, 255 },
        .is_light = false
    },
    // 2: Solarized Light
    {
        .name = "Solarized Light",
        .bg = (Color){ 253, 246, 227, 255 },
        .gutter_bg = (Color){ 238, 232, 213, 255 },
        .gutter_num = (Color){ 147, 161, 161, 255 },
        .gutter_num_curr = (Color){ 181, 137, 0, 255 },
        .cursor = (Color){ 38, 139, 210, 255 },
        .cursor_trail = (Color){ 42, 161, 152, 90 },
        .selection = (Color){ 42, 161, 152, 85 },
        .line_hl = (Color){ 242, 235, 214, 255 },
        .bracket_match = (Color){ 211, 54, 130, 200 },
        .syn_default = (Color){ 101, 123, 131, 255 },
        .syn_keyword = (Color){ 133, 153, 0, 255 },
        .syn_type = (Color){ 181, 137, 0, 255 },
        .syn_preproc = (Color){ 203, 75, 22, 255 },
        .syn_string = (Color){ 42, 161, 152, 255 },
        .syn_number = (Color){ 211, 54, 130, 255 },
        .syn_comment = (Color){ 147, 161, 161, 255 },
        .status_bg = (Color){ 238, 232, 213, 255 },
        .status_text = (Color){ 101, 123, 131, 255 },
        .menu_bg = (Color){ 248, 241, 220, 250 },
        .menu_border = (Color){ 38, 139, 210, 180 },
        .menu_hl = (Color){ 228, 220, 198, 255 },
        .is_light = true
    },
    // 3: Paper Clean
    {
        .name = "Paper Clean",
        .bg = (Color){ 250, 251, 253, 255 },
        .gutter_bg = (Color){ 240, 242, 247, 255 },
        .gutter_num = (Color){ 150, 155, 170, 255 },
        .gutter_num_curr = (Color){ 20, 30, 50, 255 },
        .cursor = (Color){ 15, 25, 45, 255 },
        .cursor_trail = (Color){ 70, 90, 130, 80 },
        .selection = (Color){ 75, 140, 245, 75 },
        .line_hl = (Color){ 240, 243, 249, 255 },
        .bracket_match = (Color){ 230, 60, 80, 210 },
        .syn_default = (Color){ 35, 42, 55, 255 },
        .syn_keyword = (Color){ 215, 30, 85, 255 },
        .syn_type = (Color){ 15, 105, 210, 255 },
        .syn_preproc = (Color){ 135, 40, 195, 255 },
        .syn_string = (Color){ 25, 145, 75, 255 },
        .syn_number = (Color){ 225, 110, 15, 255 },
        .syn_comment = (Color){ 140, 148, 162, 255 },
        .status_bg = (Color){ 235, 238, 245, 255 },
        .status_text = (Color){ 75, 85, 105, 255 },
        .menu_bg = (Color){ 255, 255, 255, 252 },
        .menu_border = (Color){ 50, 110, 220, 170 },
        .menu_hl = (Color){ 235, 240, 252, 255 },
        .is_light = true
    }
};
#define THEME_COUNT 4

static inline const Theme* Theme_Get(int index) {
    if (index < 0 || index >= THEME_COUNT) return &THEMES[0];
    return &THEMES[index];
}

static inline float Theme_GetUIScale(int index) {
    if (index < 0 || index >= UI_SCALE_COUNT) return 1.0f;
    return UI_SCALES[index];
}

/* ============================================================================
 * MODULE 2: AUDIO ENGINE (audio.h / audio.c)
 * ============================================================================ */

typedef struct {
    Sound snd_typing;
    Sound snd_space;
    Sound snd_enter;
    Sound snd_syntax_glow;
    bool is_ready;
} AudioSystem;

static Sound Audio_GenerateSynth(float freq, float duration, float decay, float volume) {
    int sample_rate = 44100;
    int frame_count = (int)(sample_rate * duration);
    short *data = (short *)malloc(frame_count * sizeof(short));
    if (!data) return (Sound){ 0 };

    for (int i = 0; i < frame_count; ++i) {
        float t = (float)i / (float)sample_rate;
        float env = expf(-t * decay);
        float sample = sinf(2.0f * PI * freq * t) * env * volume;
        data[i] = (short)(sample * 16000.0f);
    }

    Wave wave = {
        .frameCount = (unsigned int)frame_count,
        .sampleRate = (unsigned int)sample_rate,
        .sampleSize = 16,
        .channels = 1,
        .data = data
    };
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

static void Audio_Init(AudioSystem *as) {
    InitAudioDevice();
    as->is_ready = IsAudioDeviceReady();

    if (FileExists("type.wav")) as->snd_typing = LoadSound("type.wav");
    else as->snd_typing = Audio_GenerateSynth(820.0f, 0.045f, 48.0f, 0.85f);

    if (FileExists("space.wav")) as->snd_space = LoadSound("space.wav");
    else as->snd_space = Audio_GenerateSynth(380.0f, 0.07f, 32.0f, 1.0f);

    if (FileExists("enter.wav")) as->snd_enter = LoadSound("enter.wav");
    else as->snd_enter = Audio_GenerateSynth(1050.0f, 0.12f, 22.0f, 0.9f);

    if (FileExists("glow.wav")) as->snd_syntax_glow = LoadSound("glow.wav");
    else as->snd_syntax_glow = Audio_GenerateSynth(1400.0f, 0.22f, 16.0f, 1.0f);
}

static void Audio_Close(AudioSystem *as) {
    if (!as->is_ready) return;
    if (as->snd_typing.frameCount > 0) UnloadSound(as->snd_typing);
    if (as->snd_space.frameCount > 0) UnloadSound(as->snd_space);
    if (as->snd_enter.frameCount > 0) UnloadSound(as->snd_enter);
    if (as->snd_syntax_glow.frameCount > 0) UnloadSound(as->snd_syntax_glow);
    CloseAudioDevice();
    as->is_ready = false;
}

static void Audio_PlayKey(AudioSystem *as, int key, int combo_count) {
    if (!as->is_ready) return;
    float combo_pitch = 1.0f + fminf((float)combo_count * 0.015f, 0.65f);

    if (key == KEY_SPACE && as->snd_space.frameCount > 0) {
        SetSoundPitch(as->snd_space, combo_pitch * 0.95f);
        PlaySound(as->snd_space);
    } else if (key == KEY_ENTER && as->snd_enter.frameCount > 0) {
        SetSoundPitch(as->snd_enter, combo_pitch * 1.1f);
        PlaySound(as->snd_enter);
    } else if (as->snd_typing.frameCount > 0) {
        float jitter = ((float)GetRandomValue(-8, 8) / 100.0f);
        SetSoundPitch(as->snd_typing, combo_pitch + jitter);
        PlaySound(as->snd_typing);
    }
}

static void Audio_PlayGlow(AudioSystem *as) {
    if (!as->is_ready || as->snd_syntax_glow.frameCount == 0) return;
    SetSoundPitch(as->snd_syntax_glow, 1.0f + ((float)GetRandomValue(0, 15) / 100.0f));
    PlaySound(as->snd_syntax_glow);
}

/* ============================================================================
 * MODULE 3: TEXT BUFFER & MEASUREMENT (buffer.h / buffer.c)
 * ============================================================================ */

typedef struct {
    char *chars;
    size_t size;
    size_t capacity;
    bool starts_in_comment;
} Line;

static void Line_Init(Line *line) {
    line->chars = NULL;
    line->size = 0;
    line->capacity = 0;
    line->starts_in_comment = false;
}

static void Line_Free(Line *line) {
    if (line->chars) free(line->chars);
    line->chars = NULL;
    line->size = 0;
    line->capacity = 0;
}

static void Line_Reserve(Line *line, size_t needed) {
    if (needed + 1 > line->capacity) {
        size_t new_cap = line->capacity == 0 ? 16 : line->capacity * 2;
        while (new_cap < needed + 1) new_cap *= 2;
        char *nc = (char *)realloc(line->chars, new_cap);
        if (nc) {
            line->chars = nc;
            line->capacity = new_cap;
        }
    }
}

static void Line_InsertChar(Line *line, size_t col, char c) {
    Line_Reserve(line, line->size + 1);
    if (col > line->size) col = line->size;
    memmove(&line->chars[col + 1], &line->chars[col], line->size - col);
    line->chars[col] = c;
    line->size++;
    line->chars[line->size] = '\0';
}

static void Line_DeleteChar(Line *line, size_t col) {
    if (col >= line->size) return;
    memmove(&line->chars[col], &line->chars[col + 1], line->size - col);
    line->size--;
    line->chars[line->size] = '\0';
}

static void Line_AppendStr(Line *line, const char *str, size_t len) {
    if (!str || len == 0) return;
    Line_Reserve(line, line->size + len);
    memcpy(&line->chars[line->size], str, len);
    line->size += len;
    line->chars[line->size] = '\0';
}

static float Line_GetColX(Font font, const Line *line, size_t col, float font_size, float spacing) {
    if (!line || col == 0 || line->size == 0) return 0.0f;
    if (col > line->size) col = line->size;

    char saved = line->chars[col];
    ((char *)line->chars)[col] = '\0';
    Vector2 sz = MeasureTextEx(font, line->chars, font_size, spacing);
    ((char *)line->chars)[col] = saved;
    return sz.x;
}

/* ============================================================================
 * MODULE 4: SYNTAX LEXER & BRACKET MATCHING (syntax.h / syntax.c)
 * ============================================================================ */

typedef struct {
    bool found;
    size_t row;
    size_t col;
} BracketMatch;

static bool Syntax_IsKeyword(const char *word) {
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

static bool Syntax_IsType(const char *word) {
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

static BracketMatch Syntax_FindMatchingBracket(const Line *lines, size_t line_count, size_t cur_row, size_t cur_col) {
    BracketMatch match = { .found = false, .row = 0, .col = 0 };
    if (line_count == 0 || cur_row >= line_count) return match;

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

static void Syntax_UpdateMultilineComments(Line *lines, size_t line_count) {
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
                } else j++;
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
                } else j++;
            }
        }
    }
}

static void Syntax_DrawToken(Font font, const Line *line, size_t start, size_t len,
                            float start_x, float start_y, Color color, const Theme *theme) {
    if (len == 0) return;
    float offset_x = Line_GetColX(font, line, start, FONT_SIZE, FONT_SPACING);

    char buf[256];
    char *str = buf;
    if (len >= sizeof(buf)) str = (char *)malloc(len + 1);
    memcpy(str, &line->chars[start], len);
    str[len] = '\0';

    Vector2 pos = { start_x + offset_x, start_y };

    if (!theme->is_light && (color.r != theme->syn_default.r || color.g != theme->syn_default.g || color.b != theme->syn_default.b)) {
        BeginBlendMode(BLEND_ADDITIVE);
        Color glow = color;
        glow.a = 48;
        DrawTextEx(font, str, (Vector2){ pos.x - 1.0f, pos.y }, FONT_SIZE, FONT_SPACING, glow);
        DrawTextEx(font, str, (Vector2){ pos.x + 1.0f, pos.y }, FONT_SIZE, FONT_SPACING, glow);
        DrawTextEx(font, str, (Vector2){ pos.x, pos.y - 1.0f }, FONT_SIZE, FONT_SPACING, glow);
        DrawTextEx(font, str, (Vector2){ pos.x, pos.y + 1.0f }, FONT_SIZE, FONT_SPACING, glow);
        EndBlendMode();
    }

    DrawTextEx(font, str, pos, FONT_SIZE, FONT_SPACING, color);
    if (str != buf) free(str);
}

static void Syntax_DrawLine(Font font, const Line *line, float start_x, float start_y, const Theme *theme) {
    if (!line || line->size == 0) return;
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

        if (i + 1 < len && text[i] == '/' && text[i + 1] == '/') {
            Syntax_DrawToken(font, line, i, len - i, start_x, start_y, theme->syn_comment, theme);
            break;
        }

        if (i + 1 < len && text[i] == '/' && text[i + 1] == '*') {
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

/* ============================================================================
 * MODULE 5: VISUAL EFFECTS & SHADERS (fx.h / fx.c)
 * ============================================================================ */

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
    float max_life;
    float size;
    bool active;
} Particle;

typedef struct {
    Rectangle rect;
    Color color;
    float life;
    float max_life;
    bool active;
} GlowFlash;

typedef struct {
    Particle particles[MAX_PARTICLES];
    GlowFlash glow_flashes[MAX_GLOW_FLASHES];
    Texture2D spotlight_tex;
    bool enable_spotlight;
    bool enable_crt;
    float shake_trauma;
} FxSystem;

static void Fx_Init(FxSystem *fx) {
    memset(fx, 0, sizeof(FxSystem));

    int sz = 1024;
    Image img = GenImageColor(sz, sz, BLANK);
    for (int y = 0; y < sz; ++y) {
        for (int x = 0; x < sz; ++x) {
            float dx = (float)(x - sz / 2);
            float dy = (float)(y - sz / 2);
            float d = sqrtf(dx * dx + dy * dy) / (float)(sz / 2);
            if (d > 1.0f) d = 1.0f;

            float alpha = 0.0f;
            if (d > 0.18f) {
                alpha = (d - 0.18f) / (0.85f - 0.18f);
                if (alpha > 1.0f) alpha = 1.0f;
                alpha = alpha * alpha * (3.0f - 2.0f * alpha);
            }
            unsigned char a = (unsigned char)(alpha * 242.0f);
            Color col = (Color){ 4, 5, 8, a };
            ImageDrawPixel(&img, x, y, col);
        }
    }
    fx->spotlight_tex = LoadTextureFromImage(img);
    UnloadImage(img);
}

static void Fx_Close(FxSystem *fx) {
    if (fx->spotlight_tex.id > 0) {
        UnloadTexture(fx->spotlight_tex);
        fx->spotlight_tex.id = 0;
    }
}

static void Fx_EmitParticles(FxSystem *fx, Vector2 pos, Color col, int count, bool power_mode) {
    for (int i = 0; i < count; ++i) {
        for (int p = 0; p < MAX_PARTICLES; ++p) {
            if (!fx->particles[p].active) {
                fx->particles[p].active = true;
                fx->particles[p].pos = pos;
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float speed = power_mode ? (float)GetRandomValue(90, 340) : (float)GetRandomValue(40, 180);
                fx->particles[p].vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };

                if (power_mode && GetRandomValue(0, 1) == 0) {
                    fx->particles[p].color = (Color){ (unsigned char)GetRandomValue(220, 255), (unsigned char)GetRandomValue(80, 220), 40, 255 };
                } else {
                    fx->particles[p].color = col;
                }

                fx->particles[p].life = 0.0f;
                fx->particles[p].max_life = (float)GetRandomValue(28, 65) / 100.0f;
                fx->particles[p].size = power_mode ? (float)GetRandomValue(3, 7) : (float)GetRandomValue(2, 4);
                break;
            }
        }
    }
}

static void Fx_TriggerGlow(FxSystem *fx, Rectangle rect, Color col) {
    for (int i = 0; i < MAX_GLOW_FLASHES; ++i) {
        if (!fx->glow_flashes[i].active) {
            fx->glow_flashes[i].active = true;
            fx->glow_flashes[i].rect = rect;
            fx->glow_flashes[i].color = col;
            fx->glow_flashes[i].life = 0.0f;
            fx->glow_flashes[i].max_life = 0.42f;
            break;
        }
    }
}

static void Fx_AddTrauma(FxSystem *fx, float amount) {
    fx->shake_trauma = fminf(fx->shake_trauma + amount, 1.0f);
}

static void Fx_Update(FxSystem *fx, float dt) {
    if (fx->shake_trauma > 0.0f) {
        fx->shake_trauma = fmaxf(fx->shake_trauma - dt * 2.5f, 0.0f);
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!fx->particles[i].active) continue;
        fx->particles[i].life += dt;
        if (fx->particles[i].life >= fx->particles[i].max_life) {
            fx->particles[i].active = false;
            continue;
        }
        fx->particles[i].pos = Vector2Add(fx->particles[i].pos, Vector2Scale(fx->particles[i].vel, dt));
        fx->particles[i].vel = Vector2Scale(fx->particles[i].vel, 0.92f);
    }

    for (int i = 0; i < MAX_GLOW_FLASHES; ++i) {
        if (!fx->glow_flashes[i].active) continue;
        fx->glow_flashes[i].life += dt;
        if (fx->glow_flashes[i].life >= fx->glow_flashes[i].max_life) {
            fx->glow_flashes[i].active = false;
        }
    }
}

static void Fx_DrawWorld(const FxSystem *fx) {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_GLOW_FLASHES; ++i) {
        if (!fx->glow_flashes[i].active) continue;
        float t = fx->glow_flashes[i].life / fx->glow_flashes[i].max_life;
        float alpha = (1.0f - t) * 0.8f;
        Color c = fx->glow_flashes[i].color;
        c.a = (unsigned char)(alpha * 255.0f);

        Rectangle r = fx->glow_flashes[i].rect;
        float exp_val = t * 16.0f;
        DrawRectangleRounded((Rectangle){
            r.x - exp_val, r.y - exp_val * 0.5f,
            r.width + exp_val * 2.0f, r.height + exp_val
        }, 0.35f, 4, c);
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!fx->particles[i].active) continue;
        float progress = fx->particles[i].life / fx->particles[i].max_life;
        float alpha = 1.0f - progress;
        Color c = fx->particles[i].color;
        c.a = (unsigned char)(alpha * 255.0f);
        DrawCircleV(fx->particles[i].pos, fx->particles[i].size * (1.0f - progress * 0.4f), c);
    }
    EndBlendMode();
}

static void Fx_DrawSpotlight(const FxSystem *fx, Vector2 mouse_pos, int screen_w, int screen_h, const Theme *theme) {
    if (!fx->enable_spotlight || fx->spotlight_tex.id == 0) return;

    float tex_sz = 1400.0f;
    Rectangle src = { 0, 0, (float)fx->spotlight_tex.width, (float)fx->spotlight_tex.height };
    Rectangle dst = { mouse_pos.x - tex_sz * 0.5f, mouse_pos.y - tex_sz * 0.5f, tex_sz, tex_sz };
    DrawTexturePro(fx->spotlight_tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);

    Color dark = (Color){ 4, 5, 8, 242 };
    if (dst.y > 0) DrawRectangle(0, 0, screen_w, (int)dst.y, dark);
    if (dst.y + dst.height < screen_h) DrawRectangle(0, (int)(dst.y + dst.height), screen_w, screen_h - (int)(dst.y + dst.height), dark);
    if (dst.x > 0) DrawRectangle(0, (int)dst.y, (int)dst.x, (int)dst.height, dark);
    if (dst.x + dst.width < screen_w) DrawRectangle((int)(dst.x + dst.width), (int)dst.y, screen_w - (int)(dst.x + dst.width), (int)dst.height, dark);

    DrawCircleLines((int)mouse_pos.x, (int)mouse_pos.y, 45.0f, ColorAlpha(theme->cursor, 0.35f));
}

static void Fx_DrawCRT(int screen_w, int screen_h) {
    Color scanline = (Color){ 0, 0, 0, 24 };
    for (int y = 0; y < screen_h; y += 3) {
        DrawRectangle(0, y, screen_w, 1, scanline);
    }
    DrawRectangleGradientH(0, 0, 100, screen_h, (Color){ 0, 0, 0, 130 }, BLANK);
    DrawRectangleGradientH(screen_w - 100, 0, 100, screen_h, BLANK, (Color){ 0, 0, 0, 130 });
    DrawRectangleGradientV(0, 0, screen_w, 70, (Color){ 0, 0, 0, 140 }, BLANK);
    DrawRectangleGradientV(0, screen_h - 70, screen_w, 70, BLANK, (Color){ 0, 0, 0, 140 });
}

/* ============================================================================
 * MODULE 6: EDITOR CORE & ACTIONS (editor.h / editor.c)
 * ============================================================================ */

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
} Editor;

typedef struct {
    Vector2 current;
    Vector2 target;
    Vector2 trail;
    float width;
    float height;
} SmoothCursor;

static void Editor_Free(Editor *ed) {
    for (size_t i = 0; i < ed->line_count; ++i) Line_Free(&ed->lines[i]);
    free(ed->lines);
    ed->lines = NULL;
    ed->line_count = 0;
    ed->line_capacity = 0;
}

static void Editor_AddLine(Editor *ed, Line line) {
    if (ed->line_count >= ed->line_capacity) {
        size_t new_cap = ed->line_capacity == 0 ? 32 : ed->line_capacity * 2;
        Line *nl = (Line *)realloc(ed->lines, new_cap * sizeof(Line));
        if (nl) {
            ed->lines = nl;
            ed->line_capacity = new_cap;
        }
    }
    ed->lines[ed->line_count++] = line;
}

static void Editor_InitEmpty(Editor *ed) {
    Editor_Free(ed);
    Line line;
    Line_Init(&line);
    Editor_AddLine(ed, line);
    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->has_selection = false;
    ed->anchor_row = 0;
    ed->anchor_col = 0;
    ed->modified = false;
}

static void Editor_GetSelectionBounds(const Editor *ed, size_t *sr, size_t *sc, size_t *er, size_t *ec) {
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

static void Editor_ClearSelection(Editor *ed) {
    ed->has_selection = false;
    ed->anchor_row = ed->cursor_row;
    ed->anchor_col = ed->cursor_col;
}

static void Editor_DeleteSelection(Editor *ed) {
    if (!ed->has_selection) return;

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

static void Editor_CopySelection(const Editor *ed) {
    size_t sr, sc, er, ec;
    Editor_GetSelectionBounds(ed, &sr, &sc, &er, &ec);

    if (!ed->has_selection || (sr == er && sc == ec)) {
        if (ed->lines[ed->cursor_row].chars) {
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

static void Editor_CheckSyntaxFormed(Editor *ed, float line_height, FxSystem *fx, AudioSystem *as, const Theme *theme, Font font_syntax, bool power_mode) {
    Line *l = &ed->lines[ed->cursor_row];
    if (l->size == 0 || ed->cursor_col == 0) return;

    size_t end = ed->cursor_col;
    while (end > 0 && isspace((unsigned char)l->chars[end - 1])) end--;
    if (end == 0) return;

    size_t start = end;
    while (start > 0 && (isalnum((unsigned char)l->chars[start - 1]) || l->chars[start - 1] == '_')) {
        start--;
    }
    if (start == end) return;

    size_t wlen = end - start;
    char token[128];
    if (wlen < sizeof(token)) {
        memcpy(token, &l->chars[start], wlen);
        token[wlen] = '\0';

        Color glow_col = BLANK;
        if (Syntax_IsKeyword(token)) glow_col = theme->syn_keyword;
        else if (Syntax_IsType(token)) glow_col = theme->syn_type;

        if (glow_col.a > 0) {
            float x0 = Line_GetColX(font_syntax, l, start, FONT_SIZE, FONT_SPACING);
            float x1 = Line_GetColX(font_syntax, l, end, FONT_SIZE, FONT_SPACING);
            Rectangle rect = { x0, (float)ed->cursor_row * line_height, x1 - x0, line_height };

            Fx_TriggerGlow(fx, rect, glow_col);
            Fx_EmitParticles(fx, (Vector2){ x1, (float)ed->cursor_row * line_height + line_height * 0.5f }, glow_col, 25, power_mode);
            Audio_PlayGlow(as);
        }
    }
}

static void Editor_InsertChar(Editor *ed, char c) {
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
    }
    Line_InsertChar(&ed->lines[ed->cursor_row], ed->cursor_col, c);
    ed->cursor_col++;
    ed->modified = true;
    Syntax_UpdateMultilineComments(ed->lines, ed->line_count);
}

static void Editor_InsertNewline(Editor *ed) {
    if (ed->has_selection) {
        Editor_DeleteSelection(ed);
    }

    if (ed->line_count >= ed->line_capacity) {
        size_t new_cap = ed->line_capacity == 0 ? 32 : ed->line_capacity * 2;
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
    if (indent_len > 0 && indent_len < sizeof(indent_buf) - TAB_SIZE - 2) {
        memcpy(indent_buf, curr->chars, indent_len);
        total_indent = indent_len;
    }
    if (extra_indent && total_indent + TAB_SIZE < sizeof(indent_buf) - 1) {
        for (int k = 0; k < TAB_SIZE; ++k) {
            indent_buf[total_indent++] = ' ';
        }
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

static void Editor_Backspace(Editor *ed) {
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

        if (curr->size > 0) Line_AppendStr(prev, curr->chars, curr->size);
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

static void Editor_SelectAll(Editor *ed) {
    if (ed->line_count == 0) return;
    ed->anchor_row = 0;
    ed->anchor_col = 0;
    ed->cursor_row = ed->line_count - 1;
    ed->cursor_col = ed->lines[ed->cursor_row].size;
    ed->has_selection = true;
}

static void Editor_PasteClipboard(Editor *ed) {
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
            for (int k = 0; k < TAB_SIZE; ++k) Editor_InsertChar(ed, ' ');
            clip++;
        } else {
            Editor_InsertChar(ed, *clip++);
        }
    }
}

static void Editor_CutSelection(Editor *ed) {
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

static void Editor_MoveWordLeft(Editor *ed) {
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

static void Editor_MoveWordRight(Editor *ed) {
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

static void Editor_DeleteWordBackward(Editor *ed) {
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

static void Editor_DeleteWordForward(Editor *ed) {
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

static void Editor_DuplicateLine(Editor *ed) {
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
        size_t new_cap = ed->line_capacity * 2;
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

/* ============================================================================
 * MODULE 7: UI SYSTEM & MENUS (ui.h / ui.c)
 * ============================================================================ */

typedef struct {
    int streak;
    float decay_timer;
    float max_timer;
    float title_scale;
} ComboSystem;

typedef enum {
    CTX_COPY,
    CTX_CUT,
    CTX_PASTE,
    CTX_SELECT_ALL,
    CTX_SEP1,
    CTX_CAM_BOUNDS,
    CTX_CAM_CURSOR,
    CTX_CAM_LINE,
    CTX_SEP2,
    CTX_ZOOM_IN,
    CTX_ZOOM_OUT,
    CTX_ZOOM_RESET,
    CTX_SEP3,
    CTX_UI_SCALE,
    CTX_THEME_CYCLE,
    CTX_SPOTLIGHT_TOGGLE,
    CTX_CRT_TOGGLE,
    CTX_COUNT
} ContextAction;

typedef struct {
    const char *label;
    const char *shortcut;
    bool is_separator;
} ContextMenuItem;

static const ContextMenuItem MENU_ITEMS[] = {
    [CTX_COPY]             = { "Copy", "Ctrl+C", false },
    [CTX_CUT]              = { "Cut", "Ctrl+X", false },
    [CTX_PASTE]            = { "Paste", "Ctrl+V", false },
    [CTX_SELECT_ALL]       = { "Select All", "Ctrl+A", false },
    [CTX_SEP1]             = { "", "", true },
    [CTX_CAM_BOUNDS]       = { "Cam: Script Fit", "F5", false },
    [CTX_CAM_CURSOR]       = { "Cam: Cursor Focus", "F6", false },
    [CTX_CAM_LINE]         = { "Cam: Line Focus", "F7", false },
    [CTX_SEP2]             = { "", "", true },
    [CTX_ZOOM_IN]          = { "Zoom In", "Ctrl++", false },
    [CTX_ZOOM_OUT]         = { "Zoom Out", "Ctrl+-", false },
    [CTX_ZOOM_RESET]       = { "Reset Zoom", "Ctrl+0", false },
    [CTX_SEP3]             = { "", "", true },
    [CTX_UI_SCALE]         = { "UI Scale", "F8", false },
    [CTX_THEME_CYCLE]      = { "Next Theme", "F4", false },
    [CTX_SPOTLIGHT_TOGGLE] = { "Toggle Spotlight", "F3", false },
    [CTX_CRT_TOGGLE]       = { "Toggle CRT FX", "F2", false },
};

typedef struct {
    bool active;
    Vector2 pos;
    int hovered_idx;
} ContextMenu;

static void UI_DrawMinimap(const Editor *ed, Camera2D camera, int screen_w, int screen_h, float line_height, float scale, const Theme *theme) {
    float mm_w = 120.0f * scale;
    float mm_h = (float)screen_h - (40.0f * scale);
    float mm_x = (float)screen_w - mm_w - (15.0f * scale);
    float mm_y = 20.0f * scale;

    DrawRectangleRounded((Rectangle){ mm_x, mm_y, mm_w, mm_h }, 0.08f, 4, ColorAlpha(theme->gutter_bg, 0.75f));
    DrawRectangleRoundedLines((Rectangle){ mm_x, mm_y, mm_w, mm_h }, 0.08f, 4, ColorAlpha(theme->gutter_num, 0.45f));

    if (ed->line_count == 0) return;

    float line_scale = fminf((mm_h - (20.0f * scale)) / (float)ed->line_count, 3.5f * scale);
    for (size_t r = 0; r < ed->line_count; ++r) {
        float line_w = fminf((float)ed->lines[r].size * 1.5f * scale, mm_w - (18.0f * scale));
        if (line_w <= 0.0f) continue;
        float ly = mm_y + (10.0f * scale) + (float)r * line_scale;
        Color c = (r == ed->cursor_row) ? theme->cursor : ColorAlpha(theme->syn_default, 0.35f);
        DrawRectangle((int)(mm_x + (8.0f * scale)), (int)ly, (int)line_w, (int)fmaxf(line_scale - 1.0f, 1.0f), c);
    }

    float total_code_h = (float)ed->line_count * line_height;
    if (total_code_h > 0.0f) {
        float vp_top = (camera.target.y - ((float)screen_h * 0.5f) / camera.zoom) / total_code_h;
        float vp_h = ((float)screen_h / camera.zoom) / total_code_h;
        float box_y = mm_y + (10.0f * scale) + vp_top * (ed->line_count * line_scale);
        float box_h = vp_h * (ed->line_count * line_scale);
        DrawRectangleLinesEx((Rectangle){ mm_x + (4.0f * scale), box_y, mm_w - (8.0f * scale), fmaxf(box_h, 8.0f) }, 1.0f, ColorAlpha(theme->cursor, 0.65f));
    }
}

static void UI_DrawContextMenu(ContextMenu *menu, int screen_w, int screen_h, float scale, CCameraMode cam_mode, const Theme *theme, Font font_body) {
    if (!menu->active) return;

    float item_height = 28.0f * scale;
    float sep_height = 8.0f * scale;
    float width = 270.0f * scale;

    float total_h = 0.0f;
    for (int i = 0; i < CTX_COUNT; ++i) {
        total_h += MENU_ITEMS[i].is_separator ? sep_height : item_height;
    }

    if (menu->pos.x + width > screen_w) menu->pos.x = screen_w - width - 8.0f;
    if (menu->pos.y + total_h > screen_h) menu->pos.y = screen_h - total_h - 8.0f;
    if (menu->pos.x < 0) menu->pos.x = 4.0f;
    if (menu->pos.y < 0) menu->pos.y = 4.0f;

    Rectangle bg_rect = { menu->pos.x, menu->pos.y, width, total_h };
    DrawRectangleRounded(bg_rect, 0.04f, 4, theme->menu_bg);
    DrawRectangleRoundedLines(bg_rect, 0.04f, 4, theme->menu_border);

    Vector2 mpos = GetMousePosition();
    float cur_y = menu->pos.y;
    menu->hovered_idx = -1;

    for (int i = 0; i < CTX_COUNT; ++i) {
        if (MENU_ITEMS[i].is_separator) {
            DrawLine((int)(menu->pos.x + 8.0f * scale), (int)(cur_y + sep_height * 0.5f),
                     (int)(menu->pos.x + width - 8.0f * scale), (int)(cur_y + sep_height * 0.5f),
                     ColorAlpha(theme->gutter_num, 0.4f));
            cur_y += sep_height;
            continue;
        }

        Rectangle item_rect = { menu->pos.x + 4.0f * scale, cur_y + 2.0f * scale, width - 8.0f * scale, item_height - 4.0f * scale };
        bool hovered = CheckCollisionPointRec(mpos, item_rect);
        if (hovered) {
            menu->hovered_idx = i;
            DrawRectangleRounded(item_rect, 0.15f, 4, theme->menu_hl);
            DrawRectangle((int)item_rect.x, (int)item_rect.y, (int)(3.0f * scale), (int)item_rect.height, theme->cursor);
        }

        char label_buf[64];
        if (i == CTX_CAM_BOUNDS) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_BOUNDS_FIT ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_CAM_CURSOR) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_CURSOR_FOCUS ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_CAM_LINE) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (cam_mode == CAM_MODE_LINE_FOCUS ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_UI_SCALE) {
            snprintf(label_buf, sizeof(label_buf), "UI Scale: %.0f%%", scale * 100.0f);
        } else {
            snprintf(label_buf, sizeof(label_buf), "%s", MENU_ITEMS[i].label);
        }

        Color text_col = hovered ? theme->cursor : theme->syn_default;
        DrawTextEx(font_body, label_buf, (Vector2){ item_rect.x + 10.0f * scale, item_rect.y + 4.0f * scale }, 14.0f * scale, 1.0f, text_col);

        Vector2 sz = MeasureTextEx(font_body, MENU_ITEMS[i].shortcut, 12.0f * scale, 1.0f);
        DrawTextEx(font_body, MENU_ITEMS[i].shortcut, (Vector2){ item_rect.x + item_rect.width - sz.x - 8.0f * scale, item_rect.y + 6.0f * scale }, 12.0f * scale, 1.0f, theme->gutter_num);

        cur_y += item_height;
    }
}

static void UI_DrawComboHUD(const ComboSystem *combo, int screen_w, float scale, const Theme *theme, Font font_body) {
    if (combo->streak <= 2) return;

    char combo_text[64];
    snprintf(combo_text, sizeof(combo_text), "%d COMBO!", combo->streak);

    Color combo_color = theme->cursor;
    if (combo->streak > 35) combo_color = (Color){ 255, 60, 60, 255 };
    else if (combo->streak > 20) combo_color = (Color){ 255, 130, 40, 255 };

    float combo_font_size = (FONT_SIZE + 12.0f) * combo->title_scale * scale;
    Vector2 sz = MeasureTextEx(font_body, combo_text, combo_font_size, 2.0f);
    Vector2 c_pos = { (float)screen_w - sz.x - (160.0f * scale), 30.0f * scale };

    DrawTextEx(font_body, combo_text, c_pos, combo_font_size, 2.0f, combo_color);

    float bar_w = 120.0f * scale;
    float bar_ratio = combo->decay_timer / combo->max_timer;
    DrawRectangle((int)c_pos.x, (int)(c_pos.y + sz.y + 4.0f * scale), (int)bar_w, (int)(4.0f * scale), ColorAlpha(theme->gutter_num, 0.5f));
    DrawRectangle((int)c_pos.x, (int)(c_pos.y + sz.y + 4.0f * scale), (int)(bar_w * bar_ratio), (int)(4.0f * scale), combo_color);
}

static void UI_DrawHelp(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    DrawRectangle(0, 0, screen_w, screen_h, (Color){ 0, 0, 0, 195 });
    float hw = 660.0f * scale, hh = 470.0f * scale;
    float hx = ((float)screen_w - hw) * 0.5f;
    float hy = ((float)screen_h - hh) * 0.5f;

    DrawRectangleRounded((Rectangle){ hx, hy, hw, hh }, 0.05f, 6, theme->menu_bg);
    DrawRectangleRoundedLines((Rectangle){ hx, hy, hw, hh }, 0.05f, 6, theme->menu_border);

    DrawTextEx(font_body, "ded - Cinematic Editor Controls", (Vector2){ hx + 25.0f * scale, hy + 20.0f * scale }, (FONT_SIZE + 3.0f) * scale, 1.5f, theme->cursor);
    const char *help_lines[] = {
        "Auto-Indent      : Preserves tabs/spaces automatically upon Enter",
        "Camera Modes     : F5 (Script Fit) | F6 (Cursor Focus) | F7 (Line Focus)",
        "UI Scaling       : F8 (Cycles 50% -> 100% -> 120% -> 150% ... 400%)",
        "Select Text      : Click & Drag with mouse or Shift + Navigation",
        "Context Menu     : Right click anywhere on screen",
        "Copy / Cut / Past: Ctrl+C / Ctrl+X / Ctrl+V",
        "Zoom In/Out/Reset: Ctrl++ / Ctrl+- / Ctrl+0 or Ctrl + Mouse Wheel",
        "Spotlight Mode   : F3 (darkens scene, illuminates mouse)",
        "Switch Theme     : F4 (Cyber Dark, Synthwave, Solarized, Paper)",
        "Word Navigation  : Ctrl + Left / Right",
        "Word Deletion    : Ctrl + Backspace / Ctrl + Delete",
        "Duplicate Line   : Ctrl + D",
        "Select All       : Ctrl + A",
        "Toggle CRT FX    : F2",
        NULL
    };
    float ly = hy + 65.0f * scale;
    for (int i = 0; help_lines[i]; ++i) {
        DrawTextEx(font_body, help_lines[i], (Vector2){ hx + 25.0f * scale, ly }, 14.0f * scale, 1.0f, theme->syn_default);
        ly += 24.0f * scale;
    }
}

/* ============================================================================
 * MODULE 8: APPLICATION ENGINE & MAIN LOOP (app.h / main.c)
 * ============================================================================ */

typedef struct {
    Editor editor;
    SmoothCursor cursor;
    Camera2D camera;
    AudioSystem audio;
    FxSystem fx;
    ContextMenu menu;
    ComboSystem combo;

    int theme_idx;
    int ui_scale_idx;
    CCameraMode cam_mode;

    Font font_body;
    Font font_syntax;

    float user_zoom_mult;
    bool show_help;
    bool is_mouse_dragging;
} AppEngine;

static void App_Init(AppEngine *app, const char *initial_file) {
    memset(app, 0, sizeof(AppEngine));
    app->theme_idx = 0;
    app->ui_scale_idx = 1;
    app->cam_mode = CAM_MODE_BOUNDS_FIT;
    app->user_zoom_mult = 1.0f;

    Audio_Init(&app->audio);
    Fx_Init(&app->fx);

    if (FileExists("fonts/VictorMono-Regular.ttf")) {
        app->font_body = LoadFontEx("fonts/VictorMono-Regular.ttf", (int)FONT_SIZE, NULL, 0);
    } else {
        app->font_body = GetFontDefault();
    }

    if (FileExists("fonts/iosevka-regular.ttf")) {
        app->font_syntax = LoadFontEx("fonts/iosevka-regular.ttf", (int)FONT_SIZE, NULL, 0);
    } else {
        app->font_syntax = app->font_body;
    }

    Editor_InitEmpty(&app->editor);

    float line_height = FONT_SIZE + 8.0f;
    app->camera.rotation = 0.0f;
    app->camera.zoom = 1.30f;
    app->camera.target = (Vector2){ 0.0f, line_height * 0.5f };

    app->cursor.current = (Vector2){ 0.0f, 4.0f };
    app->cursor.target = (Vector2){ 0.0f, 4.0f };
    app->cursor.trail = (Vector2){ 0.0f, 4.0f };
    app->cursor.width = 3.0f;
    app->cursor.height = line_height - 6.0f;

    app->combo.max_timer = 1.35f;
    app->combo.title_scale = 1.0f;

    if (initial_file) {
        FILE *f = fopen(initial_file, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *buf = (char *)malloc(sz + 1);
            if (buf) {
                size_t rd = fread(buf, 1, sz, f);
                buf[rd] = '\0';
                for (size_t i = 0; i < rd; ++i) {
                    if (buf[i] == '\n') Editor_InsertNewline(&app->editor);
                    else if (buf[i] != '\r') Line_InsertChar(&app->editor.lines[app->editor.cursor_row], app->editor.cursor_col++, buf[i]);
                }
                free(buf);
            }
            fclose(f);
            strncpy(app->editor.file_path, initial_file, sizeof(app->editor.file_path) - 1);
            app->editor.cursor_row = 0;
            app->editor.cursor_col = 0;
            app->editor.modified = false;
        }
    } else {
        const char *sample =
            "// Welcome to ded!\n"
            "// Auto-tabs inherit leading line whitespace upon Enter.\n"
            "// Right-click: Context Menu with Camera Modes & UI Scaling.\n"
            "\n"
            "#include <stdio.h>\n"
            "\n"
            "int main(int argc, char **argv) {\n"
            "    printf(\"Cinema code editor!\\n\");\n"
            "    return 0;\n"
            "}\n";
        while (*sample) {
            if (*sample == '\n') Editor_InsertNewline(&app->editor);
            else Editor_InsertChar(&app->editor, *sample);
            sample++;
        }
        app->editor.cursor_row = 6;
        app->editor.cursor_col = 4;
        app->editor.modified = false;
    }
}

static void App_Close(AppEngine *app) {
    Fx_Close(&app->fx);
    Audio_Close(&app->audio);
    Editor_Free(&app->editor);
}

static void App_HandleAction(AppEngine *app, ContextAction action) {
    switch (action) {
        case CTX_COPY: Editor_CopySelection(&app->editor); break;
        case CTX_CUT: Editor_CutSelection(&app->editor); break;
        case CTX_PASTE: Editor_PasteClipboard(&app->editor); break;
        case CTX_SELECT_ALL:
            app->editor.anchor_row = 0;
            app->editor.anchor_col = 0;
            app->editor.cursor_row = app->editor.line_count - 1;
            app->editor.cursor_col = app->editor.lines[app->editor.cursor_row].size;
            app->editor.has_selection = true;
            break;
        case CTX_CAM_BOUNDS: app->cam_mode = CAM_MODE_BOUNDS_FIT; break;
        case CTX_CAM_CURSOR: app->cam_mode = CAM_MODE_CURSOR_FOCUS; break;
        case CTX_CAM_LINE: app->cam_mode = CAM_MODE_LINE_FOCUS; break;
        case CTX_ZOOM_IN: app->user_zoom_mult = fminf(app->user_zoom_mult * 1.15f, 3.2f); break;
        case CTX_ZOOM_OUT: app->user_zoom_mult = fmaxf(app->user_zoom_mult / 1.15f, 0.35f); break;
        case CTX_ZOOM_RESET: app->user_zoom_mult = 1.0f; break;
        case CTX_UI_SCALE: app->ui_scale_idx = (app->ui_scale_idx + 1) % UI_SCALE_COUNT; break;
        case CTX_THEME_CYCLE: app->theme_idx = (app->theme_idx + 1) % THEME_COUNT; break;
        case CTX_SPOTLIGHT_TOGGLE: app->fx.enable_spotlight = !app->fx.enable_spotlight; break;
        case CTX_CRT_TOGGLE: app->fx.enable_crt = !app->fx.enable_crt; break;
        default: break;
    }
}

int main(int argc, char **argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "ded - cinematic code editor");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    AppEngine app;
    App_Init(&app, (argc > 1) ? argv[1] : NULL);

    float line_height = FONT_SIZE + 8.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        const Theme *theme = Theme_Get(app.theme_idx);
        float ui_scale = Theme_GetUIScale(app.ui_scale_idx);

        Vector2 mouse_screen = GetMousePosition();
        Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, app.camera);

        Fx_Update(&app.fx, dt);

        if (app.combo.streak > 0) {
            app.combo.decay_timer -= dt;
            if (app.combo.decay_timer <= 0.0f) {
                app.combo.streak = 0;
                app.combo.decay_timer = 0.0f;
            }
        }
        app.combo.title_scale = Lerp(app.combo.title_scale, 1.0f, 8.0f * dt);

        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if (IsKeyPressed(KEY_F1)) app.show_help = !app.show_help;
        if (IsKeyPressed(KEY_F2)) app.fx.enable_crt = !app.fx.enable_crt;
        if (IsKeyPressed(KEY_F3)) app.fx.enable_spotlight = !app.fx.enable_spotlight;
        if (IsKeyPressed(KEY_F4)) app.theme_idx = (app.theme_idx + 1) % THEME_COUNT;
        if (IsKeyPressed(KEY_F5)) app.cam_mode = CAM_MODE_BOUNDS_FIT;
        if (IsKeyPressed(KEY_F6)) app.cam_mode = CAM_MODE_CURSOR_FOCUS;
        if (IsKeyPressed(KEY_F7)) app.cam_mode = CAM_MODE_LINE_FOCUS;
        if (IsKeyPressed(KEY_F8)) app.ui_scale_idx = (app.ui_scale_idx + 1) % UI_SCALE_COUNT;

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            app.menu.active = true;
            app.menu.pos = mouse_screen;
        }

        if (app.menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (app.menu.hovered_idx >= 0) {
                    App_HandleAction(&app, (ContextAction)app.menu.hovered_idx);
                }
                app.menu.active = false;
            }
        }

        if (!app.menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)app.editor.line_count) r = (int)app.editor.line_count - 1;

                Line *l = &app.editor.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = Line_GetColX(app.font_syntax, l, c, FONT_SIZE, FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                if (shift) {
                    if (!app.editor.has_selection) {
                        app.editor.anchor_row = app.editor.cursor_row;
                        app.editor.anchor_col = app.editor.cursor_col;
                        app.editor.has_selection = true;
                    }
                } else {
                    app.editor.has_selection = false;
                    app.editor.anchor_row = r;
                    app.editor.anchor_col = best_c;
                }

                app.editor.cursor_row = r;
                app.editor.cursor_col = best_c;
                app.is_mouse_dragging = true;
            }

            if (app.is_mouse_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)app.editor.line_count) r = (int)app.editor.line_count - 1;

                Line *l = &app.editor.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = Line_GetColX(app.font_syntax, l, c, FONT_SIZE, FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                app.editor.cursor_row = r;
                app.editor.cursor_col = best_c;
                if (app.editor.cursor_row != app.editor.anchor_row || app.editor.cursor_col != app.editor.anchor_col) {
                    app.editor.has_selection = true;
                }
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                app.is_mouse_dragging = false;
                if (app.editor.cursor_row == app.editor.anchor_row && app.editor.cursor_col == app.editor.anchor_col) {
                    app.editor.has_selection = false;
                }
            }
        }

        float wheel = GetMouseWheelMove();
        if (ctrl && wheel != 0.0f) {
            app.user_zoom_mult = Clamp(app.user_zoom_mult + wheel * 0.12f, 0.35f, 3.2f);
        }

        if (ctrl && IsKeyPressed(KEY_C)) {
            Editor_CopySelection(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_X)) {
            Editor_CutSelection(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_V)) {
            Editor_PasteClipboard(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_A)) {
            Editor_SelectAll(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_D)) {
            Editor_DuplicateLine(&app.editor);
        } else if (ctrl && (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))) {
            app.user_zoom_mult = fminf(app.user_zoom_mult * 1.15f, 3.2f);
        } else if (ctrl && (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))) {
            app.user_zoom_mult = fmaxf(app.user_zoom_mult / 1.15f, 0.35f);
        } else if (ctrl && (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0))) {
            app.user_zoom_mult = 1.0f;
        } else if (ctrl && IsKeyPressed(KEY_BACKSPACE)) {
            Editor_DeleteWordBackward(&app.editor);
            Audio_PlayKey(&app.audio, KEY_BACKSPACE, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.2f);
        } else if (ctrl && IsKeyPressed(KEY_DELETE)) {
            Editor_DeleteWordForward(&app.editor);
            Audio_PlayKey(&app.audio, KEY_BACKSPACE, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.2f);
        } else if (IsKeyPressed(KEY_ENTER)) {
            Editor_InsertNewline(&app.editor);
            app.combo.streak++;
            app.combo.decay_timer = app.combo.max_timer;
            Audio_PlayKey(&app.audio, KEY_ENTER, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.30f);
        } else if (IsKeyPressed(KEY_BACKSPACE)) {
            Editor_Backspace(&app.editor);
            Audio_PlayKey(&app.audio, KEY_BACKSPACE, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.16f);
        } else if (IsKeyPressed(KEY_DELETE)) {
            if (app.editor.has_selection) {
                Editor_DeleteSelection(&app.editor);
            } else {
                Line *curr = &app.editor.lines[app.editor.cursor_row];
                if (app.editor.cursor_col < curr->size) {
                    Line_DeleteChar(curr, app.editor.cursor_col);
                    app.editor.modified = true;
                    Syntax_UpdateMultilineComments(app.editor.lines, app.editor.line_count);
                } else if (app.editor.cursor_row + 1 < app.editor.line_count) {
                    Line *next = &app.editor.lines[app.editor.cursor_row + 1];
                    Line_AppendStr(curr, next->chars, next->size);
                    Line_Free(next);
                    memmove(&app.editor.lines[app.editor.cursor_row + 1],
                            &app.editor.lines[app.editor.cursor_row + 2],
                            (app.editor.line_count - (app.editor.cursor_row + 2)) * sizeof(Line));
                    app.editor.line_count--;
                    app.editor.modified = true;
                    Syntax_UpdateMultilineComments(app.editor.lines, app.editor.line_count);
                }
            }
        } else if (IsKeyPressed(KEY_TAB)) {
            for (int k = 0; k < TAB_SIZE; ++k) Editor_InsertChar(&app.editor, ' ');
            app.combo.streak++;
            app.combo.decay_timer = app.combo.max_timer;
            Audio_PlayKey(&app.audio, KEY_SPACE, app.combo.streak);
        } else {
            int nav_key = 0;
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) nav_key = KEY_LEFT;
            else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) nav_key = KEY_RIGHT;
            else if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) nav_key = KEY_UP;
            else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) nav_key = KEY_DOWN;
            else if (IsKeyPressed(KEY_HOME)) nav_key = KEY_HOME;
            else if (IsKeyPressed(KEY_END)) nav_key = KEY_END;

            if (nav_key != 0) {
                if (shift && !app.editor.has_selection) {
                    app.editor.anchor_row = app.editor.cursor_row;
                    app.editor.anchor_col = app.editor.cursor_col;
                    app.editor.has_selection = true;
                } else if (!shift && app.editor.has_selection) {
                    Editor_ClearSelection(&app.editor);
                }

                if (ctrl && nav_key == KEY_LEFT) Editor_MoveWordLeft(&app.editor);
                else if (ctrl && nav_key == KEY_RIGHT) Editor_MoveWordRight(&app.editor);
                else if (ctrl && nav_key == KEY_HOME) { app.editor.cursor_row = 0; app.editor.cursor_col = 0; }
                else if (ctrl && nav_key == KEY_END) { app.editor.cursor_row = app.editor.line_count - 1; app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size; }
                else if (nav_key == KEY_LEFT) {
                    if (app.editor.cursor_col > 0) app.editor.cursor_col--;
                    else if (app.editor.cursor_row > 0) {
                        app.editor.cursor_row--;
                        app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_RIGHT) {
                    if (app.editor.cursor_col < app.editor.lines[app.editor.cursor_row].size) app.editor.cursor_col++;
                    else if (app.editor.cursor_row + 1 < app.editor.line_count) {
                        app.editor.cursor_row++;
                        app.editor.cursor_col = 0;
                    }
                } else if (nav_key == KEY_UP) {
                    if (app.editor.cursor_row > 0) {
                        app.editor.cursor_row--;
                        if (app.editor.cursor_col > app.editor.lines[app.editor.cursor_row].size)
                            app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_DOWN) {
                    if (app.editor.cursor_row + 1 < app.editor.line_count) {
                        app.editor.cursor_row++;
                        if (app.editor.cursor_col > app.editor.lines[app.editor.cursor_row].size)
                            app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_HOME) {
                    app.editor.cursor_col = 0;
                } else if (nav_key == KEY_END) {
                    app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                }

                if (shift && app.editor.cursor_row == app.editor.anchor_row && app.editor.cursor_col == app.editor.anchor_col) {
                    app.editor.has_selection = false;
                }
            } else {
                int ch = GetCharPressed();
                while (ch > 0) {
                    if (ch >= 32 && ch <= 126) {
                        Editor_InsertChar(&app.editor, (char)ch);
                        app.combo.streak++;
                        app.combo.decay_timer = app.combo.max_timer;
                        app.combo.title_scale = 1.35f;

                        Audio_PlayKey(&app.audio, ch == ' ' ? KEY_SPACE : KEY_A, app.combo.streak);
                        Fx_AddTrauma(&app.fx, (app.combo.streak > 20) ? 0.35f : 0.22f);

                        float cur_x = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], app.editor.cursor_col, FONT_SIZE, FONT_SPACING);
                        Vector2 spark_pos = { cur_x, (float)app.editor.cursor_row * line_height + line_height * 0.5f };
                        Fx_EmitParticles(&app.fx, spark_pos, theme->cursor, app.combo.streak > 15 ? 12 : 7, app.combo.streak > 25);

                        Editor_CheckSyntaxFormed(&app.editor, line_height, &app.fx, &app.audio, theme, app.font_syntax, app.combo.streak > 15);
                    }
                    ch = GetCharPressed();
                }
            }
        }

        float target_cur_x = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], app.editor.cursor_col, FONT_SIZE, FONT_SPACING);
        float target_cur_y = (float)app.editor.cursor_row * line_height;

        app.cursor.target = (Vector2){ target_cur_x, target_cur_y + 4.0f };
        app.cursor.trail = Vector2Lerp(app.cursor.trail, app.cursor.current, 10.0f * dt);
        app.cursor.current = Vector2Lerp(app.cursor.current, app.cursor.target, 22.0f * dt);

        Vector2 target_center = { 0 };
        float final_target_zoom = 1.0f;
        float gutter_space = 45.0f;

        if (app.cam_mode == CAM_MODE_BOUNDS_FIT) {
            float max_script_w = 0.0f;
            for (size_t i = 0; i < app.editor.line_count; ++i) {
                float lw = Line_GetColX(app.font_syntax, &app.editor.lines[i], app.editor.lines[i].size, FONT_SIZE, FONT_SPACING);
                if (lw > max_script_w) max_script_w = lw;
            }
            float script_box_w = fmaxf(max_script_w + gutter_space + 70.0f, 70.0f);
            float script_box_h = fmaxf((float)app.editor.line_count * line_height, line_height);

            target_center = (Vector2){ (script_box_w - gutter_space) * 0.5f, script_box_h * 0.5f };
            if (app.editor.line_count == 1 && app.editor.lines[0].size == 0) {
                target_center = (Vector2){ 0.0f, line_height * 0.5f };
            }

            float zoom_fit_x = ((float)screen_w * 0.70f) / script_box_w;
            float zoom_fit_y = ((float)screen_h * 0.70f) / script_box_h;
            float base_zoom = fminf(zoom_fit_x, zoom_fit_y);
            final_target_zoom = Clamp(base_zoom * app.user_zoom_mult, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);

        } else if (app.cam_mode == CAM_MODE_CURSOR_FOCUS) {
            target_center = (Vector2){ app.cursor.target.x + 20.0f, app.cursor.target.y + line_height * 0.5f };
            final_target_zoom = Clamp(1.30f * app.user_zoom_mult, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);

        } else if (app.cam_mode == CAM_MODE_LINE_FOCUS) {
            float curr_line_w = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], app.editor.lines[app.editor.cursor_row].size, FONT_SIZE, FONT_SPACING);
            target_center = (Vector2){ curr_line_w * 0.5f, (float)app.editor.cursor_row * line_height + line_height * 0.5f };

            float needed_w = fmaxf(curr_line_w + gutter_space + 140.0f, 320.0f);
            float line_zoom = ((float)screen_w * 0.80f) / needed_w;
            final_target_zoom = Clamp(line_zoom * app.user_zoom_mult, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);
        }

        app.camera.zoom = Lerp(app.camera.zoom, final_target_zoom, 5.5f * dt);
        app.camera.target = Vector2Lerp(app.camera.target, target_center, 6.0f * dt);

        float shake_intensity = app.fx.shake_trauma * app.fx.shake_trauma;
        float shake_mag = shake_intensity * (app.combo.streak > 25 ? 24.0f : 14.0f);
        Vector2 shake_offset = {
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag,
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag
        };
        app.camera.offset = (Vector2){
            (float)screen_w * 0.5f + shake_offset.x,
            (float)screen_h * 0.5f + shake_offset.y
        };

        BracketMatch bracket = Syntax_FindMatchingBracket(app.editor.lines, app.editor.line_count, app.editor.cursor_row, app.editor.cursor_col);

        BeginDrawing();
        ClearBackground(theme->bg);

        BeginMode2D(app.camera);

        if (app.editor.has_selection) {
            size_t sr, sc, er, ec;
            Editor_GetSelectionBounds(&app.editor, &sr, &sc, &er, &ec);
            for (size_t r = sr; r <= er; ++r) {
                Line *l = &app.editor.lines[r];
                size_t c_start = (r == sr) ? sc : 0;
                size_t c_end = (r == er) ? ec : l->size;

                float x0 = Line_GetColX(app.font_syntax, l, c_start, FONT_SIZE, FONT_SPACING);
                float x1 = Line_GetColX(app.font_syntax, l, c_end, FONT_SIZE, FONT_SPACING);
                float w = x1 - x0;
                if (w < 8.0f && r < er) w = 12.0f;

                DrawRectangle((int)x0, (int)(r * line_height + 4.0f), (int)w, (int)(line_height - 4.0f), theme->selection);
            }
        }

        for (size_t i = 0; i < app.editor.line_count; ++i) {
            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%zu", i + 1);
            Color nc = (i == app.editor.cursor_row) ? theme->gutter_num_curr : theme->gutter_num;
            DrawTextEx(app.font_body, num_str, (Vector2){ -gutter_space, (float)i * line_height + 4.0f }, FONT_SIZE - 2.0f, FONT_SPACING, nc);
        }

        for (size_t i = 0; i < app.editor.line_count; ++i) {
            Syntax_DrawLine(app.font_syntax, &app.editor.lines[i], 0.0f, (float)i * line_height + 4.0f, theme);
        }

        if (bracket.found) {
            float bx0 = Line_GetColX(app.font_syntax, &app.editor.lines[bracket.row], bracket.col, FONT_SIZE, FONT_SPACING);
            float bx1 = Line_GetColX(app.font_syntax, &app.editor.lines[bracket.row], bracket.col + 1, FONT_SIZE, FONT_SPACING);
            Rectangle b_rect = { bx0, (float)bracket.row * line_height + 4.0f, bx1 - bx0, line_height - 6.0f };

            if (!theme->is_light) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangleRounded(b_rect, 0.4f, 4, theme->bracket_match);
                EndBlendMode();
            }
            DrawRectangleRoundedLines(b_rect, 0.4f, 4, theme->bracket_match);
        }

        Fx_DrawWorld(&app.fx);

        bool blink = ((int)(GetTime() * 2.4)) % 2 == 0;
        if (blink || app.combo.streak > 0 || app.is_mouse_dragging) {
            float dist = Vector2Distance(app.cursor.current, app.cursor.trail);
            if (dist > 1.0f) {
                DrawRectangleV(app.cursor.trail, (Vector2){ app.cursor.width + dist * 0.4f, app.cursor.height }, theme->cursor_trail);
            }
            DrawRectangleV(app.cursor.current, (Vector2){ app.cursor.width, app.cursor.height }, theme->cursor);
        }

        EndMode2D();

        UI_DrawMinimap(&app.editor, app.camera, screen_w, screen_h, line_height, ui_scale, theme);
        UI_DrawComboHUD(&app.combo, screen_w, ui_scale, theme, app.font_body);
        Fx_DrawSpotlight(&app.fx, mouse_screen, screen_w, screen_h, theme);
        if (app.fx.enable_crt) Fx_DrawCRT(screen_w, screen_h);
        UI_DrawContextMenu(&app.menu, screen_w, screen_h, ui_scale, app.cam_mode, theme, app.font_body);

        if (app.show_help) {
            UI_DrawHelp(screen_w, screen_h, ui_scale, theme, app.font_body);
        }

        const char *cam_names[] = { "Script Fit", "Cursor Focus", "Line Focus" };
        char stats[256];
        snprintf(stats, sizeof(stats),
                 "ded | Cam: %s | UI: %.0f%% | Theme: %s | Zoom: %.2fx (User: %.0f%%) | F1: Help",
                 cam_names[app.cam_mode], ui_scale * 100.0f, theme->name, app.camera.zoom, app.user_zoom_mult * 100.0f);
        DrawTextEx(app.font_body, stats, (Vector2){ 18.0f * ui_scale, (float)screen_h - (24.0f * ui_scale) }, 13.0f * ui_scale, 1.0f, theme->status_text);

        EndDrawing();
    }

    App_Close(&app);
    CloseWindow();
    return 0;
}