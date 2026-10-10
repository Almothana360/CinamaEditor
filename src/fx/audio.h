#ifndef CE_FX_AUDIO_H
#define CE_FX_AUDIO_H

#include "raylib.h"
#include "ui/ui.h"
#include <stdbool.h>

// Game-style UI Sound Categories
typedef enum {
    SND_UI_HOVER = 0,
    SND_UI_SELECT,
    SND_UI_OPEN,
    SND_UI_CLOSE
} UISoundType;

typedef struct {
    Sound snd_typing;
    Sound snd_space;
    Sound snd_enter;
    Sound snd_syntax_glow;

    // Phase 5: UI Interface Audio
    Sound snd_ui_hover;
    Sound snd_ui_select;
    Sound snd_ui_open;
    Sound snd_ui_close;

    bool is_ready;
} AudioSystem;

void Audio_Init(AudioSystem *as, const ComboSystem *combo);
void Audio_Close(AudioSystem *as);

// Triggers a UI sound effect immediately
void Audio_PlayUI(UISoundType type);

#endif // CE_FX_AUDIO_H