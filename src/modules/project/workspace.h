#ifndef CE_MODULES_PROJECT_WORKSPACE_H
#define CE_MODULES_PROJECT_WORKSPACE_H

#include "modules/buffer/document.h"
#include <stdbool.h>
#include <stddef.h>

#define CE_MAX_PATH 512
#define CE_MAX_FILENAME 128

// Single file or directory entry in the workspace tree
typedef struct {
    char name[CE_MAX_FILENAME];     // Filename or folder name (e.g. "main.c", "src")
    char path[CE_MAX_PATH];         // Full normalized canonical path
    char rel_path[CE_MAX_PATH];     // Path relative to workspace root (e.g. "src/main.c")
    bool is_directory;              // true if folder, false if file
    int depth;                      // Nesting depth level (0 = top level inside root)
    bool is_expanded;               // For directories: whether expanded in UI
    bool is_hidden;                 // true if an ancestor folder is collapsed
} WorkspaceEntry;

// Project workspace state
typedef struct {
    char root_path[CE_MAX_PATH];    // Active workspace directory root path
    char name[CE_MAX_FILENAME];     // Directory display name (e.g. "MyProject")

    WorkspaceEntry *entries;        // Dynamic array of all scanned project items
    int entry_count;
    int entry_capacity;

    int active_file_idx;            // Currently open file index in entries (-1 if none)
    bool is_valid;
} Workspace;

// Workspace Lifecycle
void Workspace_Init(Workspace *ws);
void Workspace_Free(Workspace *ws);

// Root Directory Management
bool Workspace_SetRoot(Workspace *ws, const char *dir_path);
const char *Workspace_GetRoot(const Workspace *ws);
const char *Workspace_GetName(const Workspace *ws);

// Traversal & Scanning
void Workspace_Refresh(Workspace *ws);

// Folder Expansion & Tree Navigation
void Workspace_ToggleFolder(Workspace *ws, int entry_idx);
void Workspace_ExpandAll(Workspace *ws);
void Workspace_CollapseAll(Workspace *ws);

// File Operations
bool Workspace_OpenFile(Workspace *ws, Document *doc, const char *filepath);
bool Workspace_OpenFileIndex(Workspace *ws, Document *doc, int entry_idx);
bool Workspace_CreateFile(Workspace *ws, const char *rel_or_full_path);
bool Workspace_CreateFolder(Workspace *ws, const char *rel_or_full_path);

// Query Helpers
int Workspace_FindEntryByPath(const Workspace *ws, const char *path);
const WorkspaceEntry *Workspace_GetEntries(const Workspace *ws, int *out_count);

#endif // CE_MODULES_PROJECT_WORKSPACE_H