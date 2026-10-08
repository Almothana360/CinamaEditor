#include "fx/fx.h"
#include <string.h>
#include <math.h>
#include <raymath.h>

void Fx_Init(FxSystem *fx) {
    if (!fx) return;
    memset(fx, 0, sizeof(FxSystem));

    // Generate smooth radial falloff texture for the spotlight aperture
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
                // Smoothstep interpolation
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

void Fx_Close(FxSystem *fx) {
    if (!fx) return;
    if (fx->spotlight_tex.id > 0) {
        UnloadTexture(fx->spotlight_tex);
        fx->spotlight_tex.id = 0;
    }
}

void Fx_EmitParticles(FxSystem *fx, Vector2 pos, Color col, int count, bool power_mode) {
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
                    fx->particles[p].color = (Color){
                        (unsigned char)GetRandomValue(220, 255),
                        (unsigned char)GetRandomValue(80, 220),
                        40,
                        255
                    };
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

void Fx_TriggerGlow(FxSystem *fx, Rectangle rect, Color col) {
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

void Fx_AddTrauma(FxSystem *fx, float amount) {
    if (!fx) return;
    fx->shake_trauma = fminf(fx->shake_trauma + amount, 1.0f);
}

void Fx_Update(FxSystem *fx, float dt) {
    if (!fx) return;

    // Decay trauma over time
    if (fx->shake_trauma > 0.0f) {
        fx->shake_trauma = fmaxf(fx->shake_trauma - dt * 2.5f, 0.0f);
    }

    // Update active particles
    for (int i = 0; i < CE_MAX_PARTICLES; ++i) {
        if (!fx->particles[i].active) continue;
        fx->particles[i].life += dt;
        if (fx->particles[i].life >= fx->particles[i].max_life) {
            fx->particles[i].active = false;
            continue;
        }
        fx->particles[i].pos = Vector2Add(fx->particles[i].pos, Vector2Scale(fx->particles[i].vel, dt));
        fx->particles[i].vel = Vector2Scale(fx->particles[i].vel, 0.92f); // Air resistance
    }

    // Update active glow flashes
    for (int i = 0; i < CE_MAX_GLOW_FLASHES; ++i) {
        if (!fx->glow_flashes[i].active) continue;
        fx->glow_flashes[i].life += dt;
        if (fx->glow_flashes[i].life >= fx->glow_flashes[i].max_life) {
            fx->glow_flashes[i].active = false;
        }
    }
}

void Fx_DrawWorld(const FxSystem *fx) {
    if (!fx) return;

    BeginBlendMode(BLEND_ADDITIVE);

    // 1. Draw expanding glow rectangles
    for (int i = 0; i < CE_MAX_GLOW_FLASHES; ++i) {
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

    // 2. Draw spark particles
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

    // Darken margins extending beyond texture boundaries
    Color dark = (Color){ 4, 5, 8, 242 };
    if (dst.y > 0) {
        DrawRectangle(0, 0, screen_w, (int)dst.y, dark);
    }
    if (dst.y + dst.height < screen_h) {
        DrawRectangle(0, (int)(dst.y + dst.height), screen_w, screen_h - (int)(dst.y + dst.height), dark);
    }
    if (dst.x > 0) {
        DrawRectangle(0, (int)dst.y, (int)dst.x, (int)dst.height, dark);
    }
    if (dst.x + dst.width < screen_w) {
        DrawRectangle((int)(dst.x + dst.width), (int)dst.y, screen_w - (int)(dst.x + dst.width), (int)dst.height, dark);
    }

    // Reticle highlight
    DrawCircleLines((int)mouse_pos.x, (int)mouse_pos.y, 45.0f, ColorAlpha(theme->cursor, 0.35f));
}

void Fx_DrawCRT(int screen_w, int screen_h) {
    // Horizontal scanlines
    Color scanline = (Color){ 0, 0, 0, 24 };
    for (int y = 0; y < screen_h; y += 3) {
        DrawRectangle(0, y, screen_w, 1, scanline);
    }

    // Soft vignette gradients
    DrawRectangleGradientH(0, 0, 100, screen_h, (Color){ 0, 0, 0, 130 }, BLANK);
    DrawRectangleGradientH(screen_w - 100, 0, 100, screen_h, BLANK, (Color){ 0, 0, 0, 130 });
    DrawRectangleGradientV(0, 0, screen_w, 70, (Color){ 0, 0, 0, 140 }, BLANK);
    DrawRectangleGradientV(0, screen_h - 70, screen_w, 70, BLANK, (Color){ 0, 0, 0, 140 });
}