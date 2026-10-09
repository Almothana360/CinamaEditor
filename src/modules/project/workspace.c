#include "modules/project/workspace.h"
#include "modules/io/disk.h"
#include "buffer/syntax.h"
#include "modules/view/camera.h"
#include "core/event.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define CE_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define CE_MKDIR(p) mkdir(p, 0755)
#endif

// Temporary scan container for sorting
typedef struct {
    char full_path[CE_MAX_PATH];
    char name[CE_MAX_FILENAME];
    bool is_dir;
} ScanItem;

static void NormalizePath(char *path) {
    if (!path) return;
    for (char *p = path; *p; ++p) {
        if (*p == '\\') *p = '/';
    }
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') {
        path[len - 1] = '\0';
        len--;
    }
}

static bool IsIgnoredName(const char *name) {
    if (!name || name[0] == '\0') return true;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return true;
    if (name[0] == '.') return true; // Ignore hidden files/dirs (.git, .vscode, etc.)
    if (strcmp(name, "cmake-build-debug") == 0) return true;
    if (strcmp(name, "build") == 0) return true;
    if (strcmp(name, "bin") == 0) return true;
    if (strcmp(name, "obj") == 0) return true;
    if (strcmp(name, "node_modules") == 0) return true;
    if (strcmp(name, "__pycache__") == 0) return true;
    if (strcmp(name, ".idea") == 0) return true;
    return false;
}

static int ScanItem_Compare(const void *a, const void *b) {
    const ScanItem *ia = (const ScanItem *)a;
    const ScanItem *ib = (const ScanItem *)b;
    if (ia->is_dir != ib->is_dir) {
        return ia->is_dir ? -1 : 1; // Directories first
    }
    return strcmp(ia->name, ib->name);
}

static bool WasPathExpanded(const Workspace *ws, const char *path) {
    if (!ws || !ws->entries) return false;
    for (int i = 0; i < ws->entry_count; ++i) {
        if (ws->entries[i].is_directory && strcmp(ws->entries[i].path, path) == 0) {
            return ws->entries[i].is_expanded;
        }
    }
    return false;
}

static void Workspace_AddEntry(Workspace *ws, const char *path, const char *name, bool is_dir, int depth, bool was_expanded) {
    if (!ws) return;
    if (ws->entry_count >= ws->entry_capacity) {
        int new_cap = (ws->entry_capacity == 0) ? 64 : ws->entry_capacity * 2;
        WorkspaceEntry *ne = (WorkspaceEntry *)realloc(ws->entries, new_cap * sizeof(WorkspaceEntry));
        if (!ne) return;
        ws->entries = ne;
        ws->entry_capacity = new_cap;
    }

    WorkspaceEntry *e = &ws->entries[ws->entry_count++];
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->name[sizeof(e->name) - 1] = '\0';
    strncpy(e->path, path, sizeof(e->path) - 1);
    e->path[sizeof(e->path) - 1] = '\0';

    // Calculate path relative to root
    size_t root_len = strlen(ws->root_path);
    if (strncmp(path, ws->root_path, root_len) == 0) {
        const char *rel = path + root_len;
        if (*rel == '/') rel++;
        strncpy(e->rel_path, rel, sizeof(e->rel_path) - 1);
        e->rel_path[sizeof(e->rel_path) - 1] = '\0';
    } else {
        strncpy(e->rel_path, name, sizeof(e->rel_path) - 1);
        e->rel_path[sizeof(e->rel_path) - 1] = '\0';
    }

    e->is_directory = is_dir;
    e->depth = depth;
    e->is_expanded = was_expanded;
    e->is_hidden = false;
}

static void Workspace_ScanRecursive(Workspace *ws, const char *dir_path, int depth) {
    if (depth > 12 || !DirectoryExists(dir_path)) return;

    FilePathList list = LoadDirectoryFiles(dir_path);
    if (list.count == 0) {
        UnloadDirectoryFiles(list);
        return;
    }

    ScanItem *items = (ScanItem *)malloc(list.count * sizeof(ScanItem));
    int valid_count = 0;

    for (unsigned int i = 0; i < list.count; ++i) {
        char norm_path[CE_MAX_PATH];
        strncpy(norm_path, list.paths[i], sizeof(norm_path) - 1);
        norm_path[sizeof(norm_path) - 1] = '\0';
        NormalizePath(norm_path);

        const char *fname = GetFileName(norm_path);
        if (IsIgnoredName(fname)) continue;

        bool is_dir = DirectoryExists(norm_path);
        strncpy(items[valid_count].full_path, norm_path, sizeof(items[valid_count].full_path) - 1);
        items[valid_count].full_path[sizeof(items[valid_count].full_path) - 1] = '\0';
        strncpy(items[valid_count].name, fname, sizeof(items[valid_count].name) - 1);
        items[valid_count].name[sizeof(items[valid_count].name) - 1] = '\0';
        items[valid_count].is_dir = is_dir;
        valid_count++;
    }

    UnloadDirectoryFiles(list);

    if (valid_count > 1) {
        qsort(items, valid_count, sizeof(ScanItem), ScanItem_Compare);
    }

    for (int i = 0; i < valid_count; ++i) {
        bool was_exp = WasPathExpanded(ws, items[i].full_path);
        Workspace_AddEntry(ws, items[i].full_path, items[i].name, items[i].is_dir, depth, was_exp);

        if (items[i].is_dir) {
            Workspace_ScanRecursive(ws, items[i].full_path, depth + 1);
        }
    }

    free(items);
}

static void Workspace_UpdateVisibility(Workspace *ws) {
    if (!ws || ws->entry_count == 0) return;

    int collapsed_depth = -1;
    for (int i = 0; i < ws->entry_count; ++i) {
        WorkspaceEntry *e = &ws->entries[i];

        if (collapsed_depth != -1 && e->depth > collapsed_depth) {
            e->is_hidden = true;
        } else {
            collapsed_depth = -1;
            e->is_hidden = false;
            if (e->is_directory && !e->is_expanded) {
                collapsed_depth = e->depth;
            }
        }
    }
}

static void EnsureParentDirectoriesExist(const char *filepath) {
    if (!filepath) return;
    char temp[CE_MAX_PATH];
    strncpy(temp, filepath, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    NormalizePath(temp);

    char *p = temp;
    if (p[0] == '/') p++;
    if (p[0] && p[1] == ':') p += 2; // Skip Windows drive prefix C:

    while (*p) {
        if (*p == '/') {
            *p = '\0';
            if (!DirectoryExists(temp)) {
                CE_MKDIR(temp);
            }
            *p = '/';
        }
        p++;
    }
}

void Workspace_Init(Workspace *ws) {
    if (!ws) return;
    memset(ws, 0, sizeof(Workspace));
    ws->active_file_idx = -1;
    ws->is_valid = false;
}

void Workspace_Free(Workspace *ws) {
    if (!ws) return;
    if (ws->entries) {
        free(ws->entries);
        ws->entries = NULL;
    }
    ws->entry_count = 0;
    ws->entry_capacity = 0;
    ws->active_file_idx = -1;
    ws->is_valid = false;
    ws->root_path[0] = '\0';
    ws->name[0] = '\0';
}

bool Workspace_SetRoot(Workspace *ws, const char *dir_path) {
    if (!ws || !dir_path || dir_path[0] == '\0') return false;

    char norm_root[CE_MAX_PATH];
    strncpy(norm_root, dir_path, sizeof(norm_root) - 1);
    norm_root[sizeof(norm_root) - 1] = '\0';
    NormalizePath(norm_root);

    if (!DirectoryExists(norm_root)) {
        return false;
    }

    strncpy(ws->root_path, norm_root, sizeof(ws->root_path) - 1);
    ws->root_path[sizeof(ws->root_path) - 1] = '\0';

    const char *folder_name = GetFileName(ws->root_path);
    if (!folder_name || folder_name[0] == '\0') {
        strncpy(ws->name, "Workspace", sizeof(ws->name) - 1);
    } else {
        strncpy(ws->name, folder_name, sizeof(ws->name) - 1);
    }
    ws->name[sizeof(ws->name) - 1] = '\0';

    ws->is_valid = true;
    ws->active_file_idx = -1;

    Workspace_Refresh(ws);
    return true;
}

const char *Workspace_GetRoot(const Workspace *ws) {
    if (!ws || !ws->is_valid) return "";
    return ws->root_path;
}

const char *Workspace_GetName(const Workspace *ws) {
    if (!ws || !ws->is_valid) return "Untitled";
    return ws->name;
}

void Workspace_Refresh(Workspace *ws) {
    if (!ws || !ws->is_valid || ws->root_path[0] == '\0') return;

    // Preserve previously active file path across rescan
    char active_path[CE_MAX_PATH] = {0};
    if (ws->active_file_idx >= 0 && ws->active_file_idx < ws->entry_count) {
        strncpy(active_path, ws->entries[ws->active_file_idx].path, sizeof(active_path) - 1);
    }

    WorkspaceEntry *old_entries = ws->entries;
    int old_count = ws->entry_count;

    ws->entries = NULL;
    ws->entry_count = 0;
    ws->entry_capacity = 0;

    Workspace_ScanRecursive(ws, ws->root_path, 0);
    Workspace_UpdateVisibility(ws);

    if (old_entries) {
        free(old_entries);
    }

    if (active_path[0] != '\0') {
        ws->active_file_idx = Workspace_FindEntryByPath(ws, active_path);
    } else {
        ws->active_file_idx = -1;
    }

    WorkspaceChangedPayload wp = { ws->root_path, ws->entry_count };
    Event_Emit(EV_WORKSPACE_CHANGED, &wp);
}

void Workspace_ToggleFolder(Workspace *ws, int entry_idx) {
    if (!ws || entry_idx < 0 || entry_idx >= ws->entry_count) return;
    if (ws->entries[entry_idx].is_directory) {
        ws->entries[entry_idx].is_expanded = !ws->entries[entry_idx].is_expanded;
        Workspace_UpdateVisibility(ws);
    }
}

void Workspace_ExpandAll(Workspace *ws) {
    if (!ws) return;
    for (int i = 0; i < ws->entry_count; ++i) {
        if (ws->entries[i].is_directory) {
            ws->entries[i].is_expanded = true;
        }
    }
    Workspace_UpdateVisibility(ws);
}

void Workspace_CollapseAll(Workspace *ws) {
    if (!ws) return;
    for (int i = 0; i < ws->entry_count; ++i) {
        if (ws->entries[i].is_directory) {
            ws->entries[i].is_expanded = false;
        }
    }
    Workspace_UpdateVisibility(ws);
}

bool Workspace_OpenFile(Workspace *ws, Document *doc, const char *filepath) {
    if (!doc || !filepath || !FileExists(filepath)) return false;

    if (!Disk_LoadFile(doc, filepath)) return false;

    if (ws && ws->is_valid) {
        ws->active_file_idx = Workspace_FindEntryByPath(ws, filepath);
    }

    Syntax_SetLanguageByFilename(filepath);
    Syntax_UpdateMultilineComments(doc->lines, doc->line_count);

    Event_Emit(EV_FILE_MODIFIED, NULL);
    CursorMovedPayload cp = { doc->cursor_row, doc->cursor_col };
    Event_Emit(EV_CURSOR_MOVED, &cp);
    Camera_SnapToTarget();

    return true;
}

bool Workspace_OpenFileIndex(Workspace *ws, Document *doc, int entry_idx) {
    if (!ws || !doc || entry_idx < 0 || entry_idx >= ws->entry_count) return false;
    if (ws->entries[entry_idx].is_directory) return false;
    return Workspace_OpenFile(ws, doc, ws->entries[entry_idx].path);
}

bool Workspace_CreateFile(Workspace *ws, const char *rel_or_full_path) {
    if (!ws || !rel_or_full_path || rel_or_full_path[0] == '\0') return false;

    char full[CE_MAX_PATH];
    if (rel_or_full_path[0] == '/' || (rel_or_full_path[0] && rel_or_full_path[1] == ':')) {
        strncpy(full, rel_or_full_path, sizeof(full) - 1);
    } else {
        snprintf(full, sizeof(full), "%s/%s", ws->root_path, rel_or_full_path);
    }
    full[sizeof(full) - 1] = '\0';
    NormalizePath(full);

    if (FileExists(full)) return false; // File already exists

    EnsureParentDirectoriesExist(full);

    FILE *f = fopen(full, "wb");
    if (!f) return false;
    fclose(f);

    Workspace_Refresh(ws);
    return true;
}

bool Workspace_CreateFolder(Workspace *ws, const char *rel_or_full_path) {
    if (!ws || !rel_or_full_path || rel_or_full_path[0] == '\0') return false;

    char full[CE_MAX_PATH];
    if (rel_or_full_path[0] == '/' || (rel_or_full_path[0] && rel_or_full_path[1] == ':')) {
        strncpy(full, rel_or_full_path, sizeof(full) - 1);
    } else {
        snprintf(full, sizeof(full), "%s/%s", ws->root_path, rel_or_full_path);
    }
    full[sizeof(full) - 1] = '\0';
    NormalizePath(full);

    if (DirectoryExists(full)) return false;

    EnsureParentDirectoriesExist(full);
    int res = CE_MKDIR(full);
    if (res != 0) return false;

    Workspace_Refresh(ws);
    return true;
}

int Workspace_FindEntryByPath(const Workspace *ws, const char *path) {
    if (!ws || !path || !ws->entries) return -1;
    char norm[CE_MAX_PATH];
    strncpy(norm, path, sizeof(norm) - 1);
    norm[sizeof(norm) - 1] = '\0';
    NormalizePath(norm);

    for (int i = 0; i < ws->entry_count; ++i) {
        if (strcmp(ws->entries[i].path, norm) == 0) {
            return i;
        }
    }
    return -1;
}

const WorkspaceEntry *Workspace_GetEntries(const Workspace *ws, int *out_count) {
    if (!ws) {
        if (out_count) *out_count = 0;
        return NULL;
    }
    if (out_count) *out_count = ws->entry_count;
    return ws->entries;
}