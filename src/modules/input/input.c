#include "modules/input/input.h"
#include "core/event.h"
#include "raylib.h"

void Input_Update(void) {
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    ActionPayload p = {0};
    p.ctrl_held = ctrl;
    p.shift_held = shift;

    // F-keys
    if (IsKeyPressed(KEY_F1)) { p.action = ACTION_TOGGLE_HELP; Event_Emit(EV_ACTION, &p); }
    if (IsKeyPressed(KEY_F2)) { p.action = ACTION_TOGGLE_CRT; Event_Emit(EV_ACTION, &p); }
    if (IsKeyPressed(KEY_F3)) { p.action = ACTION_TOGGLE_SPOTLIGHT; Event_Emit(EV_ACTION, &p); }
    if (IsKeyPressed(KEY_F4)) { p.action = ACTION_CYCLE_THEME; Event_Emit(EV_ACTION, &p); }
    if (IsKeyPressed(KEY_F5)) { p.action = ACTION_CAM_BOUNDS_FIT; Event_Emit(EV_ACTION, &p); }
    if (IsKeyPressed(KEY_F6)) { p.action = ACTION_CAM_CURSOR_FOCUS; Event_Emit(EV_ACTION, &p); }
    if (IsKeyPressed(KEY_F7)) { p.action = ACTION_CAM_LINE_FOCUS; Event_Emit(EV_ACTION, &p); }
    if (IsKeyPressed(KEY_F8)) { p.action = ACTION_CYCLE_UI_SCALE; Event_Emit(EV_ACTION, &p); }

    // Ctrl Shortcuts
    if (ctrl) {
        if (IsKeyPressed(KEY_C)) { p.action = ACTION_COPY; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_X)) { p.action = ACTION_CUT; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_V)) { p.action = ACTION_PASTE; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_A)) { p.action = ACTION_SELECT_ALL; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_D)) { p.action = ACTION_DUPLICATE_LINE; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) { p.action = ACTION_ZOOM_IN; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) { p.action = ACTION_ZOOM_OUT; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0)) { p.action = ACTION_ZOOM_RESET; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_BACKSPACE)) { p.action = ACTION_DELETE_WORD_BACKWARD; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_DELETE)) { p.action = ACTION_DELETE_WORD_FORWARD; Event_Emit(EV_ACTION, &p); }
    } else {
        // Normal keys
        if (IsKeyPressed(KEY_ENTER)) { p.action = ACTION_INSERT_NEWLINE; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_BACKSPACE)) { p.action = ACTION_DELETE_BACKWARD; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_DELETE)) { p.action = ACTION_DELETE_FORWARD; Event_Emit(EV_ACTION, &p); }
        if (IsKeyPressed(KEY_TAB)) { p.action = ACTION_INSERT_CHAR; p.char_data = '\t'; Event_Emit(EV_ACTION, &p); }
    }

    // Navigation
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        p.action = ctrl ? ACTION_MOVE_WORD_LEFT : ACTION_MOVE_LEFT;
        Event_Emit(EV_ACTION, &p);
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        p.action = ctrl ? ACTION_MOVE_WORD_RIGHT : ACTION_MOVE_RIGHT;
        Event_Emit(EV_ACTION, &p);
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        p.action = ACTION_MOVE_UP;
        Event_Emit(EV_ACTION, &p);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        p.action = ACTION_MOVE_DOWN;
        Event_Emit(EV_ACTION, &p);
    }
    if (IsKeyPressed(KEY_HOME)) {
        p.action = ACTION_MOVE_HOME;
        Event_Emit(EV_ACTION, &p);
    }
    if (IsKeyPressed(KEY_END)) {
        p.action = ACTION_MOVE_END;
        Event_Emit(EV_ACTION, &p);
    }

    // Text Input
    if (!ctrl) {
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ch >= 32 && ch <= 126) {
                p.action = ACTION_INSERT_CHAR;
                p.char_data = (char)ch;
                Event_Emit(EV_ACTION, &p);
            }
            ch = GetCharPressed();
        }
    }

    // Mouse Wheel (Zoom & Scroll)
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        p.action = ACTION_SCROLL;
        p.float_data = wheel;
        p.char_data = 0;
        Event_Emit(EV_ACTION, &p);
    }
}