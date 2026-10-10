#ifndef CE_CORE_TYPES_H
#define CE_CORE_TYPES_H

#include <stddef.h>
#include <stdbool.h>

// --- Cinema Editor Application Metadata ---
#define CE_APP_NAME_LONG   "Cinema Editor"
#define CE_APP_NAME_SHORT  "CE"
#define CE_APP_DEFAULT_TITLE "Cinema Editor (CE)"

// --- Text & Layout Metrics ---
#define CE_TAB_SIZE        4
#define CE_FONT_SIZE       64.0f
#define CE_FONT_SPACING    1.5f

// --- Particle & Visual FX Limits ---
#define CE_MAX_PARTICLES    768
#define CE_MAX_GLOW_FLASHES 64

// --- Camera Zoom Constraints ---
#define CE_MAX_CAMERA_ZOOM 2.80f
#define CE_MIN_CAMERA_ZOOM 0.40f

// --- Aliases for Seamless Internal Module Compatibility ---
#define TAB_SIZE           CE_TAB_SIZE
#define FONT_SIZE          CE_FONT_SIZE
#define FONT_SPACING       CE_FONT_SPACING
#define MAX_PARTICLES      CE_MAX_PARTICLES
#define MAX_GLOW_FLASHES   CE_MAX_GLOW_FLASHES
#define MAX_CAMERA_ZOOM    CE_MAX_CAMERA_ZOOM
#define MIN_CAMERA_ZOOM    CE_MIN_CAMERA_ZOOM

// --- Cinema Camera Modes ---
typedef enum {
    CAM_MODE_BOUNDS_FIT = 0,
    CAM_MODE_CURSOR_FOCUS,
    CAM_MODE_LINE_FOCUS,
    CAM_MODE_COUNT
} CCameraMode;

#endif // CE_CORE_TYPES_H