#include "modules/view/camera.h"
#include "core/event.h"
#include "buffer/line.h"
#include "raymath.h"
#include <math.h>

static Camera2D g_camera = {0};
static CCameraMode g_cam_mode = CAM_MODE_BOUNDS_FIT;
static float g_user_zoom_mult = 1.0f;
static Vector2 g_target_center = {0};
static float g_final_target_zoom = 1.0f;

static const Document *g_doc = NULL;
static Font g_font = {0};
static Vector2 g_shake_offset = {0};
static bool g_initialized = false;

static void Camera_RecalculateTarget(void) {
    float line_height = CE_FONT_SIZE + 8.0f;
    float gutter_space = 45.0f;

    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    if (screen_w <= 0) screen_w = 1280;
    if (screen_h <= 0) screen_h = 720;

    // Safety fallback for empty or uninitialized documents
    if (!g_doc || g_doc->line_count == 0 || !g_doc->lines) {
        g_target_center = (Vector2){ 0.0f, line_height * 0.5f };
        g_final_target_zoom = Clamp(1.0f * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
        return;
    }

    // Safely clamp active cursor coordinates
    size_t cur_row = g_doc->cursor_row;
    if (cur_row >= g_doc->line_count) {
        cur_row = g_doc->line_count - 1;
    }
    size_t cur_col = g_doc->cursor_col;
    if (cur_col > g_doc->lines[cur_row].size) {
        cur_col = g_doc->lines[cur_row].size;
    }

    float cursor_x = Line_GetColX(g_font, &g_doc->lines[cur_row], cur_col, CE_FONT_SIZE, CE_FONT_SPACING);
    float cursor_y = (float)cur_row * line_height;
    float total_doc_h = (float)g_doc->line_count * line_height;

    if (g_cam_mode == CAM_MODE_BOUNDS_FIT) {
        float visible_h = (float)screen_h * 0.72f;

        if (total_doc_h <= visible_h) {
            // --- Short Document Framing ---
            // Center the entire document within view
            float max_line_w = 0.0f;
            for (size_t i = 0; i < g_doc->line_count; ++i) {
                float lw = Line_GetColX(g_font, &g_doc->lines[i], g_doc->lines[i].size, CE_FONT_SIZE, CE_FONT_SPACING);
                if (lw > max_line_w) max_line_w = lw;
            }

            float left_bound = -gutter_space - 20.0f;
            float right_bound = max_line_w + 40.0f;
            float box_w = fmaxf(right_bound - left_bound, 240.0f);
            float box_h = fmaxf(total_doc_h, line_height);

            float target_center_x = left_bound + box_w * 0.5f;
            float target_center_y = box_h * 0.5f;

            float fit_x = ((float)screen_w * 0.70f) / box_w;
            float fit_y = visible_h / box_h;
            float base_zoom = fminf(fit_x, fit_y);
            base_zoom = Clamp(base_zoom, 0.95f, 1.30f);

            g_target_center = (Vector2){ target_center_x, target_center_y };
            g_final_target_zoom = Clamp(base_zoom * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
        } else {
            // --- Long Document Virtualized Framing ---
            // Maintain stable reading scale and avoid whole-file line scans
            float base_zoom = 1.05f;
            g_final_target_zoom = Clamp(base_zoom * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);

            float eff_zoom = g_camera.zoom > 0.1f ? g_camera.zoom : g_final_target_zoom;
            float half_view_h = ((float)screen_h * 0.5f) / eff_zoom;
            float half_view_w = ((float)screen_w * 0.5f) / eff_zoom;

            // Anchor vertical motion so line 1 starts with a comfortable top margin
            float target_y = cursor_y + line_height * 0.5f;
            float top_padding_world = 55.0f / eff_zoom;
            float min_target_y = half_view_h - top_padding_world;
            float max_target_y = total_doc_h - half_view_h + top_padding_world;

            if (min_target_y <= max_target_y) {
                target_y = Clamp(target_y, min_target_y, max_target_y);
            }

            // Fixed reading margin: line numbers sit ~70px from screen left
            float left_margin_world = 70.0f / eff_zoom;
            float default_target_x = -gutter_space - left_margin_world + half_view_w;
            float target_x = default_target_x;

            // Scan only local lines in the visible window to determine horizontal bounds
            int win_start = (int)((target_y - half_view_h) / line_height) - 1;
            int win_end = (int)((target_y + half_view_h) / line_height) + 1;
            if (win_start < 0) win_start = 0;
            if (win_end >= (int)g_doc->line_count) win_end = (int)g_doc->line_count - 1;

            float right_edge_limit = default_target_x + half_view_w - (190.0f / eff_zoom);
            if (cursor_x > right_edge_limit) {
                target_x = default_target_x + (cursor_x - right_edge_limit);
            }

            g_target_center = (Vector2){ target_x, target_y };
        }
    } else if (g_cam_mode == CAM_MODE_CURSOR_FOCUS) {
        // --- Cursor Focus Mode ---
        float base_zoom = 1.32f;
        g_final_target_zoom = Clamp(base_zoom * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
        g_target_center = (Vector2){ cursor_x + 24.0f, cursor_y + line_height * 0.5f };
    } else if (g_cam_mode == CAM_MODE_LINE_FOCUS) {
        // --- Line Focus Mode ---
        float curr_line_w = Line_GetColX(g_font, &g_doc->lines[cur_row], g_doc->lines[cur_row].size, CE_FONT_SIZE, CE_FONT_SPACING);
        float needed_w = fmaxf(curr_line_w + gutter_space + 180.0f, 480.0f);
        float line_zoom = ((float)screen_w * 0.78f) / needed_w;
        float base_zoom = Clamp(line_zoom, 1.00f, 1.30f);

        g_final_target_zoom = Clamp(base_zoom * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);

        float target_x = (curr_line_w > 0.0f) ? (curr_line_w * 0.5f) : cursor_x;
        float target_y = cursor_y + line_height * 0.5f;
        g_target_center = (Vector2){ target_x, target_y };
    }
}

static void Camera_OnEvent(EventType type, const void *payload) {
    if (type == EV_ACTION) {
        const ActionPayload *p = (const ActionPayload *)payload;
        switch (p->action) {
            case ACTION_CAM_BOUNDS_FIT:   g_cam_mode = CAM_MODE_BOUNDS_FIT; break;
            case ACTION_CAM_CURSOR_FOCUS: g_cam_mode = CAM_MODE_CURSOR_FOCUS; break;
            case ACTION_CAM_LINE_FOCUS:   g_cam_mode = CAM_MODE_LINE_FOCUS; break;
            case ACTION_ZOOM_IN:          g_user_zoom_mult = fminf(g_user_zoom_mult * 1.15f, 3.2f); break;
            case ACTION_ZOOM_OUT:         g_user_zoom_mult = fmaxf(g_user_zoom_mult / 1.15f, 0.35f); break;
            case ACTION_ZOOM_RESET:       g_user_zoom_mult = 1.0f; break;
            case ACTION_SCROLL:
                if (p->ctrl_held) {
                    g_user_zoom_mult = Clamp(g_user_zoom_mult + p->float_data * 0.12f, 0.35f, 3.2f);
                }
                break;
            default: break;
        }
    }
}

void Camera_Init(const Document *doc, Font font_syntax) {
    if (g_initialized) {
        Camera_Close();
    }

    g_doc = doc;
    g_font = font_syntax;
    g_cam_mode = CAM_MODE_BOUNDS_FIT;
    g_user_zoom_mult = 1.0f;
    g_shake_offset = (Vector2){ 0, 0 };

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0) sw = 1280;
    if (sh <= 0) sh = 720;

    float line_height = CE_FONT_SIZE + 8.0f;
    g_camera.rotation = 0.0f;
    g_camera.zoom = 1.15f;
    g_camera.offset = (Vector2){ (float)sw * 0.5f, (float)sh * 0.5f };
    g_camera.target = (Vector2){ 0.0f, line_height * 0.5f };

    Event_Subscribe(EV_ACTION, Camera_OnEvent);
    Event_Subscribe(EV_CURSOR_MOVED, Camera_OnEvent);
    Event_Subscribe(EV_TEXT_CHANGED, Camera_OnEvent);

    Camera_RecalculateTarget();
    Camera_SnapToTarget();

    g_initialized = true;
}

void Camera_Close(void) {
    if (!g_initialized) return;

    Event_Unsubscribe(EV_ACTION, Camera_OnEvent);
    Event_Unsubscribe(EV_CURSOR_MOVED, Camera_OnEvent);
    Event_Unsubscribe(EV_TEXT_CHANGED, Camera_OnEvent);

    g_doc = NULL;
    g_initialized = false;
}

void Camera_Update(float dt) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0) sw = 1280;
    if (sh <= 0) sh = 720;

    g_camera.offset = (Vector2){ (float)sw * 0.5f, (float)sh * 0.5f };

    Camera_RecalculateTarget();

    // Frame-rate independent exponential smoothing
    float dt_clamped = fminf(dt, 0.05f);
    float zoom_t = 1.0f - expf(-7.0f * dt_clamped);
    float target_t = 1.0f - expf(-8.5f * dt_clamped);

    g_camera.zoom = Lerp(g_camera.zoom, g_final_target_zoom, zoom_t);
    g_camera.target = Vector2Lerp(g_camera.target, g_target_center, target_t);
}

Camera2D Camera_GetState(void) {
    Camera2D cam = g_camera;
    cam.offset.x += g_shake_offset.x;
    cam.offset.y += g_shake_offset.y;
    return cam;
}

void Camera_SetShakeOffset(Vector2 offset) {
    g_shake_offset = offset;
}

void Camera_SnapToTarget(void) {
    g_camera.target = g_target_center;
    g_camera.zoom = g_final_target_zoom;
}

CCameraMode Camera_GetMode(void) { return g_cam_mode; }
void Camera_SetMode(CCameraMode mode) { g_cam_mode = mode; }

float Camera_GetUserZoomMult(void) { return g_user_zoom_mult; }
void Camera_SetUserZoomMult(float mult) { g_user_zoom_mult = Clamp(mult, 0.35f, 3.2f); }
void Camera_ResetZoom(void) { g_user_zoom_mult = 1.0f; }