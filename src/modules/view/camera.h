#ifndef CE_MODULES_VIEW_CAMERA_H
#define CE_MODULES_VIEW_CAMERA_H

#include "raylib.h"
#include "core/types.h"
#include "modules/buffer/document.h"

// Binds the camera to the Document and the Event Bus.
void Camera_Init(const Document *doc, Font font_syntax);

// Unsubscribes from the Event Bus and releases camera references.
void Camera_Close(void);

// Steps the camera smoothing physics and handles window resizes.
void Camera_Update(float dt);

// Injects FX shake trauma.
void Camera_SetShakeOffset(Vector2 offset);

// Returns the fully computed Raylib camera for 2D mode drawing.
Camera2D Camera_GetState(void);

// Camera state accessors & mutators
CCameraMode Camera_GetMode(void);
void Camera_SetMode(CCameraMode mode);

float Camera_GetUserZoomMult(void);
void Camera_SetUserZoomMult(float mult);
void Camera_ResetZoom(void);

// Immediately aligns the camera position and zoom with its target (no lerp delay).
void Camera_SnapToTarget(void);

#endif // CE_MODULES_VIEW_CAMERA_H