#ifndef EDUMKII_JOYSTICK_H_
#define EDUMKII_JOYSTICK_H_

#include <stdint.h>
#include <stdbool.h> // ✅ DENNE MANGLER! Dette fikser 'undeclared identifier true'

typedef struct {
    uint16_t raw_x;
    uint16_t raw_y;
    uint16_t deadzone;
    bool x_changed;
    bool y_changed;
} Joystick_t;

// API
void Joystick_Init(Joystick_t *joy, uint16_t deadzone);
void Joystick_Update(Joystick_t *joy, uint16_t raw_x, uint16_t raw_y);
uint8_t Joystick_GetVolume(Joystick_t *joy);
int8_t Joystick_GetDirectionX(Joystick_t *joy);

#endif