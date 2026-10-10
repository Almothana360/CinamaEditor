#include "render/renderer.h"
#include "render/canvas.h"
#include "buffer/syntax.h"
#include "buffer/line.h"
#include "ui/sidebar.h"
#include "ui/palette.h"
#include "ui/completion.h"
#include "ui/pause_menu.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

void Renderer_Draw(const RenderContext *ctx) {
    if (!ctx || !ctx->doc || !ctx->theme) return;

    float line_height = CE_FONT_SIZE + 8.0f;
    float gutter_space = 45.0f;

    // Viewport Culling Bounds Calculation
    float zoom = ctx->camera.zoom > 0.001f ? ctx->camera.zoom : 1.0f;
    float view_top_y = ctx->camera.target.y - (ctx->camera.offset.y / zoom);
    float view_bottom_y = ctx->camera.target.y + (((float)ctx->screen_h - ctx->camera.offset.y) / zoom);

    int start_row = (int)floorf(view_top_y / line_height) - 2;
    int end_row = (int)ceilf(view_bottom_y / line_height) + 2;

    if (start_row < 0) start_row = 0;
    if (end_row >= (int)ctx->doc->line_count) {
        end_row = (int)ctx->doc->line_count - 1;
    }

    bool has_visible_lines = (ctx->doc->line_count > 0 && ctx->doc->lines != NULL && start_row <= end_row);

    BracketMatch bracket = { .found = false, .row = 0, .col = 0 };
    if (has_visible_lines && ctx->doc->cursor_row < ctx->doc->line_count) {
        if ((int)ctx->doc->cursor_row >= start_row - 2 && (int)ctx->doc->cursor_row <= end_row + 2) {
            bracket = Syntax_FindMatchingBracket(ctx->doc->lines, ctx->doc->line_count, ctx->doc->cursor_row, ctx->doc->cursor_col);
        }
    }

    // Overdrive Opacity Multiplier
    float ui_alpha = ctx->combo ? ctx->combo->overdrive_alpha : 1.0f;

    BeginDrawing();
    ClearBackground(ctx->theme->bg);

    // --- In-World 2D Scene Pass ---
    BeginMode2D(ctx->camera);

    // 1. Culled Text Selection Rendering
    if (ctx->doc->has_selection && has_visible_lines) {
        size_t sr, sc, er, ec;
        Document_GetSelectionBounds(ctx->doc, &sr, &sc, &er, &ec);

        size_t sel_start = (sr < (size_t)start_row) ? (size_t)start_row : sr;
        size_t sel_end = (er > (size_t)end_row) ? (size_t)end_row : er;

        if (sr <= (size_t)end_row && er >= (size_t)start_row && sel_start <= sel_end) {
            for (size_t r = sel_start; r <= sel_end; ++r) {
                Line *l = &ctx->doc->lines[r];
                size_t c_start = (r == sr) ? sc : 0;
                size_t c_end = (r == er) ? ec : l->size;

                float x0 = Line_GetColX(ctx->font_syntax, l, c_start, CE_FONT_SIZE, CE_FONT_SPACING);
                float x1 = Line_GetColX(ctx->font_syntax, l, c_end, CE_FONT_SIZE, CE_FONT_SPACING);
                float w = x1 - x0;
                if (w < 8.0f && r < er) w = 12.0f;

                DrawRectangle((int)x0, (int)(r * line_height + 4.0f), (int)w, (int)(line_height - 4.0f), ctx->theme->selection);
            }
        }
    }

    // 2. Culled Gutter Line Numbers (Fades to nothing during Overdrive)
    if (has_visible_lines && ui_alpha > 0.01f) {
        for (int i = start_row; i <= end_row; ++i) {
            char num_str[16];
            snprintf(num_str, sizeof(num_str), "%d", i + 1);
            Color nc = ((size_t)i == ctx->doc->cursor_row) ? ctx->theme->gutter_num_curr : ctx->theme->gutter_num;
            DrawTextEx(ctx->font_body, num_str, (Vector2){ -gutter_space, (float)i * line_height + 4.0f }, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING, ColorAlpha(nc, ui_alpha));
        }
    }

    // 3. Culled Syntax Highlighting & Token Drawing
    if (has_visible_lines) {
        for (int i = start_row; i <= end_row; ++i) {
            Syntax_DrawLine(ctx->font_syntax, &ctx->doc->lines[i], 0.0f, (float)i * line_height + 4.0f, ctx->theme);
        }
    }

    // 4. Bracket Match Highlight
    if (bracket.found && has_visible_lines) {
        if ((int)bracket.row >= start_row && (int)bracket.row <= end_row) {
            float bx0 = Line_GetColX(ctx->font_syntax, &ctx->doc->lines[bracket.row], bracket.col, CE_FONT_SIZE, CE_FONT_SPACING);
            float bx1 = Line_GetColX(ctx->font_syntax, &ctx->doc->lines[bracket.row], bracket.col + 1, CE_FONT_SIZE, CE_FONT_SPACING);
            Rectangle b_rect = { bx0, (float)bracket.row * line_height + 4.0f, bx1 - bx0, line_height - 6.0f };

            if (!ctx->theme->is_light) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangleRounded(b_rect, 0.4f, 4, ctx->theme->bracket_match);
                EndBlendMode();
            }
            DrawRectangleRoundedLines(b_rect, 0.4f, 4, ctx->theme->bracket_match);
        }
    }

    // 5. World Visual Effects (Always renders, especially during Overdrive)
    if (ctx->fx) {
        Fx_DrawWorld(ctx->fx);
    }

    // 6. Smooth Animated Cursor & Trail
    if (ctx->cursor) {
        bool blink = ((int)(GetTime() * 2.4)) % 2 == 0;
        if (blink || (ctx->combo && ctx->combo->streak > 0) || ctx->is_mouse_dragging) {
            float dist = Vector2Distance(ctx->cursor->current, ctx->cursor->trail);
            if (dist > 1.0f) {
                DrawRectangleV(ctx->cursor->trail, (Vector2){ ctx->cursor->width + dist * 0.4f, ctx->cursor->height }, ctx->theme->cursor_trail);
            }
            DrawRectangleV(ctx->cursor->current, (Vector2){ ctx->cursor->width, ctx->cursor->height }, ctx->theme->cursor);
        }
    }

    EndMode2D();

    // --- Screen-Space Anchored Canvas UI Pass ---

    // Minimap and Status Bar linked to Overdrive Alpha Dissolve
    UI_DrawMinimap(ctx->doc, ctx->camera, ctx->screen_w, ctx->screen_h, line_height, ctx->ui_scale, ctx->theme, ui_alpha);
    UI_DrawStatusBar(ctx->screen_h, ctx->ui_scale, ctx->cam_mode, ctx->camera.zoom, ctx->zoom_mult, ctx->theme, ctx->font_body, ui_alpha);

    // Combo HUD inherently fades itself based on the streak decay, so it ignores the dissolve alpha
    if (ctx->combo) {
        UI_DrawComboHUD(ctx->combo, ctx->screen_w, ctx->ui_scale, ctx->theme, ctx->font_body);
    }

    if (ctx->fx) {
        Fx_DrawSpotlight(ctx->fx, GetMousePosition(), ctx->screen_w, ctx->screen_h, ctx->theme);
        if (ctx->fx->enable_crt) {
            Fx_DrawCRT(ctx->screen_w, ctx->screen_h);
        }
    }

    // Modals & Overlays
    Sidebar_Draw(ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->theme, ctx->font_body);

    if (ctx->menu) {
        UI_DrawContextMenu(ctx->menu, ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->cam_mode, ctx->theme, ctx->font_body);
    }

    if (ctx->show_help) {
        UI_DrawHelp(ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->theme, ctx->font_body);
    }

    Completion_Draw(ctx->camera, ctx->doc, ctx->font_syntax, ctx->font_body, ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->theme);

    Palette_Draw(ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->theme, ctx->font_body);

    PauseMenu_Draw(ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->theme, ctx->font_body);

    EndDrawing();
}