#ifndef CE_MODULES_INPUT_H
#define CE_MODULES_INPUT_H

// Polls Raylib input state, translates keystrokes and mouse wheel
// into semantic actions, and broadcasts them via the Event Bus.
void Input_Update(void);

#endif // CE_MODULES_INPUT_H