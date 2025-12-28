//=============================================================================
// FILE 5: edumkii_accel.h
//=============================================================================

#ifndef EDUMKII_ACCEL_H_
#define EDUMKII_ACCEL_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    bool x_changed;
    bool y_changed;
    bool z_changed;
    uint16_t deadzone;
} Accelerometer_t;

void Accel_Init(Accelerometer_t *accel, uint16_t deadzone);
void Accel_Update(Accelerometer_t *accel, int16_t raw_x, int16_t raw_y, int16_t raw_z);
int8_t Accel_GetTilt(Accelerometer_t *accel);  // Returns -1, 0, +1
uint8_t Accel_GetScalePosition(Accelerometer_t *accel);  // Returns 0-7

#endif
