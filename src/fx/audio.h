#ifndef CE_FX_AUDIO_H
#define CE_FX_AUDIO_H

#include "raylib.h"
#include <stdbool.h>

/**
 * Audio subsystem context holding loaded sounds and readiness state.
 */
typedef struct {
    Sound snd_typing;
    Sound snd_space;
    Sound snd_enter;
    Sound snd_syntax_glow;
    bool is_ready;
} AudioSystem;

/**
 * Initializes the audio device and loads or synthesizes all sound effects.
 */
void Audio_Init(AudioSystem *as);

/**
 * Unloads sound waveforms and closes the audio hardware device.
 */
void Audio_Close(AudioSystem *as);

/**
 * Plays the appropriate key click or chime with combo-based pitch modulation.
 */
void Audio_PlayKey(AudioSystem *as, int key, int combo_count);

/**
 * Plays the harmonic chord when a keyword or syntax token is completed.
 */
void Audio_PlayGlow(AudioSystem *as);

#endif // CE_FX_AUDIO_H