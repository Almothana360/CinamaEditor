#include "core/theme.h"

static const float UI_SCALES[CE_UI_SCALE_COUNT] = {
    0.50f, 1.00f, 1.20f, 1.50f, 2.00f, 2.50f, 3.00f, 4.00f
};

static const Theme THEMES[CE_THEME_COUNT] = {
    // 0: Cyber Dark (Default)
    {
        .name = "Cyber Dark",
        .bg = (Color){ 14, 15, 20, 255 },
        .gutter_bg = (Color){ 11, 12, 16, 255 },
        .gutter_num = (Color){ 75, 80, 95, 255 },
        .gutter_num_curr = (Color){ 255, 215, 65, 255 },
        .cursor = (Color){ 255, 210, 50, 255 },
        .cursor_trail = (Color){ 255, 170, 30, 90 },
        .selection = (Color){ 60, 110, 190, 110 },
        .line_hl = (Color){ 24, 26, 35, 255 },
        .bracket_match = (Color){ 80, 220, 255, 210 },
        .syn_default = (Color){ 230, 235, 245, 255 },
        .syn_keyword = (Color){ 255, 85, 130, 255 },
        .syn_type = (Color){ 255, 170, 60, 255 },
        .syn_preproc = (Color){ 190, 120, 255, 255 },
        .syn_string = (Color){ 130, 235, 120, 255 },
        .syn_number = (Color){ 90, 200, 255, 255 },
        .syn_comment = (Color){ 105, 115, 130, 255 },
        .status_bg = (Color){ 10, 11, 15, 255 },
        .status_text = (Color){ 140, 145, 160, 255 },
        .menu_bg = (Color){ 20, 22, 30, 248 },
        .menu_border = (Color){ 255, 210, 50, 160 },
        .menu_hl = (Color){ 42, 46, 62, 255 },
        .is_light = false
    },
    // 1: Synthwave Neon
    {
        .name = "Synthwave Neon",
        .bg = (Color){ 24, 16, 42, 255 },
        .gutter_bg = (Color){ 18, 12, 32, 255 },
        .gutter_num = (Color){ 120, 95, 155, 255 },
        .gutter_num_curr = (Color){ 255, 235, 80, 255 },
        .cursor = (Color){ 255, 50, 160, 255 },
        .cursor_trail = (Color){ 180, 40, 240, 100 },
        .selection = (Color){ 170, 50, 180, 120 },
        .line_hl = (Color){ 36, 25, 62, 255 },
        .bracket_match = (Color){ 255, 230, 60, 220 },
        .syn_default = (Color){ 240, 230, 255, 255 },
        .syn_keyword = (Color){ 255, 45, 120, 255 },
        .syn_type = (Color){ 0, 240, 255, 255 },
        .syn_preproc = (Color){ 255, 175, 40, 255 },
        .syn_string = (Color){ 75, 255, 180, 255 },
        .syn_number = (Color){ 255, 210, 70, 255 },
        .syn_comment = (Color){ 135, 110, 165, 255 },
        .status_bg = (Color){ 16, 10, 28, 255 },
        .status_text = (Color){ 185, 160, 215, 255 },
        .menu_bg = (Color){ 32, 22, 56, 248 },
        .menu_border = (Color){ 255, 50, 160, 170 },
        .menu_hl = (Color){ 58, 38, 98, 255 },
        .is_light = false
    },
    // 2: Solarized Light
    {
        .name = "Solarized Light",
        .bg = (Color){ 253, 246, 227, 255 },
        .gutter_bg = (Color){ 238, 232, 213, 255 },
        .gutter_num = (Color){ 147, 161, 161, 255 },
        .gutter_num_curr = (Color){ 181, 137, 0, 255 },
        .cursor = (Color){ 38, 139, 210, 255 },
        .cursor_trail = (Color){ 42, 161, 152, 90 },
        .selection = (Color){ 42, 161, 152, 85 },
        .line_hl = (Color){ 242, 235, 214, 255 },
        .bracket_match = (Color){ 211, 54, 130, 200 },
        .syn_default = (Color){ 101, 123, 131, 255 },
        .syn_keyword = (Color){ 133, 153, 0, 255 },
        .syn_type = (Color){ 181, 137, 0, 255 },
        .syn_preproc = (Color){ 203, 75, 22, 255 },
        .syn_string = (Color){ 42, 161, 152, 255 },
        .syn_number = (Color){ 211, 54, 130, 255 },
        .syn_comment = (Color){ 147, 161, 161, 255 },
        .status_bg = (Color){ 238, 232, 213, 255 },
        .status_text = (Color){ 101, 123, 131, 255 },
        .menu_bg = (Color){ 248, 241, 220, 250 },
        .menu_border = (Color){ 38, 139, 210, 180 },
        .menu_hl = (Color){ 228, 220, 198, 255 },
        .is_light = true
    },
    // 3: Paper Clean
    {
        .name = "Paper Clean",
        .bg = (Color){ 250, 251, 253, 255 },
        .gutter_bg = (Color){ 240, 242, 247, 255 },
        .gutter_num = (Color){ 150, 155, 170, 255 },
        .gutter_num_curr = (Color){ 20, 30, 50, 255 },
        .cursor = (Color){ 15, 25, 45, 255 },
        .cursor_trail = (Color){ 70, 90, 130, 80 },
        .selection = (Color){ 75, 140, 245, 75 },
        .line_hl = (Color){ 240, 243, 249, 255 },
        .bracket_match = (Color){ 230, 60, 80, 210 },
        .syn_default = (Color){ 35, 42, 55, 255 },
        .syn_keyword = (Color){ 215, 30, 85, 255 },
        .syn_type = (Color){ 15, 105, 210, 255 },
        .syn_preproc = (Color){ 135, 40, 195, 255 },
        .syn_string = (Color){ 25, 145, 75, 255 },
        .syn_number = (Color){ 225, 110, 15, 255 },
        .syn_comment = (Color){ 140, 148, 162, 255 },
        .status_bg = (Color){ 235, 238, 245, 255 },
        .status_text = (Color){ 75, 85, 105, 255 },
        .menu_bg = (Color){ 255, 255, 255, 252 },
        .menu_border = (Color){ 50, 110, 220, 170 },
        .menu_hl = (Color){ 235, 240, 252, 255 },
        .is_light = true
    }
};

const Theme *Theme_Get(int index) {
    if (index < 0 || index >= CE_THEME_COUNT) {
        return &THEMES[0];
    }
    return &THEMES[index];
}

int Theme_GetCount(void) {
    return CE_THEME_COUNT;
}

float Theme_GetUIScale(int index) {
    if (index < 0 || index >= CE_UI_SCALE_COUNT) {
        return 1.0f;
    }
    return UI_SCALES[index];
}

int Theme_GetUIScaleCount(void) {
    return CE_UI_SCALE_COUNT;
}