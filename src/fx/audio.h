#ifndef CE_FX_AUDIO_H
#define CE_FX_AUDIO_H

#include "raylib.h"
#include "ui/ui.h"
#include <stdbool.h>

typedef struct {
    Sound snd_typing;
    Sound snd_space;
    Sound snd_enter;
    Sound snd_syntax_glow;
    bool is_ready;
} AudioSystem;

void Audio_Init(AudioSystem *as, const ComboSystem *combo);
void Audio_Close(AudioSystem *as);

#endif // CE_FX_AUDIO_H