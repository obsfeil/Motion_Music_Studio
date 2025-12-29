/**
 * @file edumkii_joystick.c
 * @brief EDUMKII Joystick Implementation
 */

#include "edumkii_joystick.h"

//=============================================================================
// CONSTANTS
//=============================================================================
#define JOYSTICK_CENTER 2048
#define CENTER_DEADZONE 200  // For volume detection

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void Joystick_Init(Joystick_t *joy, uint16_t deadzone) {
    joy->x = JOYSTICK_CENTER;
    joy->y = JOYSTICK_CENTER;
    joy->x_changed = false;
    joy->y_changed = false;
    joy->deadzone = deadzone;
    joy->_last_x = JOYSTICK_CENTER;
    joy->_last_y = JOYSTICK_CENTER;
    joy->_first_run = true;
}

void Joystick_Update(Joystick_t *joy, uint16_t raw_x, uint16_t raw_y) {
    // Initialize on first run
    if (joy->_first_run) {
        joy->_last_x = raw_x;
        joy->_last_y = raw_y;
        joy->x = raw_x;
        joy->y = raw_y;
        joy->_first_run = false;
        joy->x_changed = false;
        joy->y_changed = false;
        return;
    }
    
    // Check X axis deadzone
    int16_t x_change = (int16_t)raw_x - (int16_t)joy->_last_x;
    if (x_change < -(int16_t)joy->deadzone || x_change > (int16_t)joy->deadzone) {
        joy->x = raw_x;
        joy->_last_x = raw_x;
        joy->x_changed = true;
    } else {
        joy->x_changed = false;
    }
    
    // Check Y axis deadzone
    int16_t y_change = (int16_t)raw_y - (int16_t)joy->_last_y;
    if (y_change < -(int16_t)joy->deadzone || y_change > (int16_t)joy->deadzone) {
        joy->y = raw_y;
        joy->_last_y = raw_y;
        joy->y_changed = true;
    } else {
        joy->y_changed = false;
    }
}

int16_t Joystick_GetX(Joystick_t *joy) {
    return (int16_t)joy->x - JOYSTICK_CENTER;
}

int16_t Joystick_GetY(Joystick_t *joy) {
    return (int16_t)joy->y - JOYSTICK_CENTER;
}

uint8_t Joystick_GetKeyIndex(Joystick_t *joy, uint8_t num_keys) {
    uint16_t x = joy->x;
    if (x > 4095) x = 4095;
    
    // Map X position to key index
    uint8_t key = (x * num_keys) / 4096;
    if (key >= num_keys) key = num_keys - 1;
    
    return key;
}

uint8_t Joystick_GetVolume(Joystick_t *joy) {
    uint16_t y = joy->y;
    if (y > 4095) y = 4095;
    
    // Check if in center deadzone
    int16_t deviation = (int16_t)y - JOYSTICK_CENTER;
    if (deviation > -CENTER_DEADZONE && deviation < CENTER_DEADZONE) {
        return 255;  // Special value = no change
    }
    
    // Map Y to volume 0-100%
    uint8_t vol = (y * 100) / 4095;
    if (vol > 100) vol = 100;
    
    return vol;
}

bool Joystick_IsCentered(Joystick_t *joy) {
    int16_t x_dev = Joystick_GetX(joy);
    int16_t y_dev = Joystick_GetY(joy);
    
    return (x_dev > -CENTER_DEADZONE && x_dev < CENTER_DEADZONE &&
            y_dev > -CENTER_DEADZONE && y_dev < CENTER_DEADZONE);
}
