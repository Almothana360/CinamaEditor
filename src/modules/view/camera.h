#ifndef CE_MODULES_VIEW_CAMERA_H
#define CE_MODULES_VIEW_CAMERA_H

#include "raylib.h"
#include "core/types.h"
#include "modules/buffer/document.h"

// Binds the camera to the Document and the Event Bus.
void Camera_Init(const Document *doc, Font font_syntax);

// Steps the camera smoothing physics and handles window resizes.
void Camera_Update(float dt);

// Injects FX shake trauma.
void Camera_SetShakeOffset(Vector2 offset);

// Returns the fully computed Raylib camera for 2D mode drawing.
Camera2D Camera_GetState(void);

CCameraMode Camera_GetMode(void);
float Camera_GetUserZoomMult(void);

#endif // CE_MODULES_VIEW_CAMERA_H