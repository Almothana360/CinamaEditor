#include "render/renderer.h"
#include "buffer/syntax.h"
#include "buffer/line.h"
#include "raymath.h"
#include <stdio.h>

void Renderer_Draw(const RenderContext *ctx) {
    float line_height = CE_FONT_SIZE + 8.0f;
    float gutter_space = 45.0f;
    BracketMatch bracket = Syntax_FindMatchingBracket(ctx->doc->lines, ctx->doc->line_count, ctx->doc->cursor_row, ctx->doc->cursor_col);

    BeginDrawing();
    ClearBackground(ctx->theme->bg);

    BeginMode2D(ctx->camera);

    if (ctx->doc->has_selection) {
        size_t sr, sc, er, ec;
        Document_GetSelectionBounds(ctx->doc, &sr, &sc, &er, &ec);
        for (size_t r = sr; r <= er; ++r) {
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

    for (size_t i = 0; i < ctx->doc->line_count; ++i) {
        char num_str[16];
        snprintf(num_str, sizeof(num_str), "%zu", i + 1);
        Color nc = (i == ctx->doc->cursor_row) ? ctx->theme->gutter_num_curr : ctx->theme->gutter_num;
        DrawTextEx(ctx->font_body, num_str, (Vector2){ -gutter_space, (float)i * line_height + 4.0f }, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING, nc);
    }

    for (size_t i = 0; i < ctx->doc->line_count; ++i) {
        Syntax_DrawLine(ctx->font_syntax, &ctx->doc->lines[i], 0.0f, (float)i * line_height + 4.0f, ctx->theme);
    }

    if (bracket.found) {
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

    Fx_DrawWorld(ctx->fx);

    bool blink = ((int)(GetTime() * 2.4)) % 2 == 0;
    if (blink || ctx->combo->streak > 0 || ctx->is_mouse_dragging) {
        float dist = Vector2Distance(ctx->cursor->current, ctx->cursor->trail);
        if (dist > 1.0f) {
            DrawRectangleV(ctx->cursor->trail, (Vector2){ ctx->cursor->width + dist * 0.4f, ctx->cursor->height }, ctx->theme->cursor_trail);
        }
        DrawRectangleV(ctx->cursor->current, (Vector2){ ctx->cursor->width, ctx->cursor->height }, ctx->theme->cursor);
    }

    EndMode2D();

    UI_DrawMinimap(ctx->doc, ctx->camera, ctx->screen_w, ctx->screen_h, line_height, ctx->ui_scale, ctx->theme);
    UI_DrawComboHUD(ctx->combo, ctx->screen_w, ctx->ui_scale, ctx->theme, ctx->font_body);
    Fx_DrawSpotlight(ctx->fx, GetMousePosition(), ctx->screen_w, ctx->screen_h, ctx->theme);
    if (ctx->fx->enable_crt) Fx_DrawCRT(ctx->screen_w, ctx->screen_h);
    UI_DrawContextMenu(ctx->menu, ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->cam_mode, ctx->theme, ctx->font_body);
    if (ctx->show_help) {
        UI_DrawHelp(ctx->screen_w, ctx->screen_h, ctx->ui_scale, ctx->theme, ctx->font_body);
    }
    UI_DrawStatusBar(ctx->screen_h, ctx->ui_scale, ctx->cam_mode, ctx->zoom_mult, Camera_GetUserZoomMult(), ctx->theme, ctx->font_body);

    EndDrawing();
}