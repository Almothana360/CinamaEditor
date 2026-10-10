#include "fx/audio.h"
#include "core/event.h"
#include <stdlib.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static AudioSystem *g_audio_sys = NULL;
static const ComboSystem *g_audio_combo = NULL;

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

// Phase 5: Linear Phase-Accumulated Chirp Generator for Sci-Fi UI sounds
static Sound Audio_GenerateChirp(float freq_start, float freq_end, float duration, float volume) {
    int sample_rate = 44100;
    int frame_count = (int)(sample_rate * duration);
    short *data = (short *)malloc(frame_count * sizeof(short));
    if (!data) return (Sound){ 0 };

    float phase = 0.0f;
    for (int i = 0; i < frame_count; ++i) {
        float t = (float)i / (float)sample_rate;
        float progress = t / duration;

        // Linear frequency interpolation
        float freq = freq_start + (freq_end - freq_start) * progress;
        phase += freq / sample_rate;
        if (phase > 1.0f) phase -= 1.0f;

        float sample = sinf(2.0f * PI * phase);

        // Soft ADSR-style envelope to prevent popping
        float env = 1.0f;
        if (progress < 0.1f) env = progress / 0.1f;
        else if (progress > 0.7f) env = 1.0f - ((progress - 0.7f) / 0.3f);

        data[i] = (short)(sample * env * volume * 18000.0f);
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

static void Audio_PlayKeyInternal(int key, int combo_count) {
    if (!g_audio_sys || !g_audio_sys->is_ready) return;
    float combo_pitch = 1.0f + fminf((float)combo_count * 0.015f, 0.65f);

    if (key == KEY_SPACE && g_audio_sys->snd_space.frameCount > 0) {
        SetSoundPitch(g_audio_sys->snd_space, combo_pitch * 0.95f);
        PlaySound(g_audio_sys->snd_space);
    } else if (key == KEY_ENTER && g_audio_sys->snd_enter.frameCount > 0) {
        SetSoundPitch(g_audio_sys->snd_enter, combo_pitch * 1.1f);
        PlaySound(g_audio_sys->snd_enter);
    } else if (g_audio_sys->snd_typing.frameCount > 0) {
        float jitter = ((float)GetRandomValue(-8, 8) / 100.0f);
        SetSoundPitch(g_audio_sys->snd_typing, combo_pitch + jitter);
        PlaySound(g_audio_sys->snd_typing);
    }
}

static void Audio_PlayGlowInternal(void) {
    if (!g_audio_sys || !g_audio_sys->is_ready || g_audio_sys->snd_syntax_glow.frameCount == 0) return;
    SetSoundPitch(g_audio_sys->snd_syntax_glow, 1.0f + ((float)GetRandomValue(0, 15) / 100.0f));
    PlaySound(g_audio_sys->snd_syntax_glow);
}

void Audio_PlayUI(UISoundType type) {
    if (!g_audio_sys || !g_audio_sys->is_ready) return;
    switch (type) {
        case SND_UI_HOVER:
            if (g_audio_sys->snd_ui_hover.frameCount > 0) PlaySound(g_audio_sys->snd_ui_hover);
            break;
        case SND_UI_SELECT:
            if (g_audio_sys->snd_ui_select.frameCount > 0) PlaySound(g_audio_sys->snd_ui_select);
            break;
        case SND_UI_OPEN:
            if (g_audio_sys->snd_ui_open.frameCount > 0) PlaySound(g_audio_sys->snd_ui_open);
            break;
        case SND_UI_CLOSE:
            if (g_audio_sys->snd_ui_close.frameCount > 0) PlaySound(g_audio_sys->snd_ui_close);
            break;
    }
}

static void Audio_OnEvent(EventType type, const void *payload) {
    if (!g_audio_sys || !g_audio_combo) return;

    if (type == EV_ACTION) {
        const ActionPayload *p = (const ActionPayload *)payload;
        if (p->action == ACTION_DELETE_BACKWARD || p->action == ACTION_DELETE_FORWARD ||
            p->action == ACTION_DELETE_WORD_BACKWARD || p->action == ACTION_DELETE_WORD_FORWARD) {
            Audio_PlayKeyInternal(KEY_BACKSPACE, g_audio_combo->streak);
        }
    } else if (type == EV_COMBO_HIT) {
        const ComboHitPayload *cp = (const ComboHitPayload *)payload;
        if (cp->trigger_action == ACTION_INSERT_NEWLINE) {
            Audio_PlayKeyInternal(KEY_ENTER, cp->new_streak);
        } else if (cp->trigger_action == ACTION_INSERT_CHAR) {
            int key = (cp->char_data == ' ' || cp->char_data == '\t') ? KEY_SPACE : KEY_A;
            Audio_PlayKeyInternal(key, cp->new_streak);
        }
    } else if (type == EV_TOKEN_COMPLETED) {
        Audio_PlayGlowInternal();
    }
}

void Audio_Init(AudioSystem *as, const ComboSystem *combo) {
    if (!as) return;
    InitAudioDevice();
    as->is_ready = IsAudioDeviceReady();

    // Text Editor Feedback
    as->snd_typing = Audio_GenerateSynth(820.0f, 0.045f, 48.0f, 0.85f);
    as->snd_space = Audio_GenerateSynth(380.0f, 0.07f, 32.0f, 1.0f);
    as->snd_enter = Audio_GenerateSynth(1050.0f, 0.12f, 22.0f, 0.9f);
    as->snd_syntax_glow = Audio_GenerateSynth(1400.0f, 0.22f, 16.0f, 1.0f);

    // Phase 5: Game UI Sci-Fi Feedback
    as->snd_ui_hover  = Audio_GenerateChirp(1400.0f, 1400.0f, 0.02f, 0.15f);     // Short blip
    as->snd_ui_select = Audio_GenerateChirp(200.0f, 40.0f, 0.18f, 0.85f);        // Heavy Thud
    as->snd_ui_open   = Audio_GenerateChirp(180.0f, 850.0f, 0.20f, 0.45f);       // Electronic Sweep Up
    as->snd_ui_close  = Audio_GenerateChirp(850.0f, 180.0f, 0.15f, 0.45f);       // Electronic Sweep Down

    g_audio_sys = as;
    g_audio_combo = combo;

    Event_Subscribe(EV_ACTION, Audio_OnEvent);
    Event_Subscribe(EV_COMBO_HIT, Audio_OnEvent);
    Event_Subscribe(EV_TOKEN_COMPLETED, Audio_OnEvent);
}

void Audio_Close(AudioSystem *as) {
    if (!as || !as->is_ready) return;
    if (as->snd_typing.frameCount > 0)      UnloadSound(as->snd_typing);
    if (as->snd_space.frameCount > 0)       UnloadSound(as->snd_space);
    if (as->snd_enter.frameCount > 0)       UnloadSound(as->snd_enter);
    if (as->snd_syntax_glow.frameCount > 0) UnloadSound(as->snd_syntax_glow);

    if (as->snd_ui_hover.frameCount > 0)    UnloadSound(as->snd_ui_hover);
    if (as->snd_ui_select.frameCount > 0)   UnloadSound(as->snd_ui_select);
    if (as->snd_ui_open.frameCount > 0)     UnloadSound(as->snd_ui_open);
    if (as->snd_ui_close.frameCount > 0)    UnloadSound(as->snd_ui_close);

    CloseAudioDevice();
    as->is_ready = false;
}