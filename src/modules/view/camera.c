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

static void Camera_RecalculateTarget(void) {
    if (!g_doc) return;

    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    float line_height = CE_FONT_SIZE + 8.0f;
    float gutter_space = 45.0f;

    float cursor_x = Line_GetColX(g_font, &g_doc->lines[g_doc->cursor_row], g_doc->cursor_col, CE_FONT_SIZE, CE_FONT_SPACING);
    float cursor_y = (float)g_doc->cursor_row * line_height;

    if (g_cam_mode == CAM_MODE_BOUNDS_FIT) {
        float visible_h = (float)screen_h * 0.70f;
        float total_script_h = (float)g_doc->line_count * line_height;

        if (total_script_h <= visible_h) {
            float max_script_w = 0.0f;
            for (size_t i = 0; i < g_doc->line_count; ++i) {
                float lw = Line_GetColX(g_font, &g_doc->lines[i], g_doc->lines[i].size, CE_FONT_SIZE, CE_FONT_SPACING);
                if (lw > max_script_w) max_script_w = lw;
            }
            float script_box_w = fmaxf(max_script_w + gutter_space + 70.0f, 70.0f);
            float script_box_h = fmaxf(total_script_h, line_height);
            g_target_center = (Vector2){ (script_box_w - gutter_space) * 0.5f, script_box_h * 0.5f };
            if (g_doc->line_count == 1 && g_doc->lines[0].size == 0) {
                g_target_center = (Vector2){ 0.0f, line_height * 0.5f };
            }
            float zoom_fit_x = ((float)screen_w * 0.70f) / script_box_w;
            float zoom_fit_y = visible_h / script_box_h;
            float base_zoom = fminf(zoom_fit_x, zoom_fit_y);
            base_zoom = Clamp(base_zoom, 0.90f, 1.35f);
            g_final_target_zoom = Clamp(base_zoom * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
        } else {
            int context_lines = 24;
            int half_context = context_lines / 2;
            int start_r = (int)g_doc->cursor_row - half_context;
            int end_r = (int)g_doc->cursor_row + half_context;
            if (start_r < 0) start_r = 0;
            if (end_r >= (int)g_doc->line_count) end_r = (int)g_doc->line_count - 1;

            float max_script_w = 0.0f;
            for (int i = start_r; i <= end_r; ++i) {
                float lw = Line_GetColX(g_font, &g_doc->lines[i], g_doc->lines[i].size, CE_FONT_SIZE, CE_FONT_SPACING);
                if (lw > max_script_w) max_script_w = lw;
            }
            float script_box_w = fmaxf(max_script_w + gutter_space + 70.0f, 70.0f);
            g_target_center = (Vector2){ (script_box_w - gutter_space) * 0.5f, cursor_y + line_height * 0.5f };

            float zoom_fit_x = ((float)screen_w * 0.70f) / script_box_w;
            float base_zoom = Clamp(zoom_fit_x, 0.90f, 1.25f);
            g_final_target_zoom = Clamp(base_zoom * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
        }
    } else if (g_cam_mode == CAM_MODE_CURSOR_FOCUS) {
        g_target_center = (Vector2){ cursor_x + 20.0f, cursor_y + line_height * 0.5f };
        g_final_target_zoom = Clamp(1.30f * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
    } else if (g_cam_mode == CAM_MODE_LINE_FOCUS) {
        float curr_line_w = Line_GetColX(g_font, &g_doc->lines[g_doc->cursor_row], g_doc->lines[g_doc->cursor_row].size, CE_FONT_SIZE, CE_FONT_SPACING);
        g_target_center = (Vector2){ curr_line_w * 0.5f, cursor_y + line_height * 0.5f };
        float needed_w = fmaxf(curr_line_w + gutter_space + 140.0f, 320.0f);
        float line_zoom = ((float)screen_w * 0.80f) / needed_w;
        g_final_target_zoom = Clamp(line_zoom * g_user_zoom_mult, CE_MIN_CAMERA_ZOOM, CE_MAX_CAMERA_ZOOM);
    }
}

static void Camera_OnEvent(EventType type, const void *payload) {
    if (type == EV_ACTION) {
        const ActionPayload *p = payload;
        bool changed = false;
        switch(p->action) {
            case ACTION_CAM_BOUNDS_FIT: g_cam_mode = CAM_MODE_BOUNDS_FIT; changed = true; break;
            case ACTION_CAM_CURSOR_FOCUS: g_cam_mode = CAM_MODE_CURSOR_FOCUS; changed = true; break;
            case ACTION_CAM_LINE_FOCUS: g_cam_mode = CAM_MODE_LINE_FOCUS; changed = true; break;
            case ACTION_ZOOM_IN: g_user_zoom_mult = fminf(g_user_zoom_mult * 1.15f, 3.2f); changed = true; break;
            case ACTION_ZOOM_OUT: g_user_zoom_mult = fmaxf(g_user_zoom_mult / 1.15f, 0.35f); changed = true; break;
            case ACTION_ZOOM_RESET: g_user_zoom_mult = 1.0f; changed = true; break;
            case ACTION_SCROLL:
                if (p->ctrl_held) {
                    g_user_zoom_mult = Clamp(g_user_zoom_mult + p->float_data * 0.12f, 0.35f, 3.2f);
                    changed = true;
                }
                break;
            default: break;
        }
        if (changed) Camera_RecalculateTarget();
    } else if (type == EV_CURSOR_MOVED || type == EV_TEXT_CHANGED) {
        Camera_RecalculateTarget();
    }
}

void Camera_Init(const Document *doc, Font font_syntax) {
    g_doc = doc;
    g_font = font_syntax;

    float line_height = CE_FONT_SIZE + 8.0f;
    g_camera.rotation = 0.0f;
    g_camera.zoom = 1.30f;
    g_camera.target = (Vector2){ 0.0f, line_height * 0.5f };
    g_camera.offset = (Vector2){ (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f };
    g_target_center = g_camera.target;

    Event_Subscribe(EV_ACTION, Camera_OnEvent);
    Event_Subscribe(EV_CURSOR_MOVED, Camera_OnEvent);
    Event_Subscribe(EV_TEXT_CHANGED, Camera_OnEvent);

    Camera_RecalculateTarget();
}

void Camera_Update(float dt) {
    static int last_sw = 0, last_sh = 0;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (sw != last_sw || sh != last_sh) {
        Camera_RecalculateTarget();
        last_sw = sw;
        last_sh = sh;
    }

    g_camera.offset = (Vector2){ (float)sw * 0.5f, (float)sh * 0.5f };
    g_camera.zoom = Lerp(g_camera.zoom, g_final_target_zoom, 5.5f * dt);
    g_camera.target = Vector2Lerp(g_camera.target, g_target_center, 6.0f * dt);
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

CCameraMode Camera_GetMode(void) { return g_cam_mode; }
float Camera_GetUserZoomMult(void) { return g_user_zoom_mult; }