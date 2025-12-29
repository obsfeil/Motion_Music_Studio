/**
 * @file edumkii_accel.c
 * @brief EDUMKII Accelerometer Implementation
 */

#include "edumkii_accel.h"

//=============================================================================
// CONSTANTS (from EDUMKII hardware)
//=============================================================================
#define ACCEL_X_NEUTRAL 2048
#define ACCEL_Y_NEUTRAL 2849
#define ACCEL_Y_THRESHOLD 300

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void Accel_Init(Accelerometer_t *accel, uint16_t deadzone) {
    accel->x = ACCEL_X_NEUTRAL;
    accel->y = ACCEL_Y_NEUTRAL;
    accel->z = 0;
    accel->x_changed = false;
    accel->y_changed = false;
    accel->z_changed = false;
    accel->deadzone = deadzone;
    accel->_last_x = ACCEL_X_NEUTRAL;
    accel->_last_y = ACCEL_Y_NEUTRAL;
    accel->_last_z = 0;
    accel->_first_run = true;
}

void Accel_Update(Accelerometer_t *accel, int16_t raw_x, int16_t raw_y, int16_t raw_z) {
    // Initialize on first run
    if (accel->_first_run) {
        accel->_last_x = raw_x;
        accel->_last_y = raw_y;
        accel->_last_z = raw_z;
        accel->x = raw_x;
        accel->y = raw_y;
        accel->z = raw_z;
        accel->_first_run = false;
        accel->x_changed = false;
        accel->y_changed = false;
        accel->z_changed = false;
        return;
    }
    
    // Check X axis deadzone
    int16_t x_change = raw_x - accel->_last_x;
    if (x_change < -(int16_t)accel->deadzone || x_change > (int16_t)accel->deadzone) {
        accel->x = raw_x;
        accel->_last_x = raw_x;
        accel->x_changed = true;
    } else {
        accel->x_changed = false;
    }
    
    // Check Y axis deadzone
    int16_t y_change = raw_y - accel->_last_y;
    if (y_change < -(int16_t)accel->deadzone || y_change > (int16_t)accel->deadzone) {
        accel->y = raw_y;
        accel->_last_y = raw_y;
        accel->y_changed = true;
    } else {
        accel->y_changed = false;
    }
    
    // Check Z axis deadzone
    int16_t z_change = raw_z - accel->_last_z;
    if (z_change < -(int16_t)accel->deadzone || z_change > (int16_t)accel->deadzone) {
        accel->z = raw_z;
        accel->_last_z = raw_z;
        accel->z_changed = true;
    } else {
        accel->z_changed = false;
    }
}

int8_t Accel_GetTilt(Accelerometer_t *accel) {
    int16_t deviation = accel->y - ACCEL_Y_NEUTRAL;
    
    if (deviation < -ACCEL_Y_THRESHOLD) {
        return -1;  // Tilted back (low octave)
    } else if (deviation > ACCEL_Y_THRESHOLD) {
        return +1;  // Tilted forward (high octave)
    } else {
        return 0;   // Flat
    }
}

uint8_t Accel_GetScalePosition(Accelerometer_t *accel) {
    int16_t deviation = accel->x - ACCEL_X_NEUTRAL;
    
    // Map deviation to 8 positions
    if (deviation < -600)
        return 0;
    else if (deviation < -400)
        return 1;
    else if (deviation < -200)
        return 2;
    else if (deviation < 200)
        return 3;
    else if (deviation < 400)
        return 4;
    else if (deviation < 600)
        return 5;
    else if (deviation < 800)
        return 6;
    else
        return 7;
}

int16_t Accel_GetXDeviation(Accelerometer_t *accel) {
    return accel->x - ACCEL_X_NEUTRAL;
}

int16_t Accel_GetYDeviation(Accelerometer_t *accel) {
    return accel->y - ACCEL_Y_NEUTRAL;
}

bool Accel_IsFlat(Accelerometer_t *accel) {
    int16_t y_deviation = accel->y - ACCEL_Y_NEUTRAL;
    return (y_deviation > -ACCEL_Y_THRESHOLD && y_deviation < ACCEL_Y_THRESHOLD);
}