#ifndef CE_MODULES_BUFFER_HISTORY_H
#define CE_MODULES_BUFFER_HISTORY_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include "core/types.h"
#include "core/event.h"
#include "raylib.h"

#define CE_MAX_HISTORY 256

// Atomic line-replacement transaction record
typedef struct {
    ActionType action;
    size_t start_row;

    char **old_lines;
    size_t old_line_count;

    char **new_lines;
    size_t new_line_count;

    size_t cursor_before_row;
    size_t cursor_before_col;
    bool had_selection_before;
    size_t anchor_before_row;
    size_t anchor_before_col;

    size_t cursor_after_row;
    size_t cursor_after_col;
    bool had_selection_after;
    size_t anchor_after_row;
    size_t anchor_after_col;

    double timestamp;
} EditRecord;

typedef struct {
    EditRecord undo_stack[CE_MAX_HISTORY];
    size_t undo_count;

    EditRecord redo_stack[CE_MAX_HISTORY];
    size_t redo_count;

    bool in_transaction;
    bool is_coalescing_active;
    bool can_coalesce;

    EditRecord current_record;
    size_t pre_doc_line_count;
} History;

static inline char *History_StrDup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    if (dst) {
        memcpy(dst, src, len + 1);
    }
    return dst;
}

static inline void History_FreeRecord(EditRecord *rec) {
    if (!rec) return;
    if (rec->old_lines) {
        for (size_t i = 0; i < rec->old_line_count; ++i) {
            if (rec->old_lines[i]) free(rec->old_lines[i]);
        }
        free(rec->old_lines);
        rec->old_lines = NULL;
    }
    rec->old_line_count = 0;

    if (rec->new_lines) {
        for (size_t i = 0; i < rec->new_line_count; ++i) {
            if (rec->new_lines[i]) free(rec->new_lines[i]);
        }
        free(rec->new_lines);
        rec->new_lines = NULL;
    }
    rec->new_line_count = 0;
}

static inline void History_Init(History *h) {
    if (!h) return;
    memset(h, 0, sizeof(History));
}

static inline void History_ClearRedo(History *h) {
    if (!h) return;
    for (size_t i = 0; i < h->redo_count; ++i) {
        History_FreeRecord(&h->redo_stack[i]);
    }
    h->redo_count = 0;
}

static inline void History_Clear(History *h) {
    if (!h) return;
    for (size_t i = 0; i < h->undo_count; ++i) {
        History_FreeRecord(&h->undo_stack[i]);
    }
    h->undo_count = 0;
    History_ClearRedo(h);
    if (h->in_transaction && !h->is_coalescing_active) {
        History_FreeRecord(&h->current_record);
    }
    h->in_transaction = false;
    h->is_coalescing_active = false;
    h->can_coalesce = false;
}

static inline void History_Free(History *h) {
    History_Clear(h);
}

static inline void History_BreakCoalesce(History *h) {
    if (!h) return;
    h->can_coalesce = false;
}

static inline void History_PushUndo(History *h, const EditRecord *rec) {
    if (!h || !rec) return;
    if (h->undo_count >= CE_MAX_HISTORY) {
        History_FreeRecord(&h->undo_stack[0]);
        memmove(&h->undo_stack[0], &h->undo_stack[1], (CE_MAX_HISTORY - 1) * sizeof(EditRecord));
        h->undo_count = CE_MAX_HISTORY - 1;
    }
    h->undo_stack[h->undo_count++] = *rec;
}

#endif // CE_MODULES_BUFFER_HISTORY_H