#ifndef CE_CORE_EVENT_H
#define CE_CORE_EVENT_H

#include <stddef.h>
#include <stdbool.h>
#include "raylib.h"

// --- Semantic Actions ---
typedef enum {
    ACTION_NONE = 0,
    ACTION_INSERT_CHAR,
    ACTION_INSERT_NEWLINE,
    ACTION_DELETE_BACKWARD,
    ACTION_DELETE_FORWARD,
    ACTION_DELETE_WORD_BACKWARD,
    ACTION_DELETE_WORD_FORWARD,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_MOVE_UP,
    ACTION_MOVE_DOWN,
    ACTION_MOVE_HOME,
    ACTION_MOVE_END,
    ACTION_MOVE_WORD_LEFT,
    ACTION_MOVE_WORD_RIGHT,
    ACTION_COPY,
    ACTION_CUT,
    ACTION_PASTE,
    ACTION_SELECT_ALL,
    ACTION_DUPLICATE_LINE,
    ACTION_UNDO,
    ACTION_REDO,
    ACTION_SAVE,
    ACTION_LOAD,
    ACTION_CHANGE_DIR,
    ACTION_SCROLL,

    // UI & View Commands
    ACTION_TOGGLE_HELP,
    ACTION_TOGGLE_CRT,
    ACTION_TOGGLE_SPOTLIGHT,
    ACTION_CYCLE_THEME,
    ACTION_CYCLE_UI_SCALE,
    ACTION_CAM_BOUNDS_FIT,
    ACTION_CAM_CURSOR_FOCUS,
    ACTION_CAM_LINE_FOCUS,
    ACTION_ZOOM_IN,
    ACTION_ZOOM_OUT,
    ACTION_ZOOM_RESET,
    ACTION_COUNT
} ActionType;

// --- Event Types ---
typedef enum {
    EV_INIT = 0,
    EV_ACTION,
    EV_TEXT_CHANGED,
    EV_CURSOR_MOVED,
    EV_FILE_MODIFIED,
    EV_COMBO_HIT,
    EV_TOKEN_COMPLETED,
    EV_THEME_CHANGED,
    EV_WORKSPACE_CHANGED,
    EV_COUNT
} EventType;

// --- Event Payloads ---
typedef struct {
    ActionType action;
    char char_data;
    float float_data;
    bool shift_held;
    bool ctrl_held;
} ActionPayload;

typedef struct {
    size_t row;
    size_t col;
} CursorMovedPayload;

typedef struct {
    size_t line_count;
    bool is_deletion;
} TextChangedPayload;

typedef struct {
    int new_streak;
    ActionType trigger_action;
    char char_data;
} ComboHitPayload;

typedef struct {
    size_t row;
    size_t start_col;
    size_t end_col;
    Color color;
} TokenCompletedPayload;

typedef struct {
    int theme_idx;
    const void *theme;
} ThemeChangedPayload;

typedef struct {
    const char *root_path;
    int entry_count;
} WorkspaceChangedPayload;

// Global Event Callback Signature
typedef void (*EventCallback)(EventType type, const void *payload);

// --- Event Bus API ---
void EventBus_Init(void);
void EventBus_Free(void);

bool Event_Subscribe(EventType type, EventCallback callback);
bool Event_Unsubscribe(EventType type, EventCallback callback);
void Event_Emit(EventType type, const void *payload);

#endif // CE_CORE_EVENT_H