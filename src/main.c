#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/types.h"
#include "core/theme.h"
#include "buffer/line.h"
#include "buffer/syntax.h"
#include "editor/editor.h"
#include "fx/audio.h"
#include "fx/fx.h"
#include "ui/ui.h"

/**
 * Top-level application coordinator state.
 */
typedef struct {
    Editor editor;
    SmoothCursor cursor;
    Camera2D camera;
    AudioSystem audio;
    FxSystem fx;
    ContextMenu menu;
    ComboSystem combo;

    int theme_idx;
    int ui_scale_idx;
    CCameraMode cam_mode;

    Font font_body;
    Font font_syntax;

    float user_zoom_mult;
    bool show_help;
    bool is_mouse_dragging;
} AppEngine;

static void App_Init(AppEngine *app, const char *initial_file) {
    memset(app, 0, sizeof(AppEngine));
    app->theme_idx = 0;
    app->ui_scale_idx = 1; // 100% default scale
    app->cam_mode = CAM_MODE_BOUNDS_FIT;
    app->user_zoom_mult = 1.0f;

    Audio_Init(&app->audio);
    Fx_Init(&app->fx);
    ContextMenu_Init(&app->menu);
    Combo_Init(&app->combo);

    if (FileExists("fonts/VictorMono-Regular.ttf")) {
        app->font_body = LoadFontEx("fonts/VictorMono-Regular.ttf", (int)CE_FONT_SIZE, NULL, 0);
    } else {
        app->font_body = GetFontDefault();
    }

    if (FileExists("fonts/iosevka-regular.ttf")) {
        app->font_syntax = LoadFontEx("fonts/iosevka-regular.ttf", (int)CE_FONT_SIZE, NULL, 0);
    } else {
        app->font_syntax = app->font_body;
    }

    Editor_InitEmpty(&app->editor);

    float line_height = CE_FONT_SIZE + 8.0f;
    app->camera.rotation = 0.0f;
    app->camera.zoom = 1.30f;
    app->camera.target = (Vector2){ 0.0f, line_height * 0.5f };

    SmoothCursor_Init(&app->cursor, line_height);

    if (initial_file) {
        Editor_LoadFile(&app->editor, initial_file);
    } else {
        const char *sample =
            "// Welcome to Cinema Editor (CE)!\n"
            "// Auto-tabs inherit leading line whitespace upon Enter.\n"
            "// Right-click: Context Menu with Camera Modes & UI Scaling.\n"
            "\n"
            "#include <stdio.h>\n"
            "\n"
            "int main(int argc, char **argv) {\n"
            "    printf(\"Cinema code editor!\\n\");\n"
            "    return 0;\n"
            "}\n";
        while (*sample) {
            if (*sample == '\n') Editor_InsertNewline(&app->editor);
            else Editor_InsertChar(&app->editor, *sample);
            sample++;
        }
        app->editor.cursor_row = 6;
        app->editor.cursor_col = 4;
        app->editor.modified = false;
    }
}

static void App_Close(AppEngine *app) {
    Fx_Close(&app->fx);
    Audio_Close(&app->audio);
    Editor_Free(&app->editor);
}

static void App_HandleAction(AppEngine *app, ContextAction action) {
    switch (action) {
        case CTX_COPY: Editor_CopySelection(&app->editor); break;
        case CTX_CUT: Editor_CutSelection(&app->editor); break;
        case CTX_PASTE: Editor_PasteClipboard(&app->editor); break;
        case CTX_SELECT_ALL: Editor_SelectAll(&app->editor); break;
        case CTX_CAM_BOUNDS: app->cam_mode = CAM_MODE_BOUNDS_FIT; break;
        case CTX_CAM_CURSOR: app->cam_mode = CAM_MODE_CURSOR_FOCUS; break;
        case CTX_CAM_LINE: app->cam_mode = CAM_MODE_LINE_FOCUS; break;
        case CTX_ZOOM_IN: app->user_zoom_mult = fminf(app->user_zoom_mult * 1.15f, 3.2f); break;
        case CTX_ZOOM_OUT: app->user_zoom_mult = fmaxf(app->user_zoom_mult / 1.15f, 0.35f); break;
        case CTX_ZOOM_RESET: app->user_zoom_mult = 1.0f; break;
        case CTX_UI_SCALE: app->ui_scale_idx = (app->ui_scale_idx + 1) % Theme_GetUIScaleCount(); break;
        case CTX_THEME_CYCLE: app->theme_idx = (app->theme_idx + 1) % Theme_GetCount(); break;
        case CTX_SPOTLIGHT_TOGGLE: app->fx.enable_spotlight = !app->fx.enable_spotlight; break;
        case CTX_CRT_TOGGLE: app->fx.enable_crt = !app->fx.enable_crt; break;
        default: break;
    }
}

int main(int argc, char **argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, CE_APP_DEFAULT_TITLE);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    AppEngine app;
    App_Init(&app, (argc > 1) ? argv[1] : NULL);

    float line_height = CE_FONT_SIZE + 8.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        const Theme *theme = Theme_Get(app.theme_idx);
        float ui_scale = Theme_GetUIScale(app.ui_scale_idx);

        Vector2 mouse_screen = GetMousePosition();
        Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, app.camera);

        Fx_Update(&app.fx, dt);
        Combo_Update(&app.combo, dt);

        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // Function Shortcuts
        if (IsKeyPressed(KEY_F1)) app.show_help = !app.show_help;
        if (IsKeyPressed(KEY_F2)) app.fx.enable_crt = !app.fx.enable_crt;
        if (IsKeyPressed(KEY_F3)) app.fx.enable_spotlight = !app.fx.enable_spotlight;
        if (IsKeyPressed(KEY_F4)) app.theme_idx = (app.theme_idx + 1) % Theme_GetCount();
        if (IsKeyPressed(KEY_F5)) app.cam_mode = CAM_MODE_BOUNDS_FIT;
        if (IsKeyPressed(KEY_F6)) app.cam_mode = CAM_MODE_CURSOR_FOCUS;
        if (IsKeyPressed(KEY_F7)) app.cam_mode = CAM_MODE_LINE_FOCUS;
        if (IsKeyPressed(KEY_F8)) app.ui_scale_idx = (app.ui_scale_idx + 1) % Theme_GetUIScaleCount();

        // Right-Click Context Menu Activation
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            ContextMenu_Open(&app.menu, mouse_screen);
        }

        if (app.menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                ContextAction action = ContextMenu_GetHoveredAction(&app.menu);
                if ((int)action >= 0) {
                    App_HandleAction(&app, action);
                }
                ContextMenu_Close(&app.menu);
            }
        }

        // Mouse Drag Selection
        if (!app.menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)app.editor.line_count) r = (int)app.editor.line_count - 1;

                Line *l = &app.editor.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = Line_GetColX(app.font_syntax, l, c, CE_FONT_SIZE, CE_FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                if (shift) {
                    if (!app.editor.has_selection) {
                        app.editor.anchor_row = app.editor.cursor_row;
                        app.editor.anchor_col = app.editor.cursor_col;
                        app.editor.has_selection = true;
                    }
                } else {
                    app.editor.has_selection = false;
                    app.editor.anchor_row = r;
                    app.editor.anchor_col = best_c;
                }

                app.editor.cursor_row = r;
                app.editor.cursor_col = best_c;
                app.is_mouse_dragging = true;
            }

            if (app.is_mouse_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)app.editor.line_count) r = (int)app.editor.line_count - 1;

                Line *l = &app.editor.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = Line_GetColX(app.font_syntax, l, c, CE_FONT_SIZE, CE_FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                app.editor.cursor_row = r;
                app.editor.cursor_col = best_c;
                if (app.editor.cursor_row != app.editor.anchor_row || app.editor.cursor_col != app.editor.anchor_col) {
                    app.editor.has_selection = true;
                }
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                app.is_mouse_dragging = false;
                if (app.editor.cursor_row == app.editor.anchor_row && app.editor.cursor_col == app.editor.anchor_col) {
                    app.editor.has_selection = false;
                }
            }
        }

        // Ctrl + Mouse Wheel Zoom
        float wheel = GetMouseWheelMove();
        if (ctrl && wheel != 0.0f) {
            app.user_zoom_mult = Clamp(app.user_zoom_mult + wheel * 0.12f, 0.35f, 3.2f);
        }

        // Keyboard Command Router
        if (ctrl && IsKeyPressed(KEY_C)) {
            Editor_CopySelection(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_X)) {
            Editor_CutSelection(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_V)) {
            Editor_PasteClipboard(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_A)) {
            Editor_SelectAll(&app.editor);
        } else if (ctrl && IsKeyPressed(KEY_D)) {
            Editor_DuplicateLine(&app.editor);
        } else if (ctrl && (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))) {
            app.user_zoom_mult = fminf(app.user_zoom_mult * 1.15f, 3.2f);
        } else if (ctrl && (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))) {
            app.user_zoom_mult = fmaxf(app.user_zoom_mult / 1.15f, 0.35f);
        } else if (ctrl && (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0))) {
            app.user_zoom_mult = 1.0f;
        } else if (ctrl && IsKeyPressed(KEY_BACKSPACE)) {
            Editor_DeleteWordBackward(&app.editor);
            Audio_PlayKey(&app.audio, KEY_BACKSPACE, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.2f);
        } else if (ctrl && IsKeyPressed(KEY_DELETE)) {
            Editor_DeleteWordForward(&app.editor);
            Audio_PlayKey(&app.audio, KEY_BACKSPACE, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.2f);
        } else if (IsKeyPressed(KEY_ENTER)) {
            Editor_InsertNewline(&app.editor);
            Combo_RegisterHit(&app.combo);
            Audio_PlayKey(&app.audio, KEY_ENTER, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.30f);
        } else if (IsKeyPressed(KEY_BACKSPACE)) {
            Editor_Backspace(&app.editor);
            Audio_PlayKey(&app.audio, KEY_BACKSPACE, app.combo.streak);
            Fx_AddTrauma(&app.fx, 0.16f);
        } else if (IsKeyPressed(KEY_DELETE)) {
            Editor_Delete(&app.editor);
        } else if (IsKeyPressed(KEY_TAB)) {
            for (int k = 0; k < CE_TAB_SIZE; ++k) {
                Editor_InsertChar(&app.editor, ' ');
            }
            Combo_RegisterHit(&app.combo);
            Audio_PlayKey(&app.audio, KEY_SPACE, app.combo.streak);
        } else {
            int nav_key = 0;
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) nav_key = KEY_LEFT;
            else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) nav_key = KEY_RIGHT;
            else if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) nav_key = KEY_UP;
            else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) nav_key = KEY_DOWN;
            else if (IsKeyPressed(KEY_HOME)) nav_key = KEY_HOME;
            else if (IsKeyPressed(KEY_END)) nav_key = KEY_END;

            if (nav_key != 0) {
                if (shift && !app.editor.has_selection) {
                    app.editor.anchor_row = app.editor.cursor_row;
                    app.editor.anchor_col = app.editor.cursor_col;
                    app.editor.has_selection = true;
                } else if (!shift && app.editor.has_selection) {
                    Editor_ClearSelection(&app.editor);
                }

                if (ctrl && nav_key == KEY_LEFT) Editor_MoveWordLeft(&app.editor);
                else if (ctrl && nav_key == KEY_RIGHT) Editor_MoveWordRight(&app.editor);
                else if (ctrl && nav_key == KEY_HOME) { app.editor.cursor_row = 0; app.editor.cursor_col = 0; }
                else if (ctrl && nav_key == KEY_END) { app.editor.cursor_row = app.editor.line_count - 1; app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size; }
                else if (nav_key == KEY_LEFT) {
                    if (app.editor.cursor_col > 0) app.editor.cursor_col--;
                    else if (app.editor.cursor_row > 0) {
                        app.editor.cursor_row--;
                        app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_RIGHT) {
                    if (app.editor.cursor_col < app.editor.lines[app.editor.cursor_row].size) app.editor.cursor_col++;
                    else if (app.editor.cursor_row + 1 < app.editor.line_count) {
                        app.editor.cursor_row++;
                        app.editor.cursor_col = 0;
                    }
                } else if (nav_key == KEY_UP) {
                    if (app.editor.cursor_row > 0) {
                        app.editor.cursor_row--;
                        if (app.editor.cursor_col > app.editor.lines[app.editor.cursor_row].size)
                            app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_DOWN) {
                    if (app.editor.cursor_row + 1 < app.editor.line_count) {
                        app.editor.cursor_row++;
                        if (app.editor.cursor_col > app.editor.lines[app.editor.cursor_row].size)
                            app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                    }
                } else if (nav_key == KEY_HOME) {
                    app.editor.cursor_col = 0;
                } else if (nav_key == KEY_END) {
                    app.editor.cursor_col = app.editor.lines[app.editor.cursor_row].size;
                }

                if (shift && app.editor.cursor_row == app.editor.anchor_row && app.editor.cursor_col == app.editor.anchor_col) {
                    app.editor.has_selection = false;
                }
            } else {
                int ch = GetCharPressed();
                while (ch > 0) {
                    if (ch >= 32 && ch <= 126) {
                        Editor_InsertChar(&app.editor, (char)ch);
                        Combo_RegisterHit(&app.combo);

                        Audio_PlayKey(&app.audio, ch == ' ' ? KEY_SPACE : KEY_A, app.combo.streak);
                        Fx_AddTrauma(&app.fx, (app.combo.streak > 20) ? 0.35f : 0.22f);

                        float cur_x = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], app.editor.cursor_col, CE_FONT_SIZE, CE_FONT_SPACING);
                        Vector2 spark_pos = { cur_x, (float)app.editor.cursor_row * line_height + line_height * 0.5f };
                        Fx_EmitParticles(&app.fx, spark_pos, theme->cursor, app.combo.streak > 15 ? 12 : 7, app.combo.streak > 25);

                        char token[128];
                        size_t s_col, e_col;
                        if (Editor_GetCompletedToken(&app.editor, token, sizeof(token), &s_col, &e_col)) {
                            Color glow_col = Syntax_GetHighlightColor(token, theme);
                            if (glow_col.a > 0) {
                                float x0 = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], s_col, CE_FONT_SIZE, CE_FONT_SPACING);
                                float x1 = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], e_col, CE_FONT_SIZE, CE_FONT_SPACING);
                                Rectangle rect = { x0, (float)app.editor.cursor_row * line_height, x1 - x0, line_height };

                                Fx_TriggerGlow(&app.fx, rect, glow_col);
                                Fx_EmitParticles(&app.fx, (Vector2){ x1, (float)app.editor.cursor_row * line_height + line_height * 0.5f }, glow_col, 25, app.combo.streak > 15);
                                Audio_PlayGlow(&app.audio);
                            }
                        }
                    }
                    ch = GetCharPressed();
                }
            }
        }

        // Smooth Cursor Movement
        float target_cur_x = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], app.editor.cursor_col, CE_FONT_SIZE, CE_FONT_SPACING);
        float target_cur_y = (float)app.editor.cursor_row * line_height;
        SmoothCursor_Update(&app.cursor, (Vector2){ target_cur_x, target_cur_y + 4.0f }, dt);

        // Cinema Camera Dynamic Target & Zoom
        Vector2 target_center = { 0 };
        float final_target_zoom = 1.0f;
        float gutter_space = 45.0f;

        if (app.cam_mode == CAM_MODE_BOUNDS_FIT) {
            float max_script_w = 0.0f;
            for (size_t i = 0; i < app.editor.line_count; ++i) {
                float lw = Line_GetColX(app.font_syntax, &app.editor.lines[i], app.editor.lines[i].size, CE_FONT_SIZE, CE_FONT_SPACING);
                if (lw > max_script_w) max_script_w = lw;
            }
            float script_box_w = fmaxf(max_script_w + gutter_space + 70.0f, 70.0f);
            float script_box_h = fmaxf((float)app.editor.line_count * line_height, line_height);

            target_center = (Vector2){ (script_box_w - gutter_space) * 0.5f, script_box_h * 0.5f };
            if (app.editor.line_count == 1 && app.editor.lines[0].size == 0) {
                target_center = (Vector2){ 0.0f, line_height * 0.5f };
            }

            float zoom_fit_x = ((float)screen_w * 0.70f) / script_box_w;
            float zoom_fit_y = ((float)screen_h * 0.70f) / script_box_h;
            float base_zoom = fminf(zoom_fit_x, zoom_fit_y);
            final_target_zoom = Clamp(base_zoom * app.user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);

        } else if (app.cam_mode == CAM_MODE_CURSOR_FOCUS) {
            target_center = (Vector2){ app.cursor.target.x + 20.0f, app.cursor.target.y + line_height * 0.5f };
            final_target_zoom = Clamp(1.30f * app.user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);

        } else if (app.cam_mode == CAM_MODE_LINE_FOCUS) {
            float curr_line_w = Line_GetColX(app.font_syntax, &app.editor.lines[app.editor.cursor_row], app.editor.lines[app.editor.cursor_row].size, CE_FONT_SIZE, CE_FONT_SPACING);
            target_center = (Vector2){ curr_line_w * 0.5f, (float)app.editor.cursor_row * line_height + line_height * 0.5f };

            float needed_w = fmaxf(curr_line_w + gutter_space + 140.0f, 320.0f);
            float line_zoom = ((float)screen_w * 0.80f) / needed_w;
            final_target_zoom = Clamp(line_zoom * app.user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
        }

        app.camera.zoom = Lerp(app.camera.zoom, final_target_zoom, 5.5f * dt);
        app.camera.target = Vector2Lerp(app.camera.target, target_center, 6.0f * dt);

        float shake_intensity = app.fx.shake_trauma * app.fx.shake_trauma;
        float shake_mag = shake_intensity * (app.combo.streak > 25 ? 24.0f : 14.0f);
        Vector2 shake_offset = {
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag,
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag
        };
        app.camera.offset = (Vector2){
            (float)screen_w * 0.5f + shake_offset.x,
            (float)screen_h * 0.5f + shake_offset.y
        };

        BracketMatch bracket = Syntax_FindMatchingBracket(app.editor.lines, app.editor.line_count, app.editor.cursor_row, app.editor.cursor_col);

        // --- Render Passes ---
        BeginDrawing();
        ClearBackground(theme->bg);

        // In-World 2D Scene
        BeginMode2D(app.camera);

        if (app.editor.has_selection) {
            size_t sr, sc, er, ec;
            Editor_GetSelectionBounds(&app.editor, &sr, &sc, &er, &ec);
            for (size_t r = sr; r <= er; ++r) {
                Line *l = &app.editor.lines[r];
                size_t c_start = (r == sr) ? sc : 0;
                size_t c_end = (r == er) ? ec : l->size;

                float x0 = Line_GetColX(app.font_syntax, l, c_start, CE_FONT_SIZE, CE_FONT_SPACING);
                float x1 = Line_GetColX(app.font_syntax, l, c_end, CE_FONT_SIZE, CE_FONT_SPACING);
                float w = x1 - x0;
                if (w < 8.0f && r < er) w = 12.0f;

                DrawRectangle((int)x0, (int)(r * line_height + 4.0f), (int)w, (int)(line_height - 4.0f), theme->selection);
            }
        }

        for (size_t i = 0; i < app.editor.line_count; ++i) {
            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%zu", i + 1);
            Color nc = (i == app.editor.cursor_row) ? theme->gutter_num_curr : theme->gutter_num;
            DrawTextEx(app.font_body, num_str, (Vector2){ -gutter_space, (float)i * line_height + 4.0f }, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING, nc);
        }

        for (size_t i = 0; i < app.editor.line_count; ++i) {
            Syntax_DrawLine(app.font_syntax, &app.editor.lines[i], 0.0f, (float)i * line_height + 4.0f, theme);
        }

        if (bracket.found) {
            float bx0 = Line_GetColX(app.font_syntax, &app.editor.lines[bracket.row], bracket.col, CE_FONT_SIZE, CE_FONT_SPACING);
            float bx1 = Line_GetColX(app.font_syntax, &app.editor.lines[bracket.row], bracket.col + 1, CE_FONT_SIZE, CE_FONT_SPACING);
            Rectangle b_rect = { bx0, (float)bracket.row * line_height + 4.0f, bx1 - bx0, line_height - 6.0f };

            if (!theme->is_light) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangleRounded(b_rect, 0.4f, 4, theme->bracket_match);
                EndBlendMode();
            }
            DrawRectangleRoundedLines(b_rect, 0.4f, 4, theme->bracket_match);
        }

        Fx_DrawWorld(&app.fx);

        bool blink = ((int)(GetTime() * 2.4)) % 2 == 0;
        if (blink || app.combo.streak > 0 || app.is_mouse_dragging) {
            float dist = Vector2Distance(app.cursor.current, app.cursor.trail);
            if (dist > 1.0f) {
                DrawRectangleV(app.cursor.trail, (Vector2){ app.cursor.width + dist * 0.4f, app.cursor.height }, theme->cursor_trail);
            }
            DrawRectangleV(app.cursor.current, (Vector2){ app.cursor.width, app.cursor.height }, theme->cursor);
        }

        EndMode2D();

        // Screen-Space UI Elements
        UI_DrawMinimap(&app.editor, app.camera, screen_w, screen_h, line_height, ui_scale, theme);
        UI_DrawComboHUD(&app.combo, screen_w, ui_scale, theme, app.font_body);
        Fx_DrawSpotlight(&app.fx, mouse_screen, screen_w, screen_h, theme);
        if (app.fx.enable_crt) Fx_DrawCRT(screen_w, screen_h);
        UI_DrawContextMenu(&app.menu, screen_w, screen_h, ui_scale, app.cam_mode, theme, app.font_body);

        if (app.show_help) {
            UI_DrawHelp(screen_w, screen_h, ui_scale, theme, app.font_body);
        }

        UI_DrawStatusBar(screen_h, ui_scale, app.cam_mode, app.camera.zoom, app.user_zoom_mult, theme, app.font_body);

        EndDrawing();
    }

    App_Close(&app);
    CloseWindow();
    return 0;
}