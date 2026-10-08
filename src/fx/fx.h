#ifndef CE_FX_FX_H
#define CE_FX_FX_H

#include "raylib.h"
#include "core/types.h"
#include "core/theme.h"
#include <stdbool.h>

/**
 * Individual kinematic spark particle.
 */
typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
    float max_life;
    float size;
    bool active;
} Particle;

/**
 * Radial blooming token flash.
 */
typedef struct {
    Rectangle rect;
    Color color;
    float life;
    float max_life;
    bool active;
} GlowFlash;

/**
 * Container holding state for all active visual effects, shaders, and trauma.
 */
typedef struct {
    Particle particles[CE_MAX_PARTICLES];
    GlowFlash glow_flashes[CE_MAX_GLOW_FLASHES];
    Texture2D spotlight_tex;
    bool enable_spotlight;
    bool enable_crt;
    float shake_trauma;
} FxSystem;

/**
 * Initializes particles, glow arrays, and generates procedural textures.
 */
void Fx_Init(FxSystem *fx);

/**
 * Unloads procedural GPU textures.
 */
void Fx_Close(FxSystem *fx);

/**
 * Spawns an explosion of spark particles at a world-space location.
 */
void Fx_EmitParticles(FxSystem *fx, Vector2 pos, Color col, int count, bool power_mode);

/**
 * Registers an expanding glow burst over a syntax token box.
 */
void Fx_TriggerGlow(FxSystem *fx, Rectangle rect, Color col);

/**
 * Adds camera shake trauma (clamped to 1.0f).
 */
void Fx_AddTrauma(FxSystem *fx, float amount);

/**
 * Advances particle lifespans, dampens velocities, and decays camera shake trauma.
 */
void Fx_Update(FxSystem *fx, float dt);

/**
 * Renders in-world visual elements (glow flashes and spark particles) with additive blending.
 */
void Fx_DrawWorld(const FxSystem *fx);

/**
 * Renders the circular spotlight aperture around the mouse cursor in screen space.
 */
void Fx_DrawSpotlight(const FxSystem *fx, Vector2 mouse_pos, int screen_w, int screen_h, const Theme *theme);

/**
 * Renders retro CRT scanlines and dark vignette borders in screen space.
 */
void Fx_DrawCRT(int screen_w, int screen_h);

#endif // CE_FX_FX_H