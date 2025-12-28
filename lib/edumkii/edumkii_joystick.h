//=============================================================================
// FILE 3: edumkii_joystick.h
//=============================================================================

#ifndef EDUMKII_JOYSTICK_H_
#define EDUMKII_JOYSTICK_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t x;           // Raw X value (0-4095)
    uint16_t y;           // Raw Y value (0-4095)
    bool x_changed;       // True if X moved outside deadzone
    bool y_changed;       // True if Y moved outside deadzone
    uint16_t deadzone;    // Deadzone size (default 100)
} Joystick_t;

// Public API
void Joystick_Init(Joystick_t *joy, uint16_t deadzone);
void Joystick_Update(Joystick_t *joy, uint16_t raw_x, uint16_t raw_y);
int16_t Joystick_GetX(Joystick_t *joy);      // Returns -2048 to +2047 (centered)
int16_t Joystick_GetY(Joystick_t *joy);
uint8_t Joystick_GetKeyIndex(Joystick_t *joy, uint8_t num_keys);  // For key selection
uint8_t Joystick_GetVolume(Joystick_t *joy); // Returns 0-100%

#endif
