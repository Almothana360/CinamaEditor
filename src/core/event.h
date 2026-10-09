#ifndef CE_CORE_EVENT_H
#define CE_CORE_EVENT_H

#include <stddef.h>
#include <stdbool.h>

// --- Semantic Actions (Target for Phase 2) ---
// These map raw input (like Ctrl+C) into distinct behaviors, decoupling input from logic.
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
    ACTION_SAVE,
    ACTION_LOAD,

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
// Modules listen to these channels to react to state changes without calling each other.
typedef enum {
    EV_INIT = 0,           // Application started
    EV_ACTION,             // User triggered a semantic action (handled by Editor/UI)
    EV_TEXT_CHANGED,       // Buffer was modified (handled by FX/Audio)
    EV_CURSOR_MOVED,       // Cursor position changed (handled by Camera/View)
    EV_FILE_MODIFIED,      // File save/load state changed (handled by UI)
    EV_COMBO_HIT,          // Combo streak increased (handled by UI/FX)
    EV_COUNT
} EventType;

// --- Event Payloads ---

// Payload for EV_ACTION
typedef struct {
    ActionType action;
    char char_data;        // Valid if action is ACTION_INSERT_CHAR
    bool shift_held;       // Modifiers for text selection
    bool ctrl_held;        // Modifiers for alternate routing
} ActionPayload;

// Payload for EV_CURSOR_MOVED
typedef struct {
    size_t row;
    size_t col;
} CursorMovedPayload;

// Payload for EV_TEXT_CHANGED
typedef struct {
    size_t line_count;
    bool is_deletion;
} TextChangedPayload;

// Payload for EV_COMBO_HIT
typedef struct {
    int new_streak;
    int trigger_key;
} ComboHitPayload;

// Global Event Callback Signature
typedef void (*EventCallback)(EventType type, const void *payload);

// --- Event Bus API ---
void EventBus_Init(void);
void EventBus_Free(void);

// Returns true on successful subscription
bool Event_Subscribe(EventType type, EventCallback callback);

// Returns true on successful unsubscription
bool Event_Unsubscribe(EventType type, EventCallback callback);

// Broadcasts an event to all subscribers of the specified type
void Event_Emit(EventType type, const void *payload);

#endif // CE_CORE_EVENT_H