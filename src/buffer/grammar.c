#include "buffer/grammar.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static LanguageDef **g_languages = NULL;
static int g_language_count = 0;
static int g_language_capacity = 0;
static bool g_grammar_initialized = false;

// Case-insensitive string search comparison for bsearch
static int Grammar_StringCompare(const void *a, const void *b) {
    const char *sa = *(const char * const *)a;
    const char *sb = *(const char * const *)b;
    return strcmp(sa, sb);
}

static void Grammar_SortKeywords(LanguageDef *lang) {
    if (!lang) return;
    if (lang->keywords && lang->keyword_count > 1) {
        qsort(lang->keywords, lang->keyword_count, sizeof(char *), Grammar_StringCompare);
    }
    if (lang->types && lang->type_count > 1) {
        qsort(lang->types, lang->type_count, sizeof(char *), Grammar_StringCompare);
    }
}

static void Grammar_FreeDef(LanguageDef *lang) {
    if (!lang) return;
    if (lang->extensions) {
        for (int i = 0; i < lang->extension_count; ++i) {
            if (lang->extensions[i]) free(lang->extensions[i]);
        }
        free(lang->extensions);
    }
    if (lang->keywords) {
        for (int i = 0; i < lang->keyword_count; ++i) {
            if (lang->keywords[i]) free(lang->keywords[i]);
        }
        free(lang->keywords);
    }
    if (lang->types) {
        for (int i = 0; i < lang->type_count; ++i) {
            if (lang->types[i]) free(lang->types[i]);
        }
        free(lang->types);
    }
    free(lang);
}

static bool Grammar_Register(LanguageDef *lang) {
    if (!lang) return false;
    Grammar_SortKeywords(lang);

    // Replace existing if matching name
    for (int i = 0; i < g_language_count; ++i) {
        if (strcmp(g_languages[i]->name, lang->name) == 0) {
            Grammar_FreeDef(g_languages[i]);
            g_languages[i] = lang;
            return true;
        }
    }

    if (g_language_count >= g_language_capacity) {
        int new_cap = (g_language_capacity == 0) ? 8 : g_language_capacity * 2;
        LanguageDef **nl = (LanguageDef **)realloc(g_languages, new_cap * sizeof(LanguageDef *));
        if (!nl) return false;
        g_languages = nl;
        g_language_capacity = new_cap;
    }

    g_languages[g_language_count++] = lang;
    return true;
}

// Helper to duplicate array of static string literals
static char **Grammar_CloneStrings(const char *const *src, int count) {
    if (count <= 0 || !src) return NULL;
    char **dst = (char **)malloc(count * sizeof(char *));
    if (!dst) return NULL;
    for (int i = 0; i < count; ++i) {
        size_t len = strlen(src[i]);
        dst[i] = (char *)malloc(len + 1);
        if (dst[i]) strcpy(dst[i], src[i]);
    }
    return dst;
}

// --- Built-in Grammar Registrations ---

static void Grammar_RegisterBuiltinC(void) {
    static const char *const C_EXTS[] = { ".c", ".h", ".cpp", ".hpp", ".cc", ".cxx", ".inl" };
    static const char *const C_KWS[] = {
        "auto", "break", "case", "catch", "class", "const", "const_cast",
        "continue", "default", "delete", "do", "dynamic_cast", "else", "enum",
        "explicit", "export", "extern", "false", "for", "friend", "goto", "if",
        "inline", "mutable", "namespace", "new", "noexcept", "nullptr", "operator",
        "private", "protected", "public", "register", "reinterpret_cast", "restrict",
        "return", "sizeof", "static", "static_assert", "static_cast", "struct",
        "switch", "template", "this", "thread_local", "throw", "true", "try",
        "typedef", "typeid", "typename", "union", "using", "virtual", "volatile",
        "while"
    };
    static const char *const C_TYPES[] = {
        "Camera2D", "Color", "FILE", "Font", "Image", "Sound", "Texture2D",
        "Vector2", "Vector3", "bool", "char", "char16_t", "char32_t", "double",
        "float", "int", "int16_t", "int32_t", "int64_t", "int8_t", "long",
        "ptrdiff_t", "short", "signed", "size_t", "ssize_t", "string", "uint16_t",
        "uint32_t", "uint64_t", "uint8_t", "uintptr_t", "unsigned", "void",
        "wchar_t"
    };

    LanguageDef *c = (LanguageDef *)calloc(1, sizeof(LanguageDef));
    strcpy(c->name, "C/C++");
    c->extensions = Grammar_CloneStrings(C_EXTS, sizeof(C_EXTS)/sizeof(C_EXTS[0]));
    c->extension_count = sizeof(C_EXTS)/sizeof(C_EXTS[0]);
    strcpy(c->single_comment, "//");
    strcpy(c->multi_comment_start, "/*");
    strcpy(c->multi_comment_end, "*/");
    c->string_delims[0] = '"';
    c->string_delims[1] = '\'';
    c->string_delim_count = 2;
    c->keywords = Grammar_CloneStrings(C_KWS, sizeof(C_KWS)/sizeof(C_KWS[0]));
    c->keyword_count = sizeof(C_KWS)/sizeof(C_KWS[0]);
    c->types = Grammar_CloneStrings(C_TYPES, sizeof(C_TYPES)/sizeof(C_TYPES[0]));
    c->type_count = sizeof(C_TYPES)/sizeof(C_TYPES[0]);

    Grammar_Register(c);
}

static void Grammar_RegisterBuiltinPython(void) {
    static const char *const PY_EXTS[] = { ".py", ".pyw", ".pyi" };
    static const char *const PY_KWS[] = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
        "while", "with", "yield"
    };
    static const char *const PY_TYPES[] = {
        "bool", "bytearray", "bytes", "classmethod", "complex", "dict", "enumerate",
        "filter", "float", "frozenset", "int", "list", "map", "memoryview",
        "object", "property", "range", "set", "slice", "staticmethod", "str",
        "super", "tuple", "type", "zip"
    };

    LanguageDef *py = (LanguageDef *)calloc(1, sizeof(LanguageDef));
    strcpy(py->name, "Python");
    py->extensions = Grammar_CloneStrings(PY_EXTS, sizeof(PY_EXTS)/sizeof(PY_EXTS[0]));
    py->extension_count = sizeof(PY_EXTS)/sizeof(PY_EXTS[0]);
    strcpy(py->single_comment, "#");
    strcpy(py->multi_comment_start, "\"\"\"");
    strcpy(py->multi_comment_end, "\"\"\"");
    py->string_delims[0] = '"';
    py->string_delims[1] = '\'';
    py->string_delim_count = 2;
    py->keywords = Grammar_CloneStrings(PY_KWS, sizeof(PY_KWS)/sizeof(PY_KWS[0]));
    py->keyword_count = sizeof(PY_KWS)/sizeof(PY_KWS[0]);
    py->types = Grammar_CloneStrings(PY_TYPES, sizeof(PY_TYPES)/sizeof(PY_TYPES[0]));
    py->type_count = sizeof(PY_TYPES)/sizeof(PY_TYPES[0]);

    Grammar_Register(py);
}

static void Grammar_RegisterBuiltinJS(void) {
    static const char *const JS_EXTS[] = { ".js", ".ts", ".jsx", ".tsx", ".mjs", ".cjs" };
    static const char *const JS_KWS[] = {
        "async", "await", "break", "case", "catch", "class", "const", "continue",
        "debugger", "default", "delete", "do", "else", "enum", "export", "extends",
        "false", "finally", "for", "function", "if", "implements", "import", "in",
        "instanceof", "interface", "let", "new", "null", "package", "private",
        "protected", "public", "return", "static", "super", "switch", "this",
        "throw", "true", "try", "typeof", "undefined", "var", "void", "while",
        "with", "yield"
    };
    static const char *const JS_TYPES[] = {
        "Array", "BigInt", "Boolean", "Date", "Error", "Function", "Map", "Number",
        "Object", "Promise", "RegExp", "Set", "String", "Symbol", "WeakMap",
        "WeakSet", "any", "boolean", "never", "number", "string", "unknown", "void"
    };

    LanguageDef *js = (LanguageDef *)calloc(1, sizeof(LanguageDef));
    strcpy(js->name, "JavaScript/TypeScript");
    js->extensions = Grammar_CloneStrings(JS_EXTS, sizeof(JS_EXTS)/sizeof(JS_EXTS[0]));
    js->extension_count = sizeof(JS_EXTS)/sizeof(JS_EXTS[0]);
    strcpy(js->single_comment, "//");
    strcpy(js->multi_comment_start, "/*");
    strcpy(js->multi_comment_end, "*/");
    js->string_delims[0] = '"';
    js->string_delims[1] = '\'';
    js->string_delims[2] = '`';
    js->string_delim_count = 3;
    js->keywords = Grammar_CloneStrings(JS_KWS, sizeof(JS_KWS)/sizeof(JS_KWS[0]));
    js->keyword_count = sizeof(JS_KWS)/sizeof(JS_KWS[0]);
    js->types = Grammar_CloneStrings(JS_TYPES, sizeof(JS_TYPES)/sizeof(JS_TYPES[0]));
    js->type_count = sizeof(JS_TYPES)/sizeof(JS_TYPES[0]);

    Grammar_Register(js);
}

static void Grammar_RegisterBuiltinJSON(void) {
    static const char *const JSON_EXTS[] = { ".json" };
    static const char *const JSON_KWS[] = { "false", "null", "true" };
    static const char *const JSON_TYPES[] = { "array", "boolean", "number", "object", "string" };

    LanguageDef *j = (LanguageDef *)calloc(1, sizeof(LanguageDef));
    strcpy(j->name, "JSON");
    j->extensions = Grammar_CloneStrings(JSON_EXTS, sizeof(JSON_EXTS)/sizeof(JSON_EXTS[0]));
    j->extension_count = sizeof(JSON_EXTS)/sizeof(JSON_EXTS[0]);
    strcpy(j->single_comment, "//");
    j->string_delims[0] = '"';
    j->string_delim_count = 1;
    j->keywords = Grammar_CloneStrings(JSON_KWS, sizeof(JSON_KWS)/sizeof(JSON_KWS[0]));
    j->keyword_count = sizeof(JSON_KWS)/sizeof(JSON_KWS[0]);
    j->types = Grammar_CloneStrings(JSON_TYPES, sizeof(JSON_TYPES)/sizeof(JSON_TYPES[0]));
    j->type_count = sizeof(JSON_TYPES)/sizeof(JSON_TYPES[0]);

    Grammar_Register(j);
}

static void Grammar_RegisterBuiltinGLSL(void) {
    static const char *const GLSL_EXTS[] = { ".glsl", ".vs", ".fs", ".frag", ".vert" };
    static const char *const GLSL_KWS[] = {
        "attribute", "break", "const", "continue", "discard", "do", "else", "for",
        "if", "in", "inout", "layout", "out", "precision", "return", "uniform",
        "varying", "while"
    };
    static const char *const GLSL_TYPES[] = {
        "bool", "bvec2", "bvec3", "bvec4", "float", "int", "ivec2", "ivec3", "ivec4",
        "mat2", "mat3", "mat4", "sampler2D", "samplerCube", "uint", "uvec2",
        "uvec3", "uvec4", "vec2", "vec3", "vec4", "void"
    };

    LanguageDef *g = (LanguageDef *)calloc(1, sizeof(LanguageDef));
    strcpy(g->name, "GLSL");
    g->extensions = Grammar_CloneStrings(GLSL_EXTS, sizeof(GLSL_EXTS)/sizeof(GLSL_EXTS[0]));
    g->extension_count = sizeof(GLSL_EXTS)/sizeof(GLSL_EXTS[0]);
    strcpy(g->single_comment, "//");
    strcpy(g->multi_comment_start, "/*");
    strcpy(g->multi_comment_end, "*/");
    g->string_delims[0] = '"';
    g->string_delim_count = 1;
    g->keywords = Grammar_CloneStrings(GLSL_KWS, sizeof(GLSL_KWS)/sizeof(GLSL_KWS[0]));
    g->keyword_count = sizeof(GLSL_KWS)/sizeof(GLSL_KWS[0]);
    g->types = Grammar_CloneStrings(GLSL_TYPES, sizeof(GLSL_TYPES)/sizeof(GLSL_TYPES[0]));
    g->type_count = sizeof(GLSL_TYPES)/sizeof(GLSL_TYPES[0]);

    Grammar_Register(g);
}

// --- Lightweight JSON Parser for syntax/*.json ---

static const char *Json_SkipWhitespace(const char *p) {
    while (*p && (unsigned char)*p <= ' ') p++;
    return p;
}

static const char *Json_ParseString(const char *p, char *out_buf, size_t max_len) {
    p = Json_SkipWhitespace(p);
    if (*p != '"') return NULL;
    p++;
    size_t len = 0;
    while (*p && *p != '"') {
        if (*p == '\\') {
            p++;
            if (!*p) break;
            char ch = *p;
            if (ch == 'n') ch = '\n';
            else if (ch == 't') ch = '\t';
            else if (ch == 'r') ch = '\r';
            if (len + 1 < max_len) out_buf[len++] = ch;
        } else {
            if (len + 1 < max_len) out_buf[len++] = *p;
        }
        p++;
    }
    if (*p == '"') p++;
    out_buf[len] = '\0';
    return p;
}

static const char *Json_ParseStringArray(const char *p, char ***out_items, int *out_count) {
    p = Json_SkipWhitespace(p);
    if (*p != '[') return NULL;
    p++;

    int cap = 16;
    int count = 0;
    char **items = (char **)malloc(cap * sizeof(char *));
    if (!items) return NULL;

    while (*p) {
        p = Json_SkipWhitespace(p);
        if (*p == ']') {
            p++;
            break;
        }
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '"') {
            char buf[128];
            p = Json_ParseString(p, buf, sizeof(buf));
            if (!p) break;
            if (count >= cap) {
                cap *= 2;
                char **n = (char **)realloc(items, cap * sizeof(char *));
                if (!n) break;
                items = n;
            }
            items[count] = (char *)malloc(strlen(buf) + 1);
            if (items[count]) {
                strcpy(items[count], buf);
                count++;
            }
        } else {
            p++;
        }
    }

    *out_items = items;
    *out_count = count;
    return p;
}

static const char *Json_SkipValue(const char *p) {
    p = Json_SkipWhitespace(p);
    if (!*p) return p;
    if (*p == '"') {
        char dummy[256];
        return Json_ParseString(p, dummy, sizeof(dummy));
    }
    if (*p == '[') {
        p++;
        int depth = 1;
        while (*p && depth > 0) {
            if (*p == '[') depth++;
            else if (*p == ']') depth--;
            else if (*p == '"') {
                char dummy[256];
                p = Json_ParseString(p, dummy, sizeof(dummy));
                continue;
            }
            if (*p) p++;
        }
        return p;
    }
    if (*p == '{') {
        p++;
        int depth = 1;
        while (*p && depth > 0) {
            if (*p == '{') depth++;
            else if (*p == '}') depth--;
            else if (*p == '"') {
                char dummy[256];
                p = Json_ParseString(p, dummy, sizeof(dummy));
                continue;
            }
            if (*p) p++;
        }
        return p;
    }
    while (*p && *p != ',' && *p != '}' && *p != ']') p++;
    return p;
}

LanguageDef *Grammar_LoadFromString(const char *json_str) {
    if (!json_str) return NULL;
    const char *p = Json_SkipWhitespace(json_str);
    if (*p != '{') return NULL;
    p++;

    LanguageDef *lang = (LanguageDef *)calloc(1, sizeof(LanguageDef));
    if (!lang) return NULL;

    while (*p) {
        p = Json_SkipWhitespace(p);
        if (*p == '}') {
            p++;
            break;
        }
        if (*p == ',') {
            p++;
            continue;
        }

        char key[64];
        p = Json_ParseString(p, key, sizeof(key));
        if (!p) break;

        p = Json_SkipWhitespace(p);
        if (*p != ':') break;
        p++;

        if (strcmp(key, "name") == 0) {
            p = Json_ParseString(p, lang->name, sizeof(lang->name));
        } else if (strcmp(key, "single_comment") == 0) {
            p = Json_ParseString(p, lang->single_comment, sizeof(lang->single_comment));
        } else if (strcmp(key, "multi_comment_start") == 0) {
            p = Json_ParseString(p, lang->multi_comment_start, sizeof(lang->multi_comment_start));
        } else if (strcmp(key, "multi_comment_end") == 0) {
            p = Json_ParseString(p, lang->multi_comment_end, sizeof(lang->multi_comment_end));
        } else if (strcmp(key, "extensions") == 0) {
            char **exts = NULL;
            int count = 0;
            p = Json_ParseStringArray(p, &exts, &count);
            // Ensure dot prefix
            for (int i = 0; i < count; ++i) {
                if (exts[i] && exts[i][0] != '.') {
                    char fixed[64];
                    snprintf(fixed, sizeof(fixed), ".%s", exts[i]);
                    free(exts[i]);
                    exts[i] = (char *)malloc(strlen(fixed) + 1);
                    if (exts[i]) strcpy(exts[i], fixed);
                }
            }
            lang->extensions = exts;
            lang->extension_count = count;
        } else if (strcmp(key, "string_delimiters") == 0) {
            char **delims = NULL;
            int count = 0;
            p = Json_ParseStringArray(p, &delims, &count);
            for (int i = 0; i < count && i < CE_MAX_GRAMMAR_DELIMS; ++i) {
                if (delims[i] && delims[i][0]) {
                    lang->string_delims[lang->string_delim_count++] = delims[i][0];
                }
                free(delims[i]);
            }
            free(delims);
        } else if (strcmp(key, "keywords") == 0) {
            p = Json_ParseStringArray(p, &lang->keywords, &lang->keyword_count);
        } else if (strcmp(key, "types") == 0) {
            p = Json_ParseStringArray(p, &lang->types, &lang->type_count);
        } else {
            p = Json_SkipValue(p);
        }

        if (!p) break;
    }

    if (lang->name[0] == '\0') {
        strcpy(lang->name, "Custom");
    }
    if (lang->string_delim_count == 0) {
        lang->string_delims[0] = '"';
        lang->string_delims[1] = '\'';
        lang->string_delim_count = 2;
    }

    Grammar_SortKeywords(lang);
    return lang;
}

bool Grammar_LoadFile(const char *filepath) {
    if (!filepath || !FileExists(filepath)) return false;
    char *text = LoadFileText(filepath);
    if (!text) return false;

    LanguageDef *lang = Grammar_LoadFromString(text);
    UnloadFileText(text);

    if (lang) {
        return Grammar_Register(lang);
    }
    return false;
}

int Grammar_LoadDirectory(const char *dir_path) {
    if (!dir_path || !DirectoryExists(dir_path)) return 0;
    FilePathList files = LoadDirectoryFiles(dir_path);
    int loaded = 0;

    for (unsigned int i = 0; i < files.count; ++i) {
        if (IsFileExtension(files.paths[i], ".json")) {
            if (Grammar_LoadFile(files.paths[i])) {
                loaded++;
            }
        }
    }

    UnloadDirectoryFiles(files);
    return loaded;
}

void Grammar_Init(void) {
    if (g_grammar_initialized) return;

    Grammar_RegisterBuiltinC();
    Grammar_RegisterBuiltinPython();
    Grammar_RegisterBuiltinJS();
    Grammar_RegisterBuiltinJSON();
    Grammar_RegisterBuiltinGLSL();

    // Check for user-defined external grammar directory
    if (DirectoryExists("syntax")) {
        Grammar_LoadDirectory("syntax");
    }

    atexit(Grammar_Free);
    g_grammar_initialized = true;
}

void Grammar_Free(void) {
    if (!g_grammar_initialized) return;
    for (int i = 0; i < g_language_count; ++i) {
        Grammar_FreeDef(g_languages[i]);
    }
    free(g_languages);
    g_languages = NULL;
    g_language_count = 0;
    g_language_capacity = 0;
    g_grammar_initialized = false;
}

const LanguageDef *Grammar_GetDefault(void) {
    if (!g_grammar_initialized) Grammar_Init();
    if (g_language_count > 0) return g_languages[0];
    return NULL;
}

const LanguageDef *Grammar_GetByName(const char *name) {
    if (!name) return Grammar_GetDefault();
    if (!g_grammar_initialized) Grammar_Init();

    for (int i = 0; i < g_language_count; ++i) {
        if (strcmp(g_languages[i]->name, name) == 0) {
            return g_languages[i];
        }
    }
    return Grammar_GetDefault();
}

static bool ExtMatches(const char *ext1, const char *ext2) {
    if (!ext1 || !ext2) return false;
    while (*ext1 && *ext2) {
        if (tolower((unsigned char)*ext1) != tolower((unsigned char)*ext2)) return false;
        ext1++;
        ext2++;
    }
    return (*ext1 == '\0' && *ext2 == '\0');
}

const LanguageDef *Grammar_GetByExtension(const char *ext) {
    if (!ext || ext[0] == '\0') return Grammar_GetDefault();
    if (!g_grammar_initialized) Grammar_Init();

    for (int i = 0; i < g_language_count; ++i) {
        const LanguageDef *lang = g_languages[i];
        for (int j = 0; j < lang->extension_count; ++j) {
            if (ExtMatches(ext, lang->extensions[j])) {
                return lang;
            }
        }
    }
    return Grammar_GetDefault();
}

const LanguageDef *Grammar_GetByFilename(const char *filepath) {
    if (!filepath || filepath[0] == '\0') return Grammar_GetDefault();
    const char *ext = GetFileExtension(filepath);
    return Grammar_GetByExtension(ext);
}

bool Grammar_IsKeyword(const LanguageDef *lang, const char *word) {
    if (!lang || !word || lang->keyword_count == 0 || !lang->keywords) return false;
    const char *key = word;
    const char **found = (const char **)bsearch(&key, lang->keywords, lang->keyword_count, sizeof(char *), Grammar_StringCompare);
    return (found != NULL);
}

bool Grammar_IsType(const LanguageDef *lang, const char *word) {
    if (!lang || !word || lang->type_count == 0 || !lang->types) return false;
    const char *key = word;
    const char **found = (const char **)bsearch(&key, lang->types, lang->type_count, sizeof(char *), Grammar_StringCompare);
    return (found != NULL);
}