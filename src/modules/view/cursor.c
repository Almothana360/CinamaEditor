#include "modules/view/cursor.h"
#include "core/types.h"
#include "buffer/line.h"
#include "raymath.h"

void Cursor_Init(SmoothCursor *cursor, float line_height) {
    if (!cursor) return;
    cursor->current = (Vector2){ 0.0f, 4.0f };
    cursor->target = (Vector2){ 0.0f, 4.0f };
    cursor->trail = (Vector2){ 0.0f, 4.0f };
    cursor->width = 3.0f;
    cursor->height = line_height - 6.0f;
}

void Cursor_Update(SmoothCursor *cursor, float dt, const Document *doc, Font font_syntax, float line_height) {
    if (!cursor || !doc) return;

    float target_cur_x = Line_GetColX(font_syntax, &doc->lines[doc->cursor_row], doc->cursor_col, CE_FONT_SIZE, CE_FONT_SPACING);
    float target_cur_y = (float)doc->cursor_row * line_height;

    cursor->target = (Vector2){ target_cur_x, target_cur_y + 4.0f };
    cursor->trail = Vector2Lerp(cursor->trail, cursor->current, 10.0f * dt);
    cursor->current = Vector2Lerp(cursor->current, cursor->target, 22.0f * dt);
}