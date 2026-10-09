#ifndef CE_BUFFER_GRAMMAR_H
#define CE_BUFFER_GRAMMAR_H

#include <stdbool.h>
#include <stddef.h>

#define CE_MAX_GRAMMAR_DELIMS 8

// Dynamic Language Grammar Definition
typedef struct {
    char name[64];

    char **extensions;
    int extension_count;

    char single_comment[16];        // e.g. "//" or "#" or "--"
    char multi_comment_start[16];   // e.g. "/*" or "<!--"
    char multi_comment_end[16];     // e.g. "*/" or "-->"

    char string_delims[CE_MAX_GRAMMAR_DELIMS]; // e.g. "\"'`"
    int string_delim_count;

    char **keywords;
    int keyword_count;

    char **types;
    int type_count;
} LanguageDef;

// Grammar Registry Lifecycle
void Grammar_Init(void);
void Grammar_Free(void);

// Loads all JSON grammar definitions from the given directory (e.g. "syntax")
int Grammar_LoadDirectory(const char *dir_path);

// Loads a single JSON grammar definition from a file on disk
bool Grammar_LoadFile(const char *filepath);

// Loads a grammar definition directly from a JSON string in memory
LanguageDef *Grammar_LoadFromString(const char *json_str);

// Grammar Lookups
const LanguageDef *Grammar_GetByExtension(const char *ext);
const LanguageDef *Grammar_GetByFilename(const char *filepath);
const LanguageDef *Grammar_GetByName(const char *name);
const LanguageDef *Grammar_GetDefault(void);

// Fast O(log N) Token Classification
bool Grammar_IsKeyword(const LanguageDef *lang, const char *word);
bool Grammar_IsType(const LanguageDef *lang, const char *word);

#endif // CE_BUFFER_GRAMMAR_H