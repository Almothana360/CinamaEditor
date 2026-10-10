#include "ui/pause_menu.h"
#include "core/event.h"
#include "fx/audio.h"
#include "raymath.h"
#include <string.h>
#include <math.h>

typedef struct {
    const char *label;
    ActionType action;
} PauseMenuItem;

// Game-style action routing linking directly to the Mission Select Datapad
static const PauseMenuItem PAUSE_ITEMS[] = {
    { "RESUME", ACTION_NONE },
    { "DATA ARCHIVES // MISSION SELECT", ACTION_CHANGE_DIR },
    { "SAVE STATE", ACTION_SAVE },
    { "CYCLE VISUAL THEME", ACTION_CYCLE_THEME },
    { "TOGGLE CRT OVERDRIVE", ACTION_TOGGLE_CRT },
    { "SYSTEM CONTROLS", ACTION_TOGGLE_HELP }
};
static const int PAUSE_ITEM_COUNT = 6;

static bool g_pause_open = false;
static bool g_pause_active = false;
static float g_pause_anim = 0.0f;
static float g_pause_vel = 0.0f;
static int g_pause_selected = 0;

void PauseMenu_Init(void) {
    g_pause_open = false;
    g_pause_active = false;
    g_pause_anim = 0.0f;
    g_pause_vel = 0.0f;
    g_pause_selected = 0;
}

void PauseMenu_Close(void) {
    if (g_pause_open) {
        g_pause_open = false;
        Audio_PlayUI(SND_UI_CLOSE);
    }
}

void PauseMenu_Toggle(void) {
    g_pause_open = !g_pause_open;
    if (g_pause_open) {
        g_pause_active = true;
        g_pause_selected = 0;
        Audio_PlayUI(SND_UI_OPEN);
    } else {
        Audio_PlayUI(SND_UI_CLOSE);
    }
}

bool PauseMenu_IsOpen(void) {
    return g_pause_active;
}

bool PauseMenu_Update(Vector2 mouse_screen, int screen_w, int screen_h, float scale) {
    // High-tension spring physics update
    float dt = GetFrameTime();
    float target = g_pause_open ? 1.0f : 0.0f;
    float tension = 300.0f;
    float damping = 2.0f * sqrtf(tension) * 0.8f;

    g_pause_vel += (target - g_pause_anim) * tension * dt;
    g_pause_vel -= g_pause_vel * damping * dt;
    g_pause_anim += g_pause_vel * dt;

    if (!g_pause_open && g_pause_anim < 0.005f) {
        g_pause_active = false;
        g_pause_anim = 0.0f;
    }

    if (!g_pause_active) return false;
    if (!g_pause_open) return true; // Consumes input while sliding away

    // Track mouse movement to prevent idle mouse from hijacking keyboard arrow navigation
    static Vector2 last_mouse = { -1, -1 };
    bool mouse_moved = Vector2Distance(mouse_screen, last_mouse) > 1.0f;
    if (mouse_moved) {
        last_mouse = mouse_screen;
    }

    // Keyboard Navigation
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        g_pause_selected = (g_pause_selected - 1 + PAUSE_ITEM_COUNT) % PAUSE_ITEM_COUNT;
        Audio_PlayUI(SND_UI_HOVER);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        g_pause_selected = (g_pause_selected + 1) % PAUSE_ITEM_COUNT;
        Audio_PlayUI(SND_UI_HOVER);
    }

    float s = (scale > 0.1f) ? scale : 1.0f;
    float item_h = 65.0f * s;
    float start_y = ((float)screen_h - ((float)PAUSE_ITEM_COUNT * item_h)) * 0.5f;

    bool clicked_item = false;

    // Hit Testing & Execution
    for (int i = 0; i < PAUSE_ITEM_COUNT; ++i) {
        // Constrain hitbox perfectly to the center button area, not the full screen width
        float btn_w = 500.0f * s;
        float btn_x = ((float)screen_w - btn_w) * 0.5f;
        Rectangle btn_rect = { btn_x, start_y + (float)i * item_h, btn_w, item_h };

        if (CheckCollisionPointRec(mouse_screen, btn_rect)) {
            if (mouse_moved && g_pause_selected != i) {
                g_pause_selected = i;
                Audio_PlayUI(SND_UI_HOVER);
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                clicked_item = true;
                Audio_PlayUI(SND_UI_SELECT);
                if (PAUSE_ITEMS[i].action != ACTION_NONE) {
                    ActionPayload p = { .action = PAUSE_ITEMS[i].action };
                    Event_Emit(EV_ACTION, &p);
                }
                PauseMenu_Close();
                return true;
            }
        }
    }

    // Execution via Keyboard Enter
    if (IsKeyPressed(KEY_ENTER)) {
        Audio_PlayUI(SND_UI_SELECT);
        if (PAUSE_ITEMS[g_pause_selected].action != ACTION_NONE) {
            ActionPayload p = { .action = PAUSE_ITEMS[g_pause_selected].action };
            Event_Emit(EV_ACTION, &p);
        }
        PauseMenu_Close();
        return true;
    }

    // Dismissal via Click Outside
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !clicked_item) {
        PauseMenu_Close();
        return true;
    }

    // Dismissal via Escape
    if (IsKeyPressed(KEY_ESCAPE)) {
        PauseMenu_Close();
        return true;
    }

    return true; // Modal consumes all input while active
}

void PauseMenu_Draw(int screen_w, int screen_h, float scale, const Theme *theme, Font font_body) {
    if (!g_pause_active || !theme) return;

    float s = (scale > 0.1f) ? scale : 1.0f;
    float alpha = g_pause_anim;

    // 1. Heavy cinematic backdrop overlay
    DrawRectangle(0, 0, screen_w, screen_h, ColorAlpha(theme->bg, 0.90f * alpha));

    // 2. Retro scanlines
    for (int y = 0; y < screen_h; y += 4) {
        DrawRectangle(0, y, screen_w, 1, ColorAlpha(theme->gutter_num, 0.12f * alpha));
    }

    // Glitch Offset (Shakes horizontally while entering/exiting)
    float glitch_offset = 0.0f;
    if (g_pause_anim < 0.98f) {
        glitch_offset = sinf(GetTime() * 80.0f) * 40.0f * (1.0f - g_pause_anim);
    }

    // 3. Status Header
    DrawTextEx(font_body, "S Y S T E M   P A U S E D", (Vector2){ 40.0f * s + glitch_offset, 40.0f * s }, 16.0f * s, 4.0f, ColorAlpha(theme->cursor, alpha));

    // 4. Centered Menu Items
    float item_h = 65.0f * s;
    float start_y = ((float)screen_h - ((float)PAUSE_ITEM_COUNT * item_h)) * 0.5f;

    for (int i = 0; i < PAUSE_ITEM_COUNT; ++i) {
        bool is_selected = (i == g_pause_selected);

        float font_size = is_selected ? (36.0f * s) : (28.0f * s);
        Color text_col = is_selected ? theme->cursor : ColorAlpha(theme->syn_default, 0.6f);
        text_col = ColorAlpha(text_col, alpha);

        const char *text = PAUSE_ITEMS[i].label;
        Vector2 sz = MeasureTextEx(font_body, text, font_size, 2.0f);

        // Apply alternating glitch offsets to rows
        float row_glitch = (i % 2 == 0) ? glitch_offset : -glitch_offset;

        float btn_w = 500.0f * s;
        float btn_x = ((float)screen_w - btn_w) * 0.5f + row_glitch;

        float text_x = ((float)screen_w - sz.x) * 0.5f + row_glitch;
        float y = start_y + (float)i * item_h + (item_h - sz.y) * 0.5f;

        if (is_selected) {
            // Draw localized button background instead of full-screen width
            DrawRectangle((int)btn_x, (int)(start_y + (float)i * item_h), (int)btn_w, (int)item_h, ColorAlpha(theme->menu_hl, 0.45f * alpha));

            float bracket_w = 20.0f * s;
            float bracket_h = 4.0f * s;
            float bracket_y = y + sz.y * 0.5f - bracket_h * 0.5f;

            DrawRectangle((int)(text_x - 40.0f * s), (int)bracket_y, (int)bracket_w, (int)bracket_h, ColorAlpha(theme->cursor, alpha));
            DrawRectangle((int)(text_x + sz.x + 20.0f * s), (int)bracket_y, (int)bracket_w, (int)bracket_h, ColorAlpha(theme->cursor, alpha));
        }

        DrawTextEx(font_body, text, (Vector2){ text_x, y }, font_size, 2.0f, text_col);
    }
}