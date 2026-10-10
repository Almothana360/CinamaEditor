#include "render/renderer.h"
#include "render/canvas.h"
#include "buffer/syntax.h"
#include "buffer/line.h"
#include "ui/sidebar.h"
#include "ui/palette.h"
#include "ui/completion.h"
#include "ui/pause_menu.h"
#include "core/event.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Phase 9: Global states for tracking Datacore deployments and destructive glitches
static char s_last_file_path[512] = {0};
static float s_deploy_time = -999.0f;
static Vector2 s_last_cursor_world = {0,0};
static Color s_glitch_color = {0};

typedef struct {
    Vector2 pos;
    float width;
    float height;
    float timer;
    Color color;
    int dir;
} GlitchFX;

static GlitchFX s_glitches[32];
static int s_glitch_count = 0;
static bool s_renderer_initialized = false;

// Listens to global action requests right before they execute to spawn destruction glitches
static void Renderer_OnAction(EventType type, const void *payload) {
    if (type == EV_ACTION) {
        const ActionPayload *p = (const ActionPayload *)payload;

        if (p->action == ACTION_DELETE_BACKWARD || p->action == ACTION_DELETE_FORWARD ||
            p->action == ACTION_DELETE_WORD_BACKWARD || p->action == ACTION_DELETE_WORD_FORWARD ||
            p->action == ACTION_CUT) {

            if (s_glitch_count < 32) {
                GlitchFX *g = &s_glitches[s_glitch_count++];
                g->pos = s_last_cursor_world;
                g->width = (p->action == ACTION_CUT || p->action == ACTION_DELETE_WORD_BACKWARD || p->action == ACTION_DELETE_WORD_FORWARD) ? 120.0f : 24.0f;

                // Shift glitch slightly left for backwards deletions so it covers the destroyed area
                if (p->action == ACTION_CUT || p->action == ACTION_DELETE_WORD_BACKWARD || p->action == ACTION_DELETE_BACKWARD) {
                    g->pos.x -= g->width;
                }

                g->height = 28.0f;
                g->timer = 0.12f; // ~7 frames matching the heavy thud audio duration
                g->color = s_glitch_color;
                g->dir = (GetRandomValue(0, 1) == 0) ? -1 : 1;
            }
        }
    }
}

void Renderer_Draw(const RenderContext *ctx) {
    if (!ctx || !ctx->doc || !ctx->theme) return;

    if (!s_renderer_initialized) {
        Event_Subscribe(EV_ACTION, Renderer_OnAction);
        s_renderer_initialized = true;
    }

    if (ctx->cursor) {
        s_last_cursor_world = ctx->cursor->current;
        s_glitch_color = ctx->theme->cursor;
    }

    // Detect File Deployments to trigger Boot Sequence Decryption
    if (strcmp(s_last_file_path, ctx->doc->file_path) != 0) {
        strncpy(s_last_file_path, ctx->doc->file_path, sizeof(s_last_file_path) - 1);
        s_last_file_path[sizeof(s_last_file_path) - 1] = '\0';
        s_deploy_time = (float)GetTime();
    }

    // Calculate decrypt ratio (0.0 to 1.0 over 0.25 seconds)
    float decrypt_ratio = fminf(((float)GetTime() - s_deploy_time) / 0.25f, 1.0f);

    float line_height = CE_FONT_SIZE + 8.0f;

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

    // 1. Tactical Target Selection Rendering (Animated Hazard Stripes)
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

                float sel_x = x0;
                float sel_y = r * line_height + 4.0f;
                float sel_w = w;
                float sel_h = line_height - 4.0f;

                DrawRectangle((int)sel_x, (int)sel_y, (int)sel_w, (int)sel_h, ColorAlpha(ctx->theme->selection, 0.25f));

                EndMode2D();

                Vector2 tl = GetWorldToScreen2D((Vector2){sel_x, sel_y}, ctx->camera);
                Vector2 br = GetWorldToScreen2D((Vector2){sel_x + sel_w, sel_y + sel_h}, ctx->camera);

                int sc_x = (int)fminf(tl.x, br.x);
                int sc_y = (int)fminf(tl.y, br.y);
                int sc_w = (int)fabsf(br.x - tl.x);
                int sc_h = (int)fabsf(br.y - tl.y);

                if (sc_w > 0 && sc_h > 0) {
                    BeginScissorMode(sc_x, sc_y, sc_w, sc_h);
                    BeginMode2D(ctx->camera);

                    float stripe_space = 18.0f;
                    float anim_offset = fmodf((float)GetTime() * 40.0f, stripe_space);
                    for (float sx = sel_x - sel_h - stripe_space; sx < sel_x + sel_w + stripe_space; sx += stripe_space) {
                        float start_x = sx + anim_offset;
                        DrawLineEx(
                            (Vector2){ start_x, sel_y + sel_h },
                            (Vector2){ start_x + sel_h, sel_y },
                            2.0f, ColorAlpha(ctx->theme->selection, 0.35f)
                        );
                    }

                    EndMode2D();
                    EndScissorMode();
                }

                BeginMode2D(ctx->camera);

                DrawLineEx((Vector2){sel_x, sel_y}, (Vector2){sel_x, sel_y + sel_h}, 2.0f, ctx->theme->selection);
                DrawLineEx((Vector2){sel_x + sel_w, sel_y}, (Vector2){sel_x + sel_w, sel_y + sel_h}, 2.0f, ctx->theme->selection);
                DrawLineEx((Vector2){sel_x, sel_y}, (Vector2){sel_x + sel_w, sel_y}, 1.0f, ColorAlpha(ctx->theme->selection, 0.6f));
                DrawLineEx((Vector2){sel_x, sel_y + sel_h}, (Vector2){sel_x + sel_w, sel_y + sel_h}, 1.0f, ColorAlpha(ctx->theme->selection, 0.6f));
            }
        }
    }

    // 2. Culled Execution Stack (Avionics Gutter)
    if (has_visible_lines && ui_alpha > 0.01f) {
        float data_line_x = -16.0f;

        float top_y = (float)start_row * line_height;
        float bottom_y = (float)(end_row + 1) * line_height;
        DrawLine((int)data_line_x, (int)top_y, (int)data_line_x, (int)bottom_y, ColorAlpha(ctx->theme->gutter_num, 0.15f * ui_alpha));

        float cur_y_center = (float)ctx->doc->cursor_row * line_height + line_height * 0.5f;
        float glow_h = line_height * 8.0f;

        DrawRectangleGradientV((int)(data_line_x - 1.0f), (int)(cur_y_center - glow_h), 3, (int)glow_h, BLANK, ColorAlpha(ctx->theme->cursor, 0.7f * ui_alpha));
        DrawRectangleGradientV((int)(data_line_x - 1.0f), (int)cur_y_center, 3, (int)glow_h, ColorAlpha(ctx->theme->cursor, 0.7f * ui_alpha), BLANK);

        float pulse = 0.4f + 0.6f * sinf((float)GetTime() * 4.5f);

        for (int i = start_row; i <= end_row; ++i) {
            float row_y = (float)i * line_height;
            int dist = i - (int)ctx->doc->cursor_row;
            if (dist < 0) dist = -dist;

            float dist_falloff = expf(-(float)dist * 0.35f);
            if (dist_falloff < 0.12f) dist_falloff = 0.12f;

            if ((size_t)i == ctx->doc->cursor_row) {
                const char *chev = ">>";
                Vector2 chev_sz = MeasureTextEx(ctx->font_body, chev, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING);
                float chev_x = data_line_x - chev_sz.x - 6.0f;
                DrawTextEx(ctx->font_body, chev, (Vector2){ chev_x, row_y + 4.0f }, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING, ColorAlpha(ctx->theme->cursor, pulse * ui_alpha));

                char num_str[16];
                snprintf(num_str, sizeof(num_str), "%04d", i + 1);
                Vector2 num_sz = MeasureTextEx(ctx->font_body, num_str, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING);
                DrawTextEx(ctx->font_body, num_str, (Vector2){ chev_x - num_sz.x - 8.0f, row_y + 4.0f }, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING, ColorAlpha(ctx->theme->cursor, ui_alpha));

                DrawRectangle((int)(data_line_x - 1.0f), (int)row_y, 3, (int)line_height, ColorAlpha(ctx->theme->cursor, ui_alpha));

                if (!ctx->theme->is_light) {
                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawRectangle((int)(data_line_x - 3.0f), (int)row_y, 7, (int)line_height, ColorAlpha(ctx->theme->cursor, 0.4f * ui_alpha));
                    EndBlendMode();
                }
            } else {
                char num_str[16];
                snprintf(num_str, sizeof(num_str), "%04d", i + 1);
                Vector2 num_sz = MeasureTextEx(ctx->font_body, num_str, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING);
                DrawTextEx(ctx->font_body, num_str, (Vector2){ data_line_x - num_sz.x - 12.0f, row_y + 4.0f }, CE_FONT_SIZE - 2.0f, CE_FONT_SPACING, ColorAlpha(ctx->theme->gutter_num, dist_falloff * ui_alpha));
            }
        }
    }

    // 3. Culled Syntax Highlighting (With Decryption Micro-Animation injected)
    if (has_visible_lines) {
        for (int i = start_row; i <= end_row; ++i) {
            Syntax_DrawLine(ctx->font_syntax, &ctx->doc->lines[i], 0.0f, (float)i * line_height + 4.0f, ctx->theme, decrypt_ratio);
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

    // 5. World Visual Effects
    if (ctx->fx) {
        Fx_DrawWorld(ctx->fx);
    }

    // 6. Expanding Reactive Targeting Matrix (Cursor)
    if (ctx->cursor) {
        bool blink = ((int)(GetTime() * 2.4)) % 2 == 0;
        if (blink || (ctx->combo && ctx->combo->streak > 0) || ctx->is_mouse_dragging) {
            float dist = Vector2Distance(ctx->cursor->current, ctx->cursor->trail);

            if (dist > 1.0f) {
                float trail_alpha = fminf(dist * 0.05f, 0.5f);
                DrawRectangleV(ctx->cursor->trail, (Vector2){ ctx->cursor->width + dist * 0.4f, ctx->cursor->height }, ColorAlpha(ctx->theme->cursor_trail, trail_alpha));
            }

            float expansion = 0.0f;
            if (dist > 1.0f) {
                expansion = fminf(dist * 0.15f, 7.0f);
            } else if (ctx->combo && ctx->combo->streak > 0) {
                expansion = fminf((float)ctx->combo->streak * 0.4f, 5.0f);
            }

            float cur_x = ctx->cursor->current.x - expansion;
            float cur_y = ctx->cursor->current.y;
            float cur_w = ctx->cursor->width + expansion * 2.0f;
            float cur_h = ctx->cursor->height;

            float br_w = 2.0f;
            float br_len = 5.0f;
            Color c_col = ctx->theme->cursor;

            DrawRectangle((int)cur_x, (int)cur_y, (int)br_w, (int)cur_h, c_col);
            DrawRectangle((int)cur_x, (int)cur_y, (int)br_len, (int)br_w, c_col);
            DrawRectangle((int)cur_x, (int)(cur_y + cur_h - br_w), (int)br_len, (int)br_w, c_col);

            DrawRectangle((int)(cur_x + cur_w - br_w), (int)cur_y, (int)br_w, (int)cur_h, c_col);
            DrawRectangle((int)(cur_x + cur_w - br_len), (int)cur_y, (int)br_len, (int)br_w, c_col);
            DrawRectangle((int)(cur_x + cur_w - br_len), (int)(cur_y + cur_h - br_w), (int)br_len, (int)br_w, c_col);

            float pulse = 0.3f + 0.7f * fabsf(sinf((float)GetTime() * 12.0f));
            if (ctx->combo && ctx->combo->streak > 0) pulse = 1.0f;

            float core_sz = 4.0f;
            DrawRectangle((int)(cur_x + cur_w * 0.5f - core_sz * 0.5f),
                          (int)(cur_y + cur_h * 0.5f - core_sz * 0.5f),
                          (int)core_sz, (int)core_sz, ColorAlpha(c_col, pulse));

            if (!ctx->theme->is_light) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangle((int)(cur_x - 3.0f), (int)(cur_y - 3.0f), (int)(cur_w + 6.0f), (int)(cur_h + 6.0f), ColorAlpha(c_col, 0.25f * pulse));
                EndBlendMode();
            }
        }
    }

    // 7. Phase 9: Destruction Glitch Particles
    for (int i = 0; i < s_glitch_count; ++i) {
        GlitchFX *g = &s_glitches[i];
        if (g->timer > 0.0f) {
            g->timer -= GetFrameTime();
            int slices = 5;
            float slice_h = g->height / (float)slices;
            for (int s = 0; s < slices; ++s) {
                float offset = ((float)GetRandomValue(5, 35) * (float)g->dir) * (g->timer / 0.12f);
                Rectangle rect = { g->pos.x + offset, g->pos.y - 4.0f + (float)s * slice_h, g->width, slice_h };
                DrawRectangleRec(rect, g->color);

                if (!ctx->theme->is_light) {
                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawRectangleRec((Rectangle){rect.x - 2.0f, rect.y, rect.width + 4.0f, rect.height}, ColorAlpha(g->color, 0.6f));
                    EndBlendMode();
                }
            }
        }
    }

    // Cleanup expired glitches
    for (int i = s_glitch_count - 1; i >= 0; --i) {
        if (s_glitches[i].timer <= 0.0f) {
            s_glitches[i] = s_glitches[s_glitch_count - 1];
            s_glitch_count--;
        }
    }

    EndMode2D();

    // --- Screen-Space UI Layer ---

    UI_DrawMinimap(ctx->doc, ctx->camera, ctx->screen_w, ctx->screen_h, line_height, ctx->ui_scale, ctx->theme, ctx->font_body, ui_alpha);

    UI_DrawStatusBar(ctx->screen_h, ctx->ui_scale, ctx->cam_mode, ctx->camera.zoom, ctx->zoom_mult, ctx->theme, ctx->font_body, ui_alpha);

    if (ctx->combo) {
        UI_DrawComboHUD(ctx->combo, ctx->screen_w, ctx->ui_scale, ctx->theme, ctx->font_body);
    }

    if (ctx->fx) {
        Fx_DrawSpotlight(ctx->fx, GetMousePosition(), ctx->screen_w, ctx->screen_h, ctx->theme);
        if (ctx->fx->enable_crt) {
            Fx_DrawCRT(ctx->screen_w, ctx->screen_h);
        }
    }

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