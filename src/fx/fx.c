#include "fx/fx.h"
#include "core/event.h"
#include "buffer/line.h"
#include <string.h>
#include <math.h>
#include <raymath.h>

static const Document *g_fx_doc = NULL;
static const ComboSystem *g_fx_combo = NULL;
static const Theme *g_fx_theme = NULL;
static Font g_fx_font = {0};
static FxSystem *g_fx_sys = NULL;

static void Fx_EmitParticlesInternal(FxSystem *fx, Vector2 pos, Color col, int count, bool power_mode) {
    if (!fx) return;
    for (int i = 0; i < count; ++i) {
        for (int p = 0; p < CE_MAX_PARTICLES; ++p) {
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

static void Fx_TriggerGlowInternal(FxSystem *fx, Rectangle rect, Color col) {
    if (!fx) return;
    for (int i = 0; i < CE_MAX_GLOW_FLASHES; ++i) {
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

static void Fx_AddTraumaInternal(FxSystem *fx, float amount) {
    if (!fx) return;
    fx->shake_trauma = fminf(fx->shake_trauma + amount, 1.0f);
}

static void Fx_OnEvent(EventType type, const void *payload) {
    if (!g_fx_sys) return;

    if (type == EV_THEME_CHANGED) {
        const ThemeChangedPayload *p = payload;
        g_fx_theme = (const Theme *)p->theme;
    } else if (type == EV_ACTION) {
        const ActionPayload *p = payload;
        if (p->action == ACTION_DELETE_BACKWARD || p->action == ACTION_DELETE_FORWARD ||
            p->action == ACTION_DELETE_WORD_BACKWARD || p->action == ACTION_DELETE_WORD_FORWARD) {
            Fx_AddTraumaInternal(g_fx_sys, 0.16f);
        } else if (p->action == ACTION_TOGGLE_CRT) {
            g_fx_sys->enable_crt = !g_fx_sys->enable_crt;
        } else if (p->action == ACTION_TOGGLE_SPOTLIGHT) {
            g_fx_sys->enable_spotlight = !g_fx_sys->enable_spotlight;
        }
    } else if (type == EV_COMBO_HIT) {
        const ComboHitPayload *cp = payload;
        if (cp->trigger_action == ACTION_INSERT_NEWLINE) {
            Fx_AddTraumaInternal(g_fx_sys, 0.30f);
        } else if (cp->trigger_action == ACTION_INSERT_CHAR) {
            if (cp->char_data != '\t' && cp->char_data != ' ') {
                Fx_AddTraumaInternal(g_fx_sys, (cp->new_streak > 20) ? 0.35f : 0.22f);
            }
            if (cp->char_data >= 32 && cp->char_data <= 126 && g_fx_doc && g_fx_theme) {
                float line_height = CE_FONT_SIZE + 8.0f;
                float cur_x = Line_GetColX(g_fx_font, &g_fx_doc->lines[g_fx_doc->cursor_row], g_fx_doc->cursor_col, CE_FONT_SIZE, CE_FONT_SPACING);
                Vector2 spark_pos = { cur_x, (float)g_fx_doc->cursor_row * line_height + line_height * 0.5f };
                Fx_EmitParticlesInternal(g_fx_sys, spark_pos, g_fx_theme->cursor, cp->new_streak > 15 ? 12 : 7, cp->new_streak > 25);
            }
        }
    } else if (type == EV_TOKEN_COMPLETED) {
        const TokenCompletedPayload *tp = payload;
        if (g_fx_doc && g_fx_theme) {
            float line_height = CE_FONT_SIZE + 8.0f;
            float x0 = Line_GetColX(g_fx_font, &g_fx_doc->lines[tp->row], tp->start_col, CE_FONT_SIZE, CE_FONT_SPACING);
            float x1 = Line_GetColX(g_fx_font, &g_fx_doc->lines[tp->row], tp->end_col, CE_FONT_SIZE, CE_FONT_SPACING);
            Rectangle rect = { x0, (float)tp->row * line_height, x1 - x0, line_height };
            Fx_TriggerGlowInternal(g_fx_sys, rect, tp->color);
            Fx_EmitParticlesInternal(g_fx_sys, (Vector2){ x1, (float)tp->row * line_height + line_height * 0.5f }, tp->color, 25, g_fx_combo ? (g_fx_combo->streak > 15) : false);
        }
    }
}

void Fx_Init(FxSystem *fx, const Document *doc, const ComboSystem *combo, Font font_syntax) {
    if (!fx) return;
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

    g_fx_sys = fx;
    g_fx_doc = doc;
    g_fx_combo = combo;
    g_fx_font = font_syntax;

    Event_Subscribe(EV_THEME_CHANGED, Fx_OnEvent);
    Event_Subscribe(EV_ACTION, Fx_OnEvent);
    Event_Subscribe(EV_COMBO_HIT, Fx_OnEvent);
    Event_Subscribe(EV_TOKEN_COMPLETED, Fx_OnEvent);
}

void Fx_Close(FxSystem *fx) {
    if (!fx) return;
    if (fx->spotlight_tex.id > 0) {
        UnloadTexture(fx->spotlight_tex);
        fx->spotlight_tex.id = 0;
    }
}

void Fx_Update(FxSystem *fx, float dt) {
    if (!fx) return;
    if (fx->shake_trauma > 0.0f) {
        fx->shake_trauma = fmaxf(fx->shake_trauma - dt * 2.5f, 0.0f);
    }
    for (int i = 0; i < CE_MAX_PARTICLES; ++i) {
        if (!fx->particles[i].active) continue;
        fx->particles[i].life += dt;
        if (fx->particles[i].life >= fx->particles[i].max_life) {
            fx->particles[i].active = false;
            continue;
        }
        fx->particles[i].pos = Vector2Add(fx->particles[i].pos, Vector2Scale(fx->particles[i].vel, dt));
        fx->particles[i].vel = Vector2Scale(fx->particles[i].vel, 0.92f);
    }
    for (int i = 0; i < CE_MAX_GLOW_FLASHES; ++i) {
        if (!fx->glow_flashes[i].active) continue;
        fx->glow_flashes[i].life += dt;
        if (fx->glow_flashes[i].life >= fx->glow_flashes[i].max_life) {
            fx->glow_flashes[i].active = false;
        }
    }
}

// ... DrawWorld, DrawSpotlight, DrawCRT remain untouched ...
void Fx_DrawWorld(const FxSystem *fx) {
    if (!fx) return;
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < CE_MAX_GLOW_FLASHES; ++i) {
        if (!fx->glow_flashes[i].active) continue;
        float t = fx->glow_flashes[i].life / fx->glow_flashes[i].max_life;
        float alpha = (1.0f - t) * 0.8f;
        Color c = fx->glow_flashes[i].color;
        c.a = (unsigned char)(alpha * 255.0f);
        Rectangle r = fx->glow_flashes[i].rect;
        float exp_val = t * 16.0f;
        DrawRectangleRounded((Rectangle){ r.x - exp_val, r.y - exp_val * 0.5f, r.width + exp_val * 2.0f, r.height + exp_val }, 0.35f, 4, c);
    }
    for (int i = 0; i < CE_MAX_PARTICLES; ++i) {
        if (!fx->particles[i].active) continue;
        float progress = fx->particles[i].life / fx->particles[i].max_life;
        float alpha = 1.0f - progress;
        Color c = fx->particles[i].color;
        c.a = (unsigned char)(alpha * 255.0f);
        DrawCircleV(fx->particles[i].pos, fx->particles[i].size * (1.0f - progress * 0.4f), c);
    }
    EndBlendMode();
}

void Fx_DrawSpotlight(const FxSystem *fx, Vector2 mouse_pos, int screen_w, int screen_h, const Theme *theme) {
    if (!fx || !fx->enable_spotlight || fx->spotlight_tex.id == 0 || !theme) return;
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

void Fx_DrawCRT(int screen_w, int screen_h) {
    Color scanline = (Color){ 0, 0, 0, 24 };
    for (int y = 0; y < screen_h; y += 3) {
        DrawRectangle(0, y, screen_w, 1, scanline);
    }
    DrawRectangleGradientH(0, 0, 100, screen_h, (Color){ 0, 0, 0, 130 }, BLANK);
    DrawRectangleGradientH(screen_w - 100, 0, 100, screen_h, BLANK, (Color){ 0, 0, 0, 130 });
    DrawRectangleGradientV(0, 0, screen_w, 70, (Color){ 0, 0, 0, 140 }, BLANK);
    DrawRectangleGradientV(0, screen_h - 70, screen_w, 70, BLANK, (Color){ 0, 0, 0, 140 });
}