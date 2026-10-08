#include "fx/audio.h"
#include <stdlib.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/**
 * Synthesizes an in-memory decaying sine wave as a procedural fallback.
 */
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

void Audio_Init(AudioSystem *as) {
    if (!as) return;
    InitAudioDevice();
    as->is_ready = IsAudioDeviceReady();

    // Standard character typing click
    if (FileExists("type.wav")) as->snd_typing = LoadSound("type.wav");
    else as->snd_typing = Audio_GenerateSynth(820.0f, 0.045f, 48.0f, 0.85f);

    // Deeper mechanical spacebar thud
    if (FileExists("space.wav")) as->snd_space = LoadSound("space.wav");
    else as->snd_space = Audio_GenerateSynth(380.0f, 0.07f, 32.0f, 1.0f);

    // Enter / return chime
    if (FileExists("enter.wav")) as->snd_enter = LoadSound("enter.wav");
    else as->snd_enter = Audio_GenerateSynth(1050.0f, 0.12f, 22.0f, 0.9f);

    // Syntax glow chord
    if (FileExists("glow.wav")) as->snd_syntax_glow = LoadSound("glow.wav");
    else as->snd_syntax_glow = Audio_GenerateSynth(1400.0f, 0.22f, 16.0f, 1.0f);
}

void Audio_Close(AudioSystem *as) {
    if (!as || !as->is_ready) return;

    if (as->snd_typing.frameCount > 0)      UnloadSound(as->snd_typing);
    if (as->snd_space.frameCount > 0)       UnloadSound(as->snd_space);
    if (as->snd_enter.frameCount > 0)       UnloadSound(as->snd_enter);
    if (as->snd_syntax_glow.frameCount > 0) UnloadSound(as->snd_syntax_glow);

    CloseAudioDevice();
    as->is_ready = false;
}

void Audio_PlayKey(AudioSystem *as, int key, int combo_count) {
    if (!as || !as->is_ready) return;

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

void Audio_PlayGlow(AudioSystem *as) {
    if (!as || !as->is_ready || as->snd_syntax_glow.frameCount == 0) return;
    SetSoundPitch(as->snd_syntax_glow, 1.0f + ((float)GetRandomValue(0, 15) / 100.0f));
    PlaySound(as->snd_syntax_glow);
}