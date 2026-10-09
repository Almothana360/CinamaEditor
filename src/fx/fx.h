#ifndef CE_FX_FX_H
#define CE_FX_FX_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include "modules/buffer/document.h"
#include "ui/ui.h"
#include <stdbool.h>

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
    Particle particles[CE_MAX_PARTICLES];
    GlowFlash glow_flashes[CE_MAX_GLOW_FLASHES];
    Texture2D spotlight_tex;
    bool enable_spotlight;
    bool enable_crt;
    float shake_trauma;
} FxSystem;

void Fx_Init(FxSystem *fx, const Document *doc, const ComboSystem *combo, Font font_syntax);
void Fx_Close(FxSystem *fx);
void Fx_Update(FxSystem *fx, float dt);
void Fx_DrawWorld(const FxSystem *fx);
void Fx_DrawSpotlight(const FxSystem *fx, Vector2 mouse_pos, int screen_w, int screen_h, const Theme *theme);
void Fx_DrawCRT(int screen_w, int screen_h);

#endif // CE_FX_FX_H