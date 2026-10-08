#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define TAB_SIZE 4
#define FONT_SIZE 22.0f
#define FONT_SPACING 1.5f
#define MAX_PARTICLES 768
#define MAX_GLOW_FLASHES 64

// Camera Zoom Clamps
#define MAX_CAMERA_ZOOM 2.80f
#define MIN_CAMERA_ZOOM 0.40f

// --- Camera Modes ---
typedef enum {
    CAM_MODE_BOUNDS_FIT = 0,
    CAM_MODE_CURSOR_FOCUS,
    CAM_MODE_LINE_FOCUS,
    CAM_MODE_COUNT
} CCameraMode;

static CCameraMode current_cam_mode = CAM_MODE_BOUNDS_FIT;

// --- UI Scale Presets ---
static const float UI_SCALES[] = { 0.50f, 1.00f, 1.20f, 1.50f, 2.00f, 2.50f, 3.00f, 4.00f };
#define UI_SCALE_COUNT 8
static int current_ui_scale_idx = 1; // Default 100%

static inline float GetUIScale(void) {
    return UI_SCALES[current_ui_scale_idx];
}

// --- Theme Definition ---

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

static int current_theme_idx = 0;
#define THEME (THEMES[current_theme_idx])

// --- Dynamic Text Structures ---

typedef struct {
    char *chars;
    size_t size;
    size_t capacity;
    bool starts_in_comment;
} Line;

typedef struct {
    Line *lines;
    size_t line_count;
    size_t line_capacity;
    size_t cursor_row;
    size_t cursor_col;

    // Selection Anchor
    bool has_selection;
    size_t anchor_row;
    size_t anchor_col;

    char file_path[512];
    bool modified;
} Editor;

// --- Smooth Cursor Struct ---

typedef struct {
    Vector2 current;
    Vector2 target;
    Vector2 trail;
    float width;
    float height;
} SmoothCursor;

// --- Particle System ---

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
    float max_life;
    float size;
    bool active;
} Particle;

static Particle particles[MAX_PARTICLES];

// --- Glow Flash Effects ---

typedef struct {
    Rectangle rect;
    Color color;
    float life;
    float max_life;
    bool active;
} GlowFlash;

static GlowFlash glow_flashes[MAX_GLOW_FLASHES];

// --- Audio & Font Assets ---

static Sound snd_typing = { 0 };
static Sound snd_space = { 0 };
static Sound snd_enter = { 0 };
static Sound snd_syntax_glow = { 0 };

static Font font_body = { 0 };
static Font font_syntax = { 0 };

// --- Combo / Power Mode Engine ---

typedef struct {
    int streak;
    float decay_timer;
    float max_timer;
    float title_scale;
} ComboSystem;

static ComboSystem combo = {
    .streak = 0,
    .decay_timer = 0.0f,
    .max_timer = 1.35f,
    .title_scale = 1.0f
};

// --- Context Menu System ---

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

static ContextMenu context_menu = {
    .active = false,
    .pos = { 0, 0 },
    .hovered_idx = -1
};

// --- Spotlight Texture State ---
static Texture2D spotlight_tex = { 0 };
static bool enable_spotlight = false;

// --- Procedural Audio Generator ---

static Sound GenerateSynthSound(float freq, float duration, float decay, float volume) {
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

static void InitAudioPlaceholders(void) {
    InitAudioDevice();

    if (FileExists("type.wav")) snd_typing = LoadSound("type.wav");
    else snd_typing = GenerateSynthSound(820.0f, 0.045f, 48.0f, 0.85f);

    if (FileExists("space.wav")) snd_space = LoadSound("space.wav");
    else snd_space = GenerateSynthSound(380.0f, 0.07f, 32.0f, 1.0f);

    if (FileExists("enter.wav")) snd_enter = LoadSound("enter.wav");
    else snd_enter = GenerateSynthSound(1050.0f, 0.12f, 22.0f, 0.9f);

    if (FileExists("glow.wav")) snd_syntax_glow = LoadSound("glow.wav");
    else snd_syntax_glow = GenerateSynthSound(1400.0f, 0.22f, 16.0f, 1.0f);
}

static void PlayKeySound(int key, int combo_count) {
    if (!IsAudioDeviceReady()) return;

    float combo_pitch = 1.0f + fminf((float)combo_count * 0.015f, 0.65f);
    if (key == KEY_SPACE && snd_space.frameCount > 0) {
        SetSoundPitch(snd_space, combo_pitch * 0.95f);
        PlaySound(snd_space);
    } else if (key == KEY_ENTER && snd_enter.frameCount > 0) {
        SetSoundPitch(snd_enter, combo_pitch * 1.1f);
        PlaySound(snd_enter);
    } else if (snd_typing.frameCount > 0) {
        float jitter = ((float)GetRandomValue(-8, 8) / 100.0f);
        SetSoundPitch(snd_typing, combo_pitch + jitter);
        PlaySound(snd_typing);
    }
}

static void PlayGlowSound(void) {
    if (IsAudioDeviceReady() && snd_syntax_glow.frameCount > 0) {
        SetSoundPitch(snd_syntax_glow, 1.0f + ((float)GetRandomValue(0, 15) / 100.0f));
        PlaySound(snd_syntax_glow);
    }
}

static void InitFontPlaceholders(void) {
    if (FileExists("fonts/VictorMono-Regular.ttf")) {
        font_body = LoadFontEx("fonts/VictorMono-Regular.ttf", (int)FONT_SIZE, NULL, 0);
    } else {
        font_body = GetFontDefault();
    }

    if (FileExists("fonts/iosevka-regular.ttf")) {
        font_syntax = LoadFontEx("fonts/iosevka-regular.ttf", (int)FONT_SIZE, NULL, 0);
    } else {
        font_syntax = font_body;
    }
}

static void InitSpotlightTexture(void) {
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
    spotlight_tex = LoadTextureFromImage(img);
    UnloadImage(img);
}

// --- Text Buffer Functions ---

static void line_init(Line *line) {
    line->chars = NULL;
    line->size = 0;
    line->capacity = 0;
    line->starts_in_comment = false;
}

static void line_free(Line *line) {
    if (line->chars) free(line->chars);
    line->chars = NULL;
    line->size = 0;
    line->capacity = 0;
}

static void line_reserve(Line *line, size_t needed) {
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

static void line_insert_char(Line *line, size_t col, char c) {
    line_reserve(line, line->size + 1);
    if (col > line->size) col = line->size;
    memmove(&line->chars[col + 1], &line->chars[col], line->size - col);
    line->chars[col] = c;
    line->size++;
    line->chars[line->size] = '\0';
}

static void line_delete_char(Line *line, size_t col) {
    if (col >= line->size) return;
    memmove(&line->chars[col], &line->chars[col + 1], line->size - col);
    line->size--;
    line->chars[line->size] = '\0';
}

static void line_append_str(Line *line, const char *str, size_t len) {
    if (!str || len == 0) return;
    line_reserve(line, line->size + len);
    memcpy(&line->chars[line->size], str, len);
    line->size += len;
    line->chars[line->size] = '\0';
}

static float get_line_col_x(Font font, const Line *line, size_t col, float font_size, float spacing) {
    if (!line || col == 0 || line->size == 0) return 0.0f;
    if (col > line->size) col = line->size;

    char saved = line->chars[col];
    ((char *)line->chars)[col] = '\0';
    Vector2 sz = MeasureTextEx(font, line->chars, font_size, spacing);
    ((char *)line->chars)[col] = saved;
    return sz.x;
}

// --- Selection Math & Operations ---

static void editor_get_selection_bounds(const Editor *ed, size_t *sr, size_t *sc, size_t *er, size_t *ec) {
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

static void editor_clear_selection(Editor *ed) {
    ed->has_selection = false;
    ed->anchor_row = ed->cursor_row;
    ed->anchor_col = ed->cursor_col;
}

static void editor_delete_selection(Editor *ed) {
    if (!ed->has_selection) return;

    size_t sr, sc, er, ec;
    editor_get_selection_bounds(ed, &sr, &sc, &er, &ec);
    if (sr == er && sc == ec) {
        editor_clear_selection(ed);
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
            line_append_str(first, &last->chars[ec], tail_len);
        } else {
            first->chars[first->size] = '\0';
        }

        for (size_t r = sr + 1; r <= er; ++r) {
            line_free(&ed->lines[r]);
        }
        memmove(&ed->lines[sr + 1], &ed->lines[er + 1], (ed->line_count - (er + 1)) * sizeof(Line));
        ed->line_count -= (er - sr);
    }

    ed->cursor_row = sr;
    ed->cursor_col = sc;
    ed->has_selection = false;
    ed->modified = true;
}

static void editor_copy_selection(const Editor *ed) {
    size_t sr, sc, er, ec;
    editor_get_selection_bounds(ed, &sr, &sc, &er, &ec);

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

// Forward declarations
static void editor_insert_char(Editor *ed, char c, float line_height, float *shake_trauma);
static void editor_insert_newline(Editor *ed, float line_height, float *shake_trauma);

static void editor_paste_clipboard(Editor *ed, float line_height, float *shake_trauma) {
    const char *clip = GetClipboardText();
    if (!clip || strlen(clip) == 0) return;

    if (ed->has_selection) {
        editor_delete_selection(ed);
    }

    while (*clip) {
        if (*clip == '\r') {
            clip++;
        } else if (*clip == '\n') {
            editor_insert_newline(ed, line_height, shake_trauma);
            clip++;
        } else if (*clip == '\t') {
            for (int k = 0; k < TAB_SIZE; ++k) {
                editor_insert_char(ed, ' ', line_height, shake_trauma);
            }
            clip++;
        } else {
            editor_insert_char(ed, *clip++, line_height, shake_trauma);
        }
    }
}

static void editor_cut_selection(Editor *ed) {
    editor_copy_selection(ed);
    if (ed->has_selection) {
        editor_delete_selection(ed);
    } else {
        if (ed->line_count > 1) {
            line_free(&ed->lines[ed->cursor_row]);
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

// --- Particles & FX ---

static void EmitParticles(Vector2 pos, Color col, int count, bool power_mode) {
    for (int i = 0; i < count; ++i) {
        for (int p = 0; p < MAX_PARTICLES; ++p) {
            if (!particles[p].active) {
                particles[p].active = true;
                particles[p].pos = pos;
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float speed = power_mode ? (float)GetRandomValue(90, 340) : (float)GetRandomValue(40, 180);
                particles[p].vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };

                if (power_mode && GetRandomValue(0, 1) == 0) {
                    particles[p].color = (Color){ (unsigned char)GetRandomValue(220, 255), (unsigned char)GetRandomValue(80, 220), 40, 255 };
                } else {
                    particles[p].color = col;
                }

                particles[p].life = 0.0f;
                particles[p].max_life = (float)GetRandomValue(28, 65) / 100.0f;
                particles[p].size = power_mode ? (float)GetRandomValue(3, 7) : (float)GetRandomValue(2, 4);
                break;
            }
        }
    }
}

static void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles[i].active) continue;
        particles[i].life += dt;
        if (particles[i].life >= particles[i].max_life) {
            particles[i].active = false;
            continue;
        }
        particles[i].pos = Vector2Add(particles[i].pos, Vector2Scale(particles[i].vel, dt));
        particles[i].vel = Vector2Scale(particles[i].vel, 0.92f);
    }
}

static void DrawParticles(void) {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles[i].active) continue;
        float progress = particles[i].life / particles[i].max_life;
        float alpha = 1.0f - progress;
        Color c = particles[i].color;
        c.a = (unsigned char)(alpha * 255.0f);
        DrawCircleV(particles[i].pos, particles[i].size * (1.0f - progress * 0.4f), c);
    }
    EndBlendMode();
}

static void TriggerGlowFlash(Rectangle rect, Color col) {
    for (int i = 0; i < MAX_GLOW_FLASHES; ++i) {
        if (!glow_flashes[i].active) {
            glow_flashes[i].active = true;
            glow_flashes[i].rect = rect;
            glow_flashes[i].color = col;
            glow_flashes[i].life = 0.0f;
            glow_flashes[i].max_life = 0.42f;
            break;
        }
    }
}

static void UpdateGlowFlashes(float dt) {
    for (int i = 0; i < MAX_GLOW_FLASHES; ++i) {
        if (!glow_flashes[i].active) continue;
        glow_flashes[i].life += dt;
        if (glow_flashes[i].life >= glow_flashes[i].max_life) {
            glow_flashes[i].active = false;
        }
    }
}

static void DrawGlowFlashes(void) {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_GLOW_FLASHES; ++i) {
        if (!glow_flashes[i].active) continue;
        float t = glow_flashes[i].life / glow_flashes[i].max_life;
        float alpha = (1.0f - t) * 0.8f;
        Color c = glow_flashes[i].color;
        c.a = (unsigned char)(alpha * 255.0f);

        Rectangle r = glow_flashes[i].rect;
        float exp_val = t * 16.0f;
        DrawRectangleRounded((Rectangle){
            r.x - exp_val, r.y - exp_val * 0.5f,
            r.width + exp_val * 2.0f, r.height + exp_val
        }, 0.35f, 4, c);
    }
    EndBlendMode();
}

// --- Syntax Rules ---

static bool is_c_keyword(const char *word) {
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

static bool is_c_type(const char *word) {
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

// --- Matching Bracket Search Engine ---

typedef struct {
    bool found;
    size_t row;
    size_t col;
} BracketMatch;

static BracketMatch FindMatchingBracket(const Editor *ed) {
    BracketMatch match = { .found = false, .row = 0, .col = 0 };
    if (ed->line_count == 0 || ed->cursor_row >= ed->line_count) return match;

    Line *cur_line = &ed->lines[ed->cursor_row];
    if (cur_line->size == 0) return match;

    size_t c_idx = ed->cursor_col;
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
        for (size_t r = ed->cursor_row; r < ed->line_count; ++r) {
            Line *l = &ed->lines[r];
            size_t start_col = (r == ed->cursor_row) ? c_idx : 0;
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
        for (long r = (long)ed->cursor_row; r >= 0; --r) {
            Line *l = &ed->lines[r];
            long start_col = (r == (long)ed->cursor_row) ? (long)c_idx : (long)l->size - 1;
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

static void CheckSyntaxFormed(Editor *ed, float line_height) {
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
        if (is_c_keyword(token)) glow_col = THEME.syn_keyword;
        else if (is_c_type(token)) glow_col = THEME.syn_type;

        if (glow_col.a > 0) {
            float x0 = get_line_col_x(font_syntax, l, start, FONT_SIZE, FONT_SPACING);
            float x1 = get_line_col_x(font_syntax, l, end, FONT_SIZE, FONT_SPACING);
            Rectangle rect = { x0, (float)ed->cursor_row * line_height, x1 - x0, line_height };

            TriggerGlowFlash(rect, glow_col);
            EmitParticles((Vector2){ x1, (float)ed->cursor_row * line_height + line_height * 0.5f }, glow_col, 25, combo.streak > 15);
            PlayGlowSound();
        }
    }
}

static void update_multiline_comments(Editor *ed) {
    bool in_comment = false;
    for (size_t i = 0; i < ed->line_count; ++i) {
        ed->lines[i].starts_in_comment = in_comment;
        const char *p = ed->lines[i].chars;
        if (!p) continue;
        size_t len = ed->lines[i].size;
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

// --- Editor Engine ---

static void editor_free(Editor *ed) {
    for (size_t i = 0; i < ed->line_count; ++i) line_free(&ed->lines[i]);
    free(ed->lines);
    ed->lines = NULL;
    ed->line_count = 0;
    ed->line_capacity = 0;
}

static void editor_add_line(Editor *ed, Line line) {
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

static void editor_init_empty(Editor *ed) {
    editor_free(ed);
    Line line;
    line_init(&line);
    editor_add_line(ed, line);
    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->has_selection = false;
    ed->anchor_row = 0;
    ed->anchor_col = 0;
    ed->modified = false;
}

static void editor_insert_char(Editor *ed, char c, float line_height, float *shake_trauma) {
    if (ed->has_selection) {
        editor_delete_selection(ed);
    }

    line_insert_char(&ed->lines[ed->cursor_row], ed->cursor_col, c);
    ed->cursor_col++;
    ed->modified = true;

    combo.streak++;
    combo.decay_timer = combo.max_timer;
    combo.title_scale = 1.35f;

    PlayKeySound(c == ' ' ? KEY_SPACE : KEY_A, combo.streak);

    float trauma_boost = (combo.streak > 20) ? 0.35f : 0.22f;
    *shake_trauma = fminf(*shake_trauma + trauma_boost, 1.0f);

    float cur_x = get_line_col_x(font_syntax, &ed->lines[ed->cursor_row], ed->cursor_col, FONT_SIZE, FONT_SPACING);
    Vector2 spark_pos = { cur_x, (float)ed->cursor_row * line_height + line_height * 0.5f };
    EmitParticles(spark_pos, THEME.cursor, combo.streak > 15 ? 12 : 7, combo.streak > 25);

    update_multiline_comments(ed);
    CheckSyntaxFormed(ed, line_height);
}

// Preserves leading indentation of current line and adds extra if previous ends with '{'
static void editor_insert_newline(Editor *ed, float line_height, float *shake_trauma) {
    if (ed->has_selection) {
        editor_delete_selection(ed);
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
    line_init(next);

    // 1. Calculate the exact leading whitespace of the current line
    size_t indent_len = 0;
    while (indent_len < curr->size && (curr->chars[indent_len] == ' ' || curr->chars[indent_len] == '\t')) {
        indent_len++;
    }
    if (indent_len > ed->cursor_col) {
        indent_len = ed->cursor_col;
    }

    // 2. Extra indentation check if '{' was just typed
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

    // 3. Slice tail of current line
    size_t tail_len = curr->size > ed->cursor_col ? curr->size - ed->cursor_col : 0;
    line_reserve(next, total_indent + tail_len);

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
    ed->cursor_col = total_indent; // Cursor starts at the indent position
    ed->modified = true;

    combo.streak++;
    combo.decay_timer = combo.max_timer;
    PlayKeySound(KEY_ENTER, combo.streak);

    *shake_trauma = fminf(*shake_trauma + 0.30f, 1.0f);
    update_multiline_comments(ed);
}

static void editor_backspace(Editor *ed, float *shake_trauma) {
    if (ed->has_selection) {
        editor_delete_selection(ed);
        PlayKeySound(KEY_BACKSPACE, combo.streak);
        *shake_trauma = fminf(*shake_trauma + 0.16f, 1.0f);
        update_multiline_comments(ed);
        return;
    }

    if (ed->cursor_col > 0) {
        line_delete_char(&ed->lines[ed->cursor_row], ed->cursor_col - 1);
        ed->cursor_col--;
        ed->modified = true;
        PlayKeySound(KEY_BACKSPACE, combo.streak);
        *shake_trauma = fminf(*shake_trauma + 0.16f, 1.0f);
    } else if (ed->cursor_row > 0) {
        Line *prev = &ed->lines[ed->cursor_row - 1];
        Line *curr = &ed->lines[ed->cursor_row];
        size_t prev_len = prev->size;

        if (curr->size > 0) line_append_str(prev, curr->chars, curr->size);
        line_free(curr);

        memmove(&ed->lines[ed->cursor_row],
                &ed->lines[ed->cursor_row + 1],
                (ed->line_count - (ed->cursor_row + 1)) * sizeof(Line));
        ed->line_count--;
        ed->cursor_row--;
        ed->cursor_col = prev_len;
        ed->modified = true;
        PlayKeySound(KEY_BACKSPACE, combo.streak);
        *shake_trauma = fminf(*shake_trauma + 0.25f, 1.0f);
    }
    update_multiline_comments(ed);
}

// --- Extended Word and Range Deletions ---

static void editor_move_word_left(Editor *ed) {
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

static void editor_move_word_right(Editor *ed) {
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

static void editor_delete_word_backward(Editor *ed, float *shake_trauma) {
    if (ed->has_selection) {
        editor_delete_selection(ed);
        return;
    }
    if (ed->cursor_col == 0 && ed->cursor_row == 0) return;

    ed->anchor_row = ed->cursor_row;
    ed->anchor_col = ed->cursor_col;
    editor_move_word_left(ed);
    ed->has_selection = true;
    editor_delete_selection(ed);
    PlayKeySound(KEY_BACKSPACE, combo.streak);
    *shake_trauma = fminf(*shake_trauma + 0.2f, 1.0f);
}

static void editor_delete_word_forward(Editor *ed, float *shake_trauma) {
    if (ed->has_selection) {
        editor_delete_selection(ed);
        return;
    }
    Line *l = &ed->lines[ed->cursor_row];
    if (ed->cursor_col >= l->size && ed->cursor_row + 1 >= ed->line_count) return;

    ed->anchor_row = ed->cursor_row;
    ed->anchor_col = ed->cursor_col;
    editor_move_word_right(ed);
    ed->has_selection = true;
    editor_delete_selection(ed);
    PlayKeySound(KEY_BACKSPACE, combo.streak);
    *shake_trauma = fminf(*shake_trauma + 0.2f, 1.0f);
}

static void editor_duplicate_line(Editor *ed) {
    if (ed->has_selection) {
        editor_copy_selection(ed);
        float dummy = 0.0f;
        editor_paste_clipboard(ed, FONT_SIZE + 8.0f, &dummy);
        return;
    }

    Line *curr = &ed->lines[ed->cursor_row];
    Line dup;
    line_init(&dup);
    line_append_str(&dup, curr->chars, curr->size);

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
    update_multiline_comments(ed);
}

// --- Syntax Rendering with Bloom Halo ---

static void draw_token(Font font, const Line *line, size_t start, size_t len,
                       float start_x, float start_y, Color color) {
    if (len == 0) return;
    float offset_x = get_line_col_x(font, line, start, FONT_SIZE, FONT_SPACING);

    char buf[256];
    char *str = buf;
    if (len >= sizeof(buf)) str = (char *)malloc(len + 1);
    memcpy(str, &line->chars[start], len);
    str[len] = '\0';

    Vector2 pos = { start_x + offset_x, start_y };

    if (!THEME.is_light && (color.r != THEME.syn_default.r || color.g != THEME.syn_default.g || color.b != THEME.syn_default.b)) {
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

static void draw_syntax_line(Font font, const Line *line, float start_x, float start_y) {
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
            draw_token(font, line, token_start, i - token_start, start_x, start_y, THEME.syn_comment);
            continue;
        }

        if (text[i] == ' ' || text[i] == '\t') {
            i++;
            continue;
        }

        if (i + 1 < len && text[i] == '/' && text[i + 1] == '/') {
            draw_token(font, line, i, len - i, start_x, start_y, THEME.syn_comment);
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
            draw_token(font, line, token_start, i - token_start, start_x, start_y, THEME.syn_comment);
            continue;
        }

        if (text[i] == '#') {
            size_t token_start = i;
            while (i < len && !isspace((unsigned char)text[i])) i++;
            draw_token(font, line, token_start, i - token_start, start_x, start_y, THEME.syn_preproc);
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
            draw_token(font, line, token_start, i - token_start, start_x, start_y,
                       quote == '"' ? THEME.syn_string : THEME.syn_number);
            continue;
        }

        if (isdigit((unsigned char)text[i])) {
            size_t token_start = i;
            while (i < len && (isalnum((unsigned char)text[i]) || text[i] == '.')) i++;
            draw_token(font, line, token_start, i - token_start, start_x, start_y, THEME.syn_number);
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
                Color col = THEME.syn_default;
                if (is_c_keyword(word)) col = THEME.syn_keyword;
                else if (is_c_type(word)) col = THEME.syn_type;
                draw_token(font, line, token_start, wlen, start_x, start_y, col);
            } else {
                draw_token(font, line, token_start, wlen, start_x, start_y, THEME.syn_default);
            }
            continue;
        }

        draw_token(font, line, i, 1, start_x, start_y, THEME.syn_default);
        i++;
    }
}

// --- Minimap ---

static void DrawMinimap(const Editor *ed, Camera2D camera, int screen_w, int screen_h, float line_height, float scale) {
    float mm_w = 120.0f * scale;
    float mm_h = (float)screen_h - (40.0f * scale);
    float mm_x = (float)screen_w - mm_w - (15.0f * scale);
    float mm_y = 20.0f * scale;

    DrawRectangleRounded((Rectangle){ mm_x, mm_y, mm_w, mm_h }, 0.08f, 4, ColorAlpha(THEME.gutter_bg, 0.75f));
    DrawRectangleRoundedLines((Rectangle){ mm_x, mm_y, mm_w, mm_h }, 0.08f, 4, ColorAlpha(THEME.gutter_num, 0.45f));

    if (ed->line_count == 0) return;

    float line_scale = fminf((mm_h - (20.0f * scale)) / (float)ed->line_count, 3.5f * scale);
    for (size_t r = 0; r < ed->line_count; ++r) {
        float line_w = fminf((float)ed->lines[r].size * 1.5f * scale, mm_w - (18.0f * scale));
        if (line_w <= 0.0f) continue;
        float ly = mm_y + (10.0f * scale) + (float)r * line_scale;
        Color c = (r == ed->cursor_row) ? THEME.cursor : ColorAlpha(THEME.syn_default, 0.35f);
        DrawRectangle((int)(mm_x + (8.0f * scale)), (int)ly, (int)line_w, (int)fmaxf(line_scale - 1.0f, 1.0f), c);
    }

    float total_code_h = (float)ed->line_count * line_height;
    if (total_code_h > 0.0f) {
        float vp_top = (camera.target.y - ((float)screen_h * 0.5f) / camera.zoom) / total_code_h;
        float vp_h = ((float)screen_h / camera.zoom) / total_code_h;
        float box_y = mm_y + (10.0f * scale) + vp_top * (ed->line_count * line_scale);
        float box_h = vp_h * (ed->line_count * line_scale);
        DrawRectangleLinesEx((Rectangle){ mm_x + (4.0f * scale), box_y, mm_w - (8.0f * scale), fmaxf(box_h, 8.0f) }, 1.0f, ColorAlpha(THEME.cursor, 0.65f));
    }
}

// --- CRT Effect ---

static void DrawCRTEffect(int screen_w, int screen_h) {
    Color scanline = (Color){ 0, 0, 0, 24 };
    for (int y = 0; y < screen_h; y += 3) {
        DrawRectangle(0, y, screen_w, 1, scanline);
    }
    DrawRectangleGradientH(0, 0, 100, screen_h, (Color){ 0, 0, 0, 130 }, BLANK);
    DrawRectangleGradientH(screen_w - 100, 0, 100, screen_h, BLANK, (Color){ 0, 0, 0, 130 });
    DrawRectangleGradientV(0, 0, screen_w, 70, (Color){ 0, 0, 0, 140 }, BLANK);
    DrawRectangleGradientV(0, screen_h - 70, screen_w, 70, BLANK, (Color){ 0, 0, 0, 140 });
}

// --- Spotlight Highlight ---

static void DrawSpotlight(Vector2 mouse_pos, int screen_w, int screen_h) {
    if (!enable_spotlight || spotlight_tex.id == 0) return;

    float tex_sz = 1400.0f;
    Rectangle src = { 0, 0, (float)spotlight_tex.width, (float)spotlight_tex.height };
    Rectangle dst = { mouse_pos.x - tex_sz * 0.5f, mouse_pos.y - tex_sz * 0.5f, tex_sz, tex_sz };
    DrawTexturePro(spotlight_tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);

    Color dark = (Color){ 4, 5, 8, 242 };
    if (dst.y > 0) DrawRectangle(0, 0, screen_w, (int)dst.y, dark);
    if (dst.y + dst.height < screen_h) DrawRectangle(0, (int)(dst.y + dst.height), screen_w, screen_h - (int)(dst.y + dst.height), dark);
    if (dst.x > 0) DrawRectangle(0, (int)dst.y, (int)dst.x, (int)dst.height, dark);
    if (dst.x + dst.width < screen_w) DrawRectangle((int)(dst.x + dst.width), (int)dst.y, screen_w - (int)(dst.x + dst.width), (int)dst.height, dark);

    DrawCircleLines((int)mouse_pos.x, (int)mouse_pos.y, 45.0f, ColorAlpha(THEME.cursor, 0.35f));
}

// --- Context Menu Drawing & Interaction ---

static void DrawContextMenu(ContextMenu *menu, int screen_w, int screen_h, float scale) {
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
    DrawRectangleRounded(bg_rect, 0.04f, 4, THEME.menu_bg);
    DrawRectangleRoundedLines(bg_rect, 0.04f, 4, THEME.menu_border);

    Vector2 mpos = GetMousePosition();
    float cur_y = menu->pos.y;
    menu->hovered_idx = -1;

    for (int i = 0; i < CTX_COUNT; ++i) {
        if (MENU_ITEMS[i].is_separator) {
            DrawLine((int)(menu->pos.x + 8.0f * scale), (int)(cur_y + sep_height * 0.5f),
                     (int)(menu->pos.x + width - 8.0f * scale), (int)(cur_y + sep_height * 0.5f),
                     ColorAlpha(THEME.gutter_num, 0.4f));
            cur_y += sep_height;
            continue;
        }

        Rectangle item_rect = { menu->pos.x + 4.0f * scale, cur_y + 2.0f * scale, width - 8.0f * scale, item_height - 4.0f * scale };
        bool hovered = CheckCollisionPointRec(mpos, item_rect);
        if (hovered) {
            menu->hovered_idx = i;
            DrawRectangleRounded(item_rect, 0.15f, 4, THEME.menu_hl);
            DrawRectangle((int)item_rect.x, (int)item_rect.y, (int)(3.0f * scale), (int)item_rect.height, THEME.cursor);
        }

        // Display checkmarks for camera modes
        char label_buf[64];
        if (i == CTX_CAM_BOUNDS) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (current_cam_mode == CAM_MODE_BOUNDS_FIT ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_CAM_CURSOR) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (current_cam_mode == CAM_MODE_CURSOR_FOCUS ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_CAM_LINE) {
            snprintf(label_buf, sizeof(label_buf), "%s %s", (current_cam_mode == CAM_MODE_LINE_FOCUS ? "[*]" : "[ ]"), MENU_ITEMS[i].label);
        } else if (i == CTX_UI_SCALE) {
            snprintf(label_buf, sizeof(label_buf), "UI Scale: %.0f%%", GetUIScale() * 100.0f);
        } else {
            snprintf(label_buf, sizeof(label_buf), "%s", MENU_ITEMS[i].label);
        }

        Color text_col = hovered ? THEME.cursor : THEME.syn_default;
        DrawTextEx(font_body, label_buf, (Vector2){ item_rect.x + 10.0f * scale, item_rect.y + 4.0f * scale }, 14.0f * scale, 1.0f, text_col);

        Vector2 sz = MeasureTextEx(font_body, MENU_ITEMS[i].shortcut, 12.0f * scale, 1.0f);
        DrawTextEx(font_body, MENU_ITEMS[i].shortcut, (Vector2){ item_rect.x + item_rect.width - sz.x - 8.0f * scale, item_rect.y + 6.0f * scale }, 12.0f * scale, 1.0f, THEME.gutter_num);

        cur_y += item_height;
    }
}

// --- Main Program ---

int main(int argc, char **argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "ded - cinematic code editor");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    InitAudioPlaceholders();
    InitFontPlaceholders();
    InitSpotlightTexture();

    float line_height = FONT_SIZE + 8.0f;

    Editor editor;
    memset(&editor, 0, sizeof(Editor));
    editor_init_empty(&editor);

    Camera2D camera = { 0 };
    camera.rotation = 0.0f;
    camera.zoom = 1.30f;
    camera.target = (Vector2){ 0.0f, line_height * 0.5f };

    float user_zoom_mult = 1.0f;

    SmoothCursor cursor = {
        .current = { 0.0f, 4.0f },
        .target = { 0.0f, 4.0f },
        .trail = { 0.0f, 4.0f },
        .width = 3.0f,
        .height = line_height - 6.0f
    };

    float shake_trauma = 0.0f;
    bool enable_crt = false;
    bool show_help = false;
    bool is_mouse_dragging = false;

    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *buf = (char *)malloc(sz + 1);
            if (buf) {
                size_t rd = fread(buf, 1, sz, f);
                buf[rd] = '\0';
                for (size_t i = 0; i < rd; ++i) {
                    if (buf[i] == '\n') editor_insert_newline(&editor, line_height, &shake_trauma);
                    else if (buf[i] != '\r') line_insert_char(&editor.lines[editor.cursor_row], editor.cursor_col++, buf[i]);
                }
                free(buf);
            }
            fclose(f);
            strncpy(editor.file_path, argv[1], sizeof(editor.file_path) - 1);
            editor.cursor_row = 0;
            editor.cursor_col = 0;
            editor.modified = false;
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
            if (*sample == '\n') editor_insert_newline(&editor, line_height, &shake_trauma);
            else editor_insert_char(&editor, *sample, line_height, &shake_trauma);
            sample++;
        }
        editor.cursor_row = 6;
        editor.cursor_col = 4;
        editor.modified = false;
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        float ui_scale = GetUIScale();

        Vector2 mouse_screen = GetMousePosition();
        Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, camera);

        // 1. Decay Screen Shake & Combo
        if (shake_trauma > 0.0f) {
            shake_trauma = fmaxf(shake_trauma - dt * 2.5f, 0.0f);
        }

        if (combo.streak > 0) {
            combo.decay_timer -= dt;
            if (combo.decay_timer <= 0.0f) {
                combo.streak = 0;
                combo.decay_timer = 0.0f;
            }
        }
        combo.title_scale = Lerp(combo.title_scale, 1.0f, 8.0f * dt);

        // 2. Modifiers
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // 3. Hotkeys
        if (IsKeyPressed(KEY_F1)) show_help = !show_help;
        if (IsKeyPressed(KEY_F2)) enable_crt = !enable_crt;
        if (IsKeyPressed(KEY_F3)) enable_spotlight = !enable_spotlight;
        if (IsKeyPressed(KEY_F4)) current_theme_idx = (current_theme_idx + 1) % 4;
        if (IsKeyPressed(KEY_F5)) current_cam_mode = CAM_MODE_BOUNDS_FIT;
        if (IsKeyPressed(KEY_F6)) current_cam_mode = CAM_MODE_CURSOR_FOCUS;
        if (IsKeyPressed(KEY_F7)) current_cam_mode = CAM_MODE_LINE_FOCUS;
        if (IsKeyPressed(KEY_F8)) current_ui_scale_idx = (current_ui_scale_idx + 1) % UI_SCALE_COUNT;

        // Context Menu Activation
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            context_menu.active = true;
            context_menu.pos = mouse_screen;
        }

        // Context Menu Click Handling
        if (context_menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (context_menu.hovered_idx >= 0) {
                    switch (context_menu.hovered_idx) {
                        case CTX_COPY: editor_copy_selection(&editor); break;
                        case CTX_CUT: editor_cut_selection(&editor); break;
                        case CTX_PASTE: editor_paste_clipboard(&editor, line_height, &shake_trauma); break;
                        case CTX_SELECT_ALL:
                            editor.anchor_row = 0;
                            editor.anchor_col = 0;
                            editor.cursor_row = editor.line_count - 1;
                            editor.cursor_col = editor.lines[editor.cursor_row].size;
                            editor.has_selection = true;
                            break;
                        case CTX_CAM_BOUNDS: current_cam_mode = CAM_MODE_BOUNDS_FIT; break;
                        case CTX_CAM_CURSOR: current_cam_mode = CAM_MODE_CURSOR_FOCUS; break;
                        case CTX_CAM_LINE: current_cam_mode = CAM_MODE_LINE_FOCUS; break;
                        case CTX_ZOOM_IN: user_zoom_mult = fminf(user_zoom_mult * 1.15f, 3.2f); break;
                        case CTX_ZOOM_OUT: user_zoom_mult = fmaxf(user_zoom_mult / 1.15f, 0.35f); break;
                        case CTX_ZOOM_RESET: user_zoom_mult = 1.0f; break;
                        case CTX_UI_SCALE: current_ui_scale_idx = (current_ui_scale_idx + 1) % UI_SCALE_COUNT; break;
                        case CTX_THEME_CYCLE: current_theme_idx = (current_theme_idx + 1) % 4; break;
                        case CTX_SPOTLIGHT_TOGGLE: enable_spotlight = !enable_spotlight; break;
                        case CTX_CRT_TOGGLE: enable_crt = !enable_crt; break;
                        default: break;
                    }
                    context_menu.active = false;
                } else {
                    context_menu.active = false;
                }
            }
        }

        // 4. Mouse Text Selection
        if (!context_menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)editor.line_count) r = (int)editor.line_count - 1;

                Line *l = &editor.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = get_line_col_x(font_syntax, l, c, FONT_SIZE, FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                if (shift) {
                    if (!editor.has_selection) {
                        editor.anchor_row = editor.cursor_row;
                        editor.anchor_col = editor.cursor_col;
                        editor.has_selection = true;
                    }
                } else {
                    editor.has_selection = false;
                    editor.anchor_row = r;
                    editor.anchor_col = best_c;
                }

                editor.cursor_row = r;
                editor.cursor_col = best_c;
                is_mouse_dragging = true;
            }

            if (is_mouse_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)editor.line_count) r = (int)editor.line_count - 1;

                Line *l = &editor.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = get_line_col_x(font_syntax, l, c, FONT_SIZE, FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                editor.cursor_row = r;
                editor.cursor_col = best_c;
                if (editor.cursor_row != editor.anchor_row || editor.cursor_col != editor.anchor_col) {
                    editor.has_selection = true;
                }
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                is_mouse_dragging = false;
                if (editor.cursor_row == editor.anchor_row && editor.cursor_col == editor.anchor_col) {
                    editor.has_selection = false;
                }
            }
        }

        // 5. Mouse Wheel Zoom (with Ctrl)
        float wheel = GetMouseWheelMove();
        if (ctrl && wheel != 0.0f) {
            user_zoom_mult = Clamp(user_zoom_mult + wheel * 0.12f, 0.35f, 3.2f);
        }

        // 6. Keyboard Editing & Shortcuts
        if (ctrl && IsKeyPressed(KEY_C)) {
            editor_copy_selection(&editor);
        } else if (ctrl && IsKeyPressed(KEY_X)) {
            editor_cut_selection(&editor);
        } else if (ctrl && IsKeyPressed(KEY_V)) {
            editor_paste_clipboard(&editor, line_height, &shake_trauma);
        } else if (ctrl && IsKeyPressed(KEY_A)) {
            editor.anchor_row = 0;
            editor.anchor_col = 0;
            editor.cursor_row = editor.line_count - 1;
            editor.cursor_col = editor.lines[editor.cursor_row].size;
            editor.has_selection = true;
        } else if (ctrl && IsKeyPressed(KEY_D)) {
            editor_duplicate_line(&editor);
        } else if (ctrl && (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))) {
            user_zoom_mult = fminf(user_zoom_mult * 1.15f, 3.2f);
        } else if (ctrl && (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))) {
            user_zoom_mult = fmaxf(user_zoom_mult / 1.15f, 0.35f);
        } else if (ctrl && (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0))) {
            user_zoom_mult = 1.0f;
        } else if (ctrl && IsKeyPressed(KEY_BACKSPACE)) {
            editor_delete_word_backward(&editor, &shake_trauma);
        } else if (ctrl && IsKeyPressed(KEY_DELETE)) {
            editor_delete_word_forward(&editor, &shake_trauma);
        } else if (IsKeyPressed(KEY_ENTER)) {
            editor_insert_newline(&editor, line_height, &shake_trauma);
        } else if (IsKeyPressed(KEY_BACKSPACE)) {
            editor_backspace(&editor, &shake_trauma);
        } else if (IsKeyPressed(KEY_DELETE)) {
            if (editor.has_selection) {
                editor_delete_selection(&editor);
            } else {
                Line *curr = &editor.lines[editor.cursor_row];
                if (editor.cursor_col < curr->size) {
                    line_delete_char(curr, editor.cursor_col);
                    editor.modified = true;
                    update_multiline_comments(&editor);
                } else if (editor.cursor_row + 1 < editor.line_count) {
                    Line *next = &editor.lines[editor.cursor_row + 1];
                    line_append_str(curr, next->chars, next->size);
                    line_free(next);
                    memmove(&editor.lines[editor.cursor_row + 1],
                            &editor.lines[editor.cursor_row + 2],
                            (editor.line_count - (editor.cursor_row + 2)) * sizeof(Line));
                    editor.line_count--;
                    editor.modified = true;
                    update_multiline_comments(&editor);
                }
            }
        } else if (IsKeyPressed(KEY_TAB)) {
            for (int k = 0; k < TAB_SIZE; ++k) {
                editor_insert_char(&editor, ' ', line_height, &shake_trauma);
            }
        } else {
            // Arrow Keys and Navigation with Shift-Selection
            int nav_key = 0;
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) nav_key = KEY_LEFT;
            else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) nav_key = KEY_RIGHT;
            else if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) nav_key = KEY_UP;
            else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) nav_key = KEY_DOWN;
            else if (IsKeyPressed(KEY_HOME)) nav_key = KEY_HOME;
            else if (IsKeyPressed(KEY_END)) nav_key = KEY_END;

            if (nav_key != 0) {
                if (shift && !editor.has_selection) {
                    editor.anchor_row = editor.cursor_row;
                    editor.anchor_col = editor.cursor_col;
                    editor.has_selection = true;
                } else if (!shift && editor.has_selection) {
                    editor_clear_selection(&editor);
                }

                if (ctrl && nav_key == KEY_LEFT) editor_move_word_left(&editor);
                else if (ctrl && nav_key == KEY_RIGHT) editor_move_word_right(&editor);
                else if (ctrl && nav_key == KEY_HOME) { editor.cursor_row = 0; editor.cursor_col = 0; }
                else if (ctrl && nav_key == KEY_END) { editor.cursor_row = editor.line_count - 1; editor.cursor_col = editor.lines[editor.cursor_row].size; }
                else if (nav_key == KEY_LEFT) {
                    if (editor.cursor_col > 0) editor.cursor_col--;
                    else if (editor.cursor_row > 0) {
                        editor.cursor_row--;
                        editor.cursor_col = editor.lines[editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_RIGHT) {
                    if (editor.cursor_col < editor.lines[editor.cursor_row].size) editor.cursor_col++;
                    else if (editor.cursor_row + 1 < editor.line_count) {
                        editor.cursor_row++;
                        editor.cursor_col = 0;
                    }
                } else if (nav_key == KEY_UP) {
                    if (editor.cursor_row > 0) {
                        editor.cursor_row--;
                        if (editor.cursor_col > editor.lines[editor.cursor_row].size)
                            editor.cursor_col = editor.lines[editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_DOWN) {
                    if (editor.cursor_row + 1 < editor.line_count) {
                        editor.cursor_row++;
                        if (editor.cursor_col > editor.lines[editor.cursor_row].size)
                            editor.cursor_col = editor.lines[editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_HOME) {
                    editor.cursor_col = 0;
                } else if (nav_key == KEY_END) {
                    editor.cursor_col = editor.lines[editor.cursor_row].size;
                }

                if (shift && editor.cursor_row == editor.anchor_row && editor.cursor_col == editor.anchor_col) {
                    editor.has_selection = false;
                }
            } else {
                int ch = GetCharPressed();
                while (ch > 0) {
                    if (ch >= 32 && ch <= 126) {
                        editor_insert_char(&editor, (char)ch, line_height, &shake_trauma);
                    }
                    ch = GetCharPressed();
                }
            }
        }

        // 7. Update Smooth Cursor
        float target_cur_x = get_line_col_x(font_syntax, &editor.lines[editor.cursor_row], editor.cursor_col, FONT_SIZE, FONT_SPACING);
        float target_cur_y = (float)editor.cursor_row * line_height;

        cursor.target = (Vector2){ target_cur_x, target_cur_y + 4.0f };
        cursor.trail = Vector2Lerp(cursor.trail, cursor.current, 10.0f * dt);
        cursor.current = Vector2Lerp(cursor.current, cursor.target, 22.0f * dt);

        // 8. Multi-Mode Camera Calculations
        Vector2 target_center = { 0 };
        float final_target_zoom = 1.0f;
        float gutter_space = 45.0f;

        if (current_cam_mode == CAM_MODE_BOUNDS_FIT) {
            float max_script_w = 0.0f;
            for (size_t i = 0; i < editor.line_count; ++i) {
                float lw = get_line_col_x(font_syntax, &editor.lines[i], editor.lines[i].size, FONT_SIZE, FONT_SPACING);
                if (lw > max_script_w) max_script_w = lw;
            }
            float script_box_w = fmaxf(max_script_w + gutter_space + 70.0f, 70.0f);
            float script_box_h = fmaxf((float)editor.line_count * line_height, line_height);

            target_center = (Vector2){ (script_box_w - gutter_space) * 0.5f, script_box_h * 0.5f };
            if (editor.line_count == 1 && editor.lines[0].size == 0) {
                target_center = (Vector2){ 0.0f, line_height * 0.5f };
            }

            float zoom_fit_x = ((float)screen_w * 0.70f) / script_box_w;
            float zoom_fit_y = ((float)screen_h * 0.70f) / script_box_h;
            float base_zoom = fminf(zoom_fit_x, zoom_fit_y);
            final_target_zoom = Clamp(base_zoom * user_zoom_mult, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);

        } else if (current_cam_mode == CAM_MODE_CURSOR_FOCUS) {
            target_center = (Vector2){ cursor.target.x + 20.0f, cursor.target.y + line_height * 0.5f };
            final_target_zoom = Clamp(1.30f * user_zoom_mult, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);

        } else if (current_cam_mode == CAM_MODE_LINE_FOCUS) {
            float curr_line_w = get_line_col_x(font_syntax, &editor.lines[editor.cursor_row], editor.lines[editor.cursor_row].size, FONT_SIZE, FONT_SPACING);
            target_center = (Vector2){ curr_line_w * 0.5f, (float)editor.cursor_row * line_height + line_height * 0.5f };

            float needed_w = fmaxf(curr_line_w + gutter_space + 140.0f, 320.0f);
            float line_zoom = ((float)screen_w * 0.80f) / needed_w;
            final_target_zoom = Clamp(line_zoom * user_zoom_mult, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);
        }

        camera.zoom = Lerp(camera.zoom, final_target_zoom, 5.5f * dt);
        camera.target = Vector2Lerp(camera.target, target_center, 6.0f * dt);

        float shake_intensity = shake_trauma * shake_trauma;
        float shake_mag = shake_intensity * (combo.streak > 25 ? 24.0f : 14.0f);
        Vector2 shake_offset = {
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag,
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag
        };
        camera.offset = (Vector2){
            (float)screen_w * 0.5f + shake_offset.x,
            (float)screen_h * 0.5f + shake_offset.y
        };

        UpdateParticles(dt);
        UpdateGlowFlashes(dt);

        BracketMatch bracket = FindMatchingBracket(&editor);

        // 9. Rendering
        BeginDrawing();
        ClearBackground(THEME.bg);

        BeginMode2D(camera);

        // Selection Highlighting
        if (editor.has_selection) {
            size_t sr, sc, er, ec;
            editor_get_selection_bounds(&editor, &sr, &sc, &er, &ec);
            for (size_t r = sr; r <= er; ++r) {
                Line *l = &editor.lines[r];
                size_t c_start = (r == sr) ? sc : 0;
                size_t c_end = (r == er) ? ec : l->size;

                float x0 = get_line_col_x(font_syntax, l, c_start, FONT_SIZE, FONT_SPACING);
                float x1 = get_line_col_x(font_syntax, l, c_end, FONT_SIZE, FONT_SPACING);
                float w = x1 - x0;
                if (w < 8.0f && r < er) w = 12.0f;

                DrawRectangle((int)x0, (int)(r * line_height + 4.0f), (int)w, (int)(line_height - 4.0f), THEME.selection);
            }
        }

        // Line Numbers
        for (size_t i = 0; i < editor.line_count; ++i) {
            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%zu", i + 1);
            Color nc = (i == editor.cursor_row) ? THEME.gutter_num_curr : THEME.gutter_num;
            DrawTextEx(font_body, num_str, (Vector2){ -gutter_space, (float)i * line_height + 4.0f }, FONT_SIZE - 2.0f, FONT_SPACING, nc);
        }

        // Code Syntax Lines
        for (size_t i = 0; i < editor.line_count; ++i) {
            draw_syntax_line(font_syntax, &editor.lines[i], 0.0f, (float)i * line_height + 4.0f);
        }

        // Bracket Match
        if (bracket.found) {
            float bx0 = get_line_col_x(font_syntax, &editor.lines[bracket.row], bracket.col, FONT_SIZE, FONT_SPACING);
            float bx1 = get_line_col_x(font_syntax, &editor.lines[bracket.row], bracket.col + 1, FONT_SIZE, FONT_SPACING);
            Rectangle b_rect = { bx0, (float)bracket.row * line_height + 4.0f, bx1 - bx0, line_height - 6.0f };

            if (!THEME.is_light) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangleRounded(b_rect, 0.4f, 4, THEME.bracket_match);
                EndBlendMode();
            }
            DrawRectangleRoundedLines(b_rect, 0.4f, 4, THEME.bracket_match);
        }

        DrawGlowFlashes();
        DrawParticles();

        // Smooth Caret Cursor
        bool blink = ((int)(GetTime() * 2.4)) % 2 == 0;
        if (blink || combo.streak > 0 || is_mouse_dragging) {
            float dist = Vector2Distance(cursor.current, cursor.trail);
            if (dist > 1.0f) {
                DrawRectangleV(cursor.trail, (Vector2){ cursor.width + dist * 0.4f, cursor.height }, THEME.cursor_trail);
            }
            DrawRectangleV(cursor.current, (Vector2){ cursor.width, cursor.height }, THEME.cursor);
        }

        EndMode2D();

        // 10. Minimap (Scaled)
        DrawMinimap(&editor, camera, screen_w, screen_h, line_height, ui_scale);

        // 11. Combo HUD (Scaled)
        if (combo.streak > 2) {
            char combo_text[64];
            snprintf(combo_text, sizeof(combo_text), "%d COMBO!", combo.streak);

            Color combo_color = THEME.cursor;
            if (combo.streak > 35) combo_color = (Color){ 255, 60, 60, 255 };
            else if (combo.streak > 20) combo_color = (Color){ 255, 130, 40, 255 };

            float combo_font_size = (FONT_SIZE + 12.0f) * combo.title_scale * ui_scale;
            Vector2 sz = MeasureTextEx(font_body, combo_text, combo_font_size, 2.0f);
            Vector2 c_pos = { (float)screen_w - sz.x - (160.0f * ui_scale), 30.0f * ui_scale };

            DrawTextEx(font_body, combo_text, c_pos, combo_font_size, 2.0f, combo_color);

            float bar_w = 120.0f * ui_scale;
            float bar_ratio = combo.decay_timer / combo.max_timer;
            DrawRectangle((int)c_pos.x, (int)(c_pos.y + sz.y + 4.0f * ui_scale), (int)bar_w, (int)(4.0f * ui_scale), ColorAlpha(THEME.gutter_num, 0.5f));
            DrawRectangle((int)c_pos.x, (int)(c_pos.y + sz.y + 4.0f * ui_scale), (int)(bar_w * bar_ratio), (int)(4.0f * ui_scale), combo_color);
        }

        // 12. Spotlight Flashlight Overlay
        DrawSpotlight(mouse_screen, screen_w, screen_h);

        // 13. CRT Scanline Effect
        if (enable_crt) DrawCRTEffect(screen_w, screen_h);

        // 14. Context Menu (Scaled)
        DrawContextMenu(&context_menu, screen_w, screen_h, ui_scale);

        // 15. Help Overlay (F1)
        if (show_help) {
            DrawRectangle(0, 0, screen_w, screen_h, (Color){ 0, 0, 0, 195 });
            float hw = 660.0f * ui_scale, hh = 470.0f * ui_scale;
            float hx = ((float)screen_w - hw) * 0.5f;
            float hy = ((float)screen_h - hh) * 0.5f;

            DrawRectangleRounded((Rectangle){ hx, hy, hw, hh }, 0.05f, 6, THEME.menu_bg);
            DrawRectangleRoundedLines((Rectangle){ hx, hy, hw, hh }, 0.05f, 6, THEME.menu_border);

            DrawTextEx(font_body, "ded - Cinematic Editor Controls", (Vector2){ hx + 25.0f * ui_scale, hy + 20.0f * ui_scale }, (FONT_SIZE + 3.0f) * ui_scale, 1.5f, THEME.cursor);
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
            float ly = hy + 65.0f * ui_scale;
            for (int i = 0; help_lines[i]; ++i) {
                DrawTextEx(font_body, help_lines[i], (Vector2){ hx + 25.0f * ui_scale, ly }, 14.0f * ui_scale, 1.0f, THEME.syn_default);
                ly += 24.0f * ui_scale;
            }
        }

        // Stats Footer Bar (Scaled)
        const char *cam_names[] = { "Script Fit", "Cursor Focus", "Line Focus" };
        char stats[256];
        snprintf(stats, sizeof(stats),
                 "ded | Cam: %s | UI: %.0f%% | Theme: %s | Zoom: %.2fx (User: %.0f%%) | F1: Help",
                 cam_names[current_cam_mode], ui_scale * 100.0f, THEME.name, camera.zoom, user_zoom_mult * 100.0f);
        DrawTextEx(font_body, stats, (Vector2){ 18.0f * ui_scale, (float)screen_h - (24.0f * ui_scale) }, 13.0f * ui_scale, 1.0f, THEME.status_text);

        EndDrawing();
    }

    if (spotlight_tex.id > 0) UnloadTexture(spotlight_tex);

    if (IsAudioDeviceReady()) {
        if (snd_typing.frameCount > 0) UnloadSound(snd_typing);
        if (snd_space.frameCount > 0) UnloadSound(snd_space);
        if (snd_enter.frameCount > 0) UnloadSound(snd_enter);
        if (snd_syntax_glow.frameCount > 0) UnloadSound(snd_syntax_glow);
        CloseAudioDevice();
    }

    editor_free(&editor);
    CloseWindow();
    return 0;
}