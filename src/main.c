#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/types.h"
#include "core/theme.h"
#include "core/event.h"

#include "modules/buffer/document.h"
#include "modules/io/disk.h"
#include "modules/input/input.h"
#include "modules/view/camera.h"
#include "modules/view/cursor.h"
#include "buffer/syntax.h"

#include "fx/audio.h"
#include "fx/fx.h"
#include "ui/ui.h"
#include "render/renderer.h"

typedef struct {
    Document doc;
    SmoothCursor cursor;
    AudioSystem audio;
    FxSystem fx;
    ContextMenu menu;
    ComboSystem combo;
    int theme_idx;
    int ui_scale_idx;
    Font font_body;
    Font font_syntax;
    bool show_help;
    bool is_mouse_dragging;
} AppEngine;

static AppEngine *g_app_instance = NULL;

static void App_OnAction(EventType type, const void *payload) {
    if (type != EV_ACTION || !payload) return;
    const ActionPayload *p = (const ActionPayload *)payload;
    AppEngine *app = g_app_instance;
    if (!app) return;

    switch (p->action) {
        case ACTION_TOGGLE_HELP: app->show_help = !app->show_help; break;
        case ACTION_CYCLE_THEME: {
            app->theme_idx = (app->theme_idx + 1) % Theme_GetCount();
            ThemeChangedPayload tp = { app->theme_idx, Theme_Get(app->theme_idx) };
            Event_Emit(EV_THEME_CHANGED, &tp);
            break;
        }
        case ACTION_CYCLE_UI_SCALE: app->ui_scale_idx = (app->ui_scale_idx + 1) % Theme_GetUIScaleCount(); break;
        default: break;
    }
}

static void App_Init(AppEngine *app, const char *initial_file) {
    memset(app, 0, sizeof(AppEngine));
    g_app_instance = app;

    EventBus_Init();

    app->theme_idx = 0;
    app->ui_scale_idx = 1;

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

    float line_height = CE_FONT_SIZE + 8.0f;

    // Connect Modules to Event Bus
    Document_Init(&app->doc);
    Disk_Init(&app->doc);
    Cursor_Init(&app->cursor, line_height);
    Camera_Init(&app->doc, app->font_syntax);
    Combo_Init(&app->combo);
    Audio_Init(&app->audio, &app->combo);
    Fx_Init(&app->fx, &app->doc, &app->combo, app->font_syntax);
    Syntax_Init(&app->doc, app->font_syntax);
    ContextMenu_Init(&app->menu);

    Event_Subscribe(EV_ACTION, App_OnAction);

    // Initial theme push
    ThemeChangedPayload tp = { app->theme_idx, Theme_Get(app->theme_idx) };
    Event_Emit(EV_THEME_CHANGED, &tp);

    if (initial_file) {
        Disk_LoadFile(&app->doc, initial_file);
    } else {
        const char *sample =
            "// Welcome to Cinema Editor (CE)!\n"
            "// Auto-tabs inherit leading line whitespace upon Enter.\n"
            "// Right-click: Context Menu with Camera Modes & UI Scaling.\n"
            "\n"
            "#include <stdio.h>\n"
            "\n"
            "int main(int argc, char **argv) {\n"
            "\tprintf(\"Cinema code editor!\\n\");\n"
            "\treturn 0;\n"
            "}\n";

        Document_InitEmpty(&app->doc);
        const char *p = sample;
        const char *line_start = p;
        while (*p) {
            if (*p == '\n') {
                Line line;
                Line_Init(&line);
                Line_AppendStr(&line, line_start, (size_t)(p - line_start));
                Document_AddLine(&app->doc, line);
                line_start = p + 1;
            }
            p++;
        }
        if (p > line_start) {
            Line line;
            Line_Init(&line);
            Line_AppendStr(&line, line_start, (size_t)(p - line_start));
            Document_AddLine(&app->doc, line);
        }
        app->doc.cursor_row = 6;
        app->doc.cursor_col = 1;
        app->doc.modified = false;
        Syntax_UpdateMultilineComments(app->doc.lines, app->doc.line_count);
    }
}

static void App_Close(AppEngine *app) {
    Fx_Close(&app->fx);
    Audio_Close(&app->audio);
    Document_Free(&app->doc);
    EventBus_Free();
}

static void App_HandleMenuAction(ContextAction action) {
    ActionPayload p = {0};
    switch (action) {
        case CTX_COPY: p.action = ACTION_COPY; break;
        case CTX_CUT: p.action = ACTION_CUT; break;
        case CTX_PASTE: p.action = ACTION_PASTE; break;
        case CTX_SELECT_ALL: p.action = ACTION_SELECT_ALL; break;
        case CTX_CAM_BOUNDS: p.action = ACTION_CAM_BOUNDS_FIT; break;
        case CTX_CAM_CURSOR: p.action = ACTION_CAM_CURSOR_FOCUS; break;
        case CTX_CAM_LINE: p.action = ACTION_CAM_LINE_FOCUS; break;
        case CTX_ZOOM_IN: p.action = ACTION_ZOOM_IN; break;
        case CTX_ZOOM_OUT: p.action = ACTION_ZOOM_OUT; break;
        case CTX_ZOOM_RESET: p.action = ACTION_ZOOM_RESET; break;
        case CTX_UI_SCALE: p.action = ACTION_CYCLE_UI_SCALE; break;
        case CTX_THEME_CYCLE: p.action = ACTION_CYCLE_THEME; break;
        case CTX_SPOTLIGHT_TOGGLE: p.action = ACTION_TOGGLE_SPOTLIGHT; break;
        case CTX_CRT_TOGGLE: p.action = ACTION_TOGGLE_CRT; break;
        default: return;
    }
    Event_Emit(EV_ACTION, &p);
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

        Camera2D cam = Camera_GetState();
        Vector2 mouse_screen = GetMousePosition();
        Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, cam);

        Fx_Update(&app.fx, dt);
        Combo_Update(&app.combo, dt);

        Input_Update();

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            ContextMenu_Open(&app.menu, mouse_screen);
        }

        if (app.menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                ContextAction action = ContextMenu_GetHoveredAction(&app.menu);
                if ((int)action >= 0) {
                    App_HandleMenuAction(action);
                }
                ContextMenu_Close(&app.menu);
            }
        }

        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (!app.menu.active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)app.doc.line_count) r = (int)app.doc.line_count - 1;

                Line *l = &app.doc.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = Line_GetColX(app.font_syntax, l, c, CE_FONT_SIZE, CE_FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                if (shift) {
                    if (!app.doc.has_selection) {
                        app.doc.anchor_row = app.doc.cursor_row;
                        app.doc.anchor_col = app.doc.cursor_col;
                        app.doc.has_selection = true;
                    }
                } else {
                    app.doc.has_selection = false;
                    app.doc.anchor_row = r;
                    app.doc.anchor_col = best_c;
                }
                app.doc.cursor_row = r;
                app.doc.cursor_col = best_c;
                app.is_mouse_dragging = true;
            }

            if (app.is_mouse_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int r = (int)((mouse_world.y - 4.0f) / line_height);
                if (r < 0) r = 0;
                if (r >= (int)app.doc.line_count) r = (int)app.doc.line_count - 1;

                Line *l = &app.doc.lines[r];
                size_t best_c = 0;
                float min_d = 1e9f;
                for (size_t c = 0; c <= l->size; ++c) {
                    float cx = Line_GetColX(app.font_syntax, l, c, CE_FONT_SIZE, CE_FONT_SPACING);
                    float d = fabsf(cx - mouse_world.x);
                    if (d < min_d) { min_d = d; best_c = c; }
                }

                app.doc.cursor_row = r;
                app.doc.cursor_col = best_c;
                if (app.doc.cursor_row != app.doc.anchor_row || app.doc.cursor_col != app.doc.anchor_col) {
                    app.doc.has_selection = true;
                }
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                app.is_mouse_dragging = false;
                if (app.doc.cursor_row == app.doc.anchor_row && app.doc.cursor_col == app.doc.anchor_col) {
                    app.doc.has_selection = false;
                }
            }
        }

        Cursor_Update(&app.cursor, dt, &app.doc, app.font_syntax, line_height);
        Camera_Update(dt);

        float shake_intensity = app.fx.shake_trauma * app.fx.shake_trauma;
        float shake_mag = shake_intensity * (app.combo.streak > 25 ? 24.0f : 14.0f);
        Vector2 shake_offset = {
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag,
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag
        };
        Camera_SetShakeOffset(shake_offset);

        RenderContext ctx = {
            .doc = &app.doc,
            .cursor = &app.cursor,
            .camera = Camera_GetState(),
            .cam_mode = Camera_GetMode(),
            .zoom_mult = Camera_GetUserZoomMult(),
            .fx = &app.fx,
            .menu = &app.menu,
            .combo = &app.combo,
            .show_help = app.show_help,
            .theme = Theme_Get(app.theme_idx),
            .ui_scale = Theme_GetUIScale(app.ui_scale_idx),
            .font_body = app.font_body,
            .font_syntax = app.font_syntax,
            .screen_w = screen_w,
            .screen_h = screen_h,
            .is_mouse_dragging = app.is_mouse_dragging
        };

        Renderer_Draw(&ctx);
    }

    App_Close(&app);
    return 0;
}