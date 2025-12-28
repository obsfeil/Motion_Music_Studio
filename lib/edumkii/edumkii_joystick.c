//=============================================================================
// FILE 4: edumkii_joystick.c
//=============================================================================

#include "edumkii_joystick.h"

typedef struct {
    uint16_t last_x;
    uint16_t last_y;
    bool first_run;
} Joystick_Internal_t;

static Joystick_Internal_t internal = {0};

void Joystick_Init(Joystick_t *joy, uint16_t deadzone) {
    joy->x = 2048;
    joy->y = 2048;
    joy->x_changed = false;
    joy->y_changed = false;
    joy->deadzone = deadzone;
    internal.first_run = true;
}

void Joystick_Update(Joystick_t *joy, uint16_t raw_x, uint16_t raw_y) {
    // Initialize on first run
    if (internal.first_run) {
        internal.last_x = raw_x;
        internal.last_y = raw_y;
        internal.first_run = false;
        return;
    }
    
    // Check X deadzone
    int16_t x_change = (int16_t)raw_x - (int16_t)internal.last_x;
    if (x_change < -(int16_t)joy->deadzone || x_change > (int16_t)joy->deadzone) {
        joy->x = raw_x;
        internal.last_x = raw_x;
        joy->x_changed = true;
    } else {
        joy->x_changed = false;
    }
    
    // Check Y deadzone
    int16_t y_change = (int16_t)raw_y - (int16_t)internal.last_y;
    if (y_change < -(int16_t)joy->deadzone || y_change > (int16_t)joy->deadzone) {
        joy->y = raw_y;
        internal.last_y = raw_y;
        joy->y_changed = true;
    } else {
        joy->y_changed = false;
    }
}

int16_t Joystick_GetX(Joystick_t *joy) {
    return (int16_t)joy->x - 2048;
}

int16_t Joystick_GetY(Joystick_t *joy) {
    return (int16_t)joy->y - 2048;
}

uint8_t Joystick_GetKeyIndex(Joystick_t *joy, uint8_t num_keys) {
    uint16_t x = joy->x;
    if (x > 4095) x = 4095;
    uint8_t key = (x * num_keys) / 4096;
    if (key >= num_keys) key = num_keys - 1;
    return key;
}

uint8_t Joystick_GetVolume(Joystick_t *joy) {
    uint16_t y = joy->y;
    if (y > 4095) y = 4095;
    
    // Apply center deadzone
    int16_t deviation = (int16_t)y - 2048;
    if (deviation > -200 && deviation < 200) {
        return 255;  // Special value = no change
    }
    
    uint8_t vol = (y * 100) / 4095;
    if (vol > 100) vol = 100;
    return vol;
}
