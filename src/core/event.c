#include "core/event.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    EventCallback *callbacks;
    size_t count;
    size_t capacity;
} SubscriberList;

static SubscriberList g_subscribers[EV_COUNT];
static bool g_bus_initialized = false;

void EventBus_Init(void) {
    if (g_bus_initialized) return;
    for (int i = 0; i < EV_COUNT; ++i) {
        g_subscribers[i].callbacks = NULL;
        g_subscribers[i].count = 0;
        g_subscribers[i].capacity = 0;
    }
    g_bus_initialized = true;
}

void EventBus_Free(void) {
    if (!g_bus_initialized) return;
    for (int i = 0; i < EV_COUNT; ++i) {
        if (g_subscribers[i].callbacks) {
            free(g_subscribers[i].callbacks);
        }
        g_subscribers[i].callbacks = NULL;
        g_subscribers[i].count = 0;
        g_subscribers[i].capacity = 0;
    }
    g_bus_initialized = false;
}

bool Event_Subscribe(EventType type, EventCallback callback) {
    if (!g_bus_initialized || type < 0 || type >= EV_COUNT || !callback) return false;

    SubscriberList *list = &g_subscribers[type];

    // Prevent duplicate subscriptions
    for (size_t i = 0; i < list->count; ++i) {
        if (list->callbacks[i] == callback) return true;
    }

    if (list->count >= list->capacity) {
        size_t new_cap = (list->capacity == 0) ? 8 : list->capacity * 2;
        EventCallback *new_cb = (EventCallback *)realloc(list->callbacks, new_cap * sizeof(EventCallback));
        if (!new_cb) return false;
        list->callbacks = new_cb;
        list->capacity = new_cap;
    }

    list->callbacks[list->count++] = callback;
    return true;
}

bool Event_Unsubscribe(EventType type, EventCallback callback) {
    if (!g_bus_initialized || type < 0 || type >= EV_COUNT || !callback) return false;

    SubscriberList *list = &g_subscribers[type];
    for (size_t i = 0; i < list->count; ++i) {
        if (list->callbacks[i] == callback) {
            // Shift remaining elements down
            memmove(&list->callbacks[i],
                    &list->callbacks[i + 1],
                    (list->count - i - 1) * sizeof(EventCallback));
            list->count--;
            return true;
        }
    }
    return false;
}

void Event_Emit(EventType type, const void *payload) {
    if (!g_bus_initialized || type < 0 || type >= EV_COUNT) return;

    SubscriberList *list = &g_subscribers[type];
    if (list->count == 0) return;

    // Copy array to stack/heap to prevent segfaults if a callback
    // subscribes/unsubscribes during iteration.
    size_t active_count = list->count;
    EventCallback *temp_cbs = (EventCallback *)malloc(active_count * sizeof(EventCallback));
    if (!temp_cbs) return;

    memcpy(temp_cbs, list->callbacks, active_count * sizeof(EventCallback));

    for (size_t i = 0; i < active_count; ++i) {
        if (temp_cbs[i]) {
            temp_cbs[i](type, payload);
        }
    }

    free(temp_cbs);
}