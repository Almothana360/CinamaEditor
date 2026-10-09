#include "modules/io/disk.h"
#include "core/event.h"
#include "buffer/syntax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Document *g_disk_doc = NULL;

static void Disk_OnAction(EventType type, const void *payload) {
    if (type != EV_ACTION || !payload || !g_disk_doc) return;
    const ActionPayload *p = (const ActionPayload *)payload;

    if (p->action == ACTION_SAVE) {
        Disk_SaveFile(g_disk_doc);
        Event_Emit(EV_FILE_MODIFIED, NULL);
    }
}

void Disk_Init(Document *doc) {
    g_disk_doc = doc;
    Event_Subscribe(EV_ACTION, Disk_OnAction);
}

bool Disk_LoadFile(Document *doc, const char *filepath) {
    if (!doc || !filepath) return false;
    FILE *f = fopen(filepath, "rb");
    if (!f) return false;

    Document_Free(doc);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) {
        fclose(f);
        Document_InitEmpty(doc);
        strncpy(doc->file_path, filepath, sizeof(doc->file_path) - 1);
        doc->file_path[sizeof(doc->file_path) - 1] = '\0';
        doc->modified = false;
        return true;
    }

    char *buf = (char *)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return false;
    }

    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = '\0';
    fclose(f);

    size_t start = 0;
    for (size_t i = 0; i <= rd; ++i) {
        if (buf[i] == '\n' || buf[i] == '\0') {
            size_t len = i - start;
            if (len > 0 && buf[start + len - 1] == '\r') {
                len--;
            }
            Line line;
            Line_Init(&line);
            Line_AppendStr(&line, &buf[start], len);
            Document_AddLine(doc, line);
            start = i + 1;
        }
    }
    free(buf);

    if (doc->line_count == 0) {
        Document_InitEmpty(doc);
    }

    strncpy(doc->file_path, filepath, sizeof(doc->file_path) - 1);
    doc->file_path[sizeof(doc->file_path) - 1] = '\0';
    doc->cursor_row = 0;
    doc->cursor_col = 0;
    doc->has_selection = false;
    doc->modified = false;
    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);
    return true;
}

bool Disk_SaveFile(Document *doc) {
    if (!doc) return false;
    if (strlen(doc->file_path) == 0) {
        strncpy(doc->file_path, "untitled.c", sizeof(doc->file_path) - 1);
        doc->file_path[sizeof(doc->file_path) - 1] = '\0';
    }

    FILE *f = fopen(doc->file_path, "wb");
    if (!f) return false;

    for (size_t i = 0; i < doc->line_count; ++i) {
        if (doc->lines[i].size > 0) {
            fwrite(doc->lines[i].chars, 1, doc->lines[i].size, f);
        }
        if (i + 1 < doc->line_count) {
            fputc('\n', f);
        }
    }
    fclose(f);
    doc->modified = false;
    return true;
}