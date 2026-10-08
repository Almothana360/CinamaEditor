#ifndef CE_CORE_THEME_H
#define CE_CORE_THEME_H

#include "raylib.h"
#include <stdbool.h>

#define CE_THEME_COUNT    4
#define CE_UI_SCALE_COUNT 8

// Backward compatibility macros
#define THEME_COUNT       CE_THEME_COUNT
#define UI_SCALE_COUNT    CE_UI_SCALE_COUNT

typedef struct {
    const char *name;
    Color bg;
    Color gutter_bg;
    Color gutter_num;
    Color gutter_num_curr;
    Color cursor;
    Color cursor_trail;
    Color selection;
    Color line_hl;
    Color bracket_match;
    Color syn_default;
    Color syn_keyword;
    Color syn_type;
    Color syn_preproc;
    Color syn_string;
    Color syn_number;
    Color syn_comment;
    Color status_bg;
    Color status_text;
    Color menu_bg;
    Color menu_border;
    Color menu_hl;
    bool is_light;
} Theme;

/**
 * Returns a pointer to the Theme definition matching the given index.
 * Falls back safely to index 0 if the provided index is out of bounds.
 */
const Theme *Theme_Get(int index);

/**
 * Returns the total number of registered themes.
 */
int Theme_GetCount(void);

/**
 * Returns the UI scale multiplier for a given preset index.
 * Returns 1.0f if the index is out of bounds.
 */
float Theme_GetUIScale(int index);

/**
 * Returns the total count of available UI scaling presets.
 */
int Theme_GetUIScaleCount(void);

#endif // CE_CORE_THEME_H