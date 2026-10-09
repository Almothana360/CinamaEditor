#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/types.h"
#include "core/theme.h"
#include "core/event.h"

#include "modules/buffer/document.h"
#include "modules/project/workspace.h"
#include "modules/io/disk.h"
#include "modules/input/input.h"
#include "modules/view/camera.h"
#include "modules/view/cursor.h"
#include "buffer/syntax.h"

#include "fx/audio.h"
#include "fx/fx.h"
#include "ui/ui.h"
#include "ui/menu_bar.h"
#include "ui/sidebar.h"
#include "ui/palette.h"
#include "ui/completion.h"
#include "render/renderer.h"

typedef struct {
    Document doc;
    Workspace workspace;
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
        case ACTION_LOAD: {
            if (app->doc.file_path[0] != '\0' && FileExists(app->doc.file_path)) {
                Workspace_OpenFile(&app->workspace, &app->doc, app->doc.file_path);
            }
            break;
        }
        default: break;
    }
}

static void App_Init(AppEngine *app, const char *initial_arg) {
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
    Workspace_Init(&app->workspace);
    Disk_Init(&app->doc);
    Cursor_Init(&app->cursor, line_height);
    Camera_Init(&app->doc, app->font_syntax);
    Combo_Init(&app->combo);
    Audio_Init(&app->audio, &app->combo);
    Fx_Init(&app->fx, &app->doc, &app->combo, app->font_syntax);
    Syntax_Init(&app->doc, app->font_syntax);
    ContextMenu_Init(&app->menu);
    MenuBar_Init();
    Sidebar_Init(&app->workspace, &app->doc);
    Palette_Init(&app->workspace, &app->doc);
    Completion_Init(&app->doc, app->font_syntax);

    Event_Subscribe(EV_ACTION, App_OnAction);

    // Initial theme push
    ThemeChangedPayload tp = { app->theme_idx, Theme_Get(app->theme_idx) };
    Event_Emit(EV_THEME_CHANGED, &tp);

    // Command-Line Argument / Workspace Resolution
    bool file_loaded = false;
    if (initial_arg && initial_arg[0] != '\0') {
        if (DirectoryExists(initial_arg)) {
            Workspace_SetRoot(&app->workspace, initial_arg);

            int count = 0;
            const WorkspaceEntry *entries = Workspace_GetEntries(&app->workspace, &count);
            int first_file_idx = -1;
            for (int i = 0; i < count; ++i) {
                if (!entries[i].is_directory) {
                    first_file_idx = i;
                    break;
                }
            }

            if (first_file_idx >= 0) {
                file_loaded = Workspace_OpenFileIndex(&app->workspace, &app->doc, first_file_idx);
            } else {
                Document_InitEmpty(&app->doc);
                char default_path[CE_MAX_PATH];
                snprintf(default_path, sizeof(default_path), "%s/main.c", app->workspace.root_path);
                strncpy(app->doc.file_path, default_path, sizeof(app->doc.file_path) - 1);
                app->doc.modified = false;
                Syntax_SetLanguageByFilename(app->doc.file_path);
                file_loaded = true;
            }
        } else if (FileExists(initial_arg)) {
            const char *parent_dir = GetDirectoryPath(initial_arg);
            if (!parent_dir || parent_dir[0] == '\0' || strcmp(parent_dir, ".") == 0) {
                Workspace_SetRoot(&app->workspace, GetWorkingDirectory());
            } else {
                Workspace_SetRoot(&app->workspace, parent_dir);
            }
            file_loaded = Workspace_OpenFile(&app->workspace, &app->doc, initial_arg);
        } else {
            Workspace_SetRoot(&app->workspace, GetWorkingDirectory());
            Document_InitEmpty(&app->doc);
            strncpy(app->doc.file_path, initial_arg, sizeof(app->doc.file_path) - 1);
            app->doc.modified = false;
            Syntax_SetLanguageByFilename(app->doc.file_path);
            file_loaded = true;
        }
    }

    if (!file_loaded) {
        Workspace_SetRoot(&app->workspace, GetWorkingDirectory());

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

        Document_Free(&app->doc);
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

    Camera_SnapToTarget();
}

static void App_Close(AppEngine *app) {
    Completion_Free();
    Palette_Free();
    MenuBar_Close();
    Camera_Close();
    Fx_Close(&app->fx);
    Audio_Close(&app->audio);
    Workspace_Free(&app->workspace);
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
        float ui_scale = Theme_GetUIScale(app.ui_scale_idx);

        Vector2 mouse_screen = GetMousePosition();

        // 1. Process Command Palette modal input (takes highest modal priority)
        bool palette_was_open = Palette_IsOpen();
        bool palette_consumed = Palette_Update(mouse_screen, screen_w, screen_h, ui_scale);
        bool palette_active = Palette_IsOpen();

        // 2. Dismiss completion if modal palette or context menu is active
        if (palette_active || app.menu.active) {
            Completion_Close();
        }

        // 3. Process Code Completion input (intercepts Up, Down, Tab, Enter, Esc when suggestions are visible)
        bool comp_consumed = false;
        if (!palette_was_open && !palette_active) {
            comp_consumed = Completion_Update(&app.doc, ui_scale);
        }

        // 4. Process editor inputs via event pipeline
        if (!palette_was_open && !palette_active && !comp_consumed) {
            Input_Update();
        }

        // 5. Check and refresh code completion trigger based on active word under cursor
        if (!palette_active && !app.menu.active) {
            Completion_CheckTrigger(&app.doc, app.font_syntax);
        }

        // 6. Advance visual and physics modules
        Cursor_Update(&app.cursor, dt, &app.doc, app.font_syntax, line_height);
        Camera_Update(dt);
        Fx_Update(&app.fx, dt);
        Combo_Update(&app.combo, dt);

        // 7. Inject shake trauma into the camera
        float shake_intensity = app.fx.shake_trauma * app.fx.shake_trauma;
        float shake_mag = shake_intensity * (app.combo.streak > 25 ? 24.0f : 14.0f);
        Vector2 shake_offset = {
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag,
            ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag
        };
        Camera_SetShakeOffset(shake_offset);

        // 8. Compute camera state with sidebar horizontal viewport offset
        Camera2D cam = Camera_GetState();
        float sidebar_w = Sidebar_GetWidth(ui_scale);
        if (sidebar_w > 0.0f) {
            cam.offset.x = ((float)screen_w + sidebar_w) * 0.5f;
        } else {
            cam.offset.x = (float)screen_w * 0.5f;
        }

        Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, cam);

        // 9. Screen UI Input Handling (Menu bar and Sidebar intercept before text selection)
        bool menu_consumed = false;
        bool sidebar_consumed = false;

        if (!palette_active) {
            menu_consumed = MenuBar_Update(mouse_screen, screen_w, ui_scale);
            sidebar_consumed = Sidebar_Update(mouse_screen, screen_w, screen_h, ui_scale);
        }

        // 10. Context menu handling
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (!menu_consumed && !sidebar_consumed && !palette_active) {
                ContextMenu_Open(&app.menu, mouse_screen);
            }
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

        // 11. Mouse selection handling (isolated from menu bar, sidebar, palette, and completion popup)
        bool in_comp = Completion_ContainsPoint(mouse_screen, cam, &app.doc, app.font_syntax, screen_w, screen_h, ui_scale);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool in_text_area = (!app.menu.active && !menu_consumed && !sidebar_consumed && !palette_active && !in_comp &&
                             mouse_screen.y > MenuBar_GetHeight(ui_scale) &&
                             (!Sidebar_IsOpen() || mouse_screen.x > sidebar_w));

        if (in_text_area) {
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

        // 12. Multi-layer rendering pipeline
        RenderContext ctx = {
            .doc = &app.doc,
            .cursor = &app.cursor,
            .camera = cam,
            .cam_mode = Camera_GetMode(),
            .zoom_mult = Camera_GetUserZoomMult(),
            .fx = &app.fx,
            .menu = &app.menu,
            .combo = &app.combo,
            .show_help = app.show_help,
            .theme = Theme_Get(app.theme_idx),
            .ui_scale = ui_scale,
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