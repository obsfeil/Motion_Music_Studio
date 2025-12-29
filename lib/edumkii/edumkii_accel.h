/**
 * @file edumkii_accel.h
 * @brief EDUMKII Accelerometer Library
 * @version 1.0.0
 * 
 * Provides accelerometer reading with deadzone filtering and helpers for:
 * - Tilt detection (octave shift)
 * - Position mapping (scale position)
 * - Orientation detection
 * 
 * Usage:
 *   Accelerometer_t accel;
 *   Accel_Init(&accel, 100);  // 100 = deadzone
 *   
 *   // In main loop:
 *   Accel_Update(&accel, gSynthState.accel_x, gSynthState.accel_y, gSynthState.accel_z);
 *   
 *   int8_t tilt = Accel_GetTilt(&accel);  // -1, 0, +1
 */

#ifndef EDUMKII_ACCEL_H_
#define EDUMKII_ACCEL_H_

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// PUBLIC TYPES
//=============================================================================

/**
 * @brief Accelerometer structure with deadzone filtering
 */
typedef struct {
    int16_t x;            ///< Current X value
    int16_t y;            ///< Current Y value
    int16_t z;            ///< Current Z value
    bool x_changed;       ///< True if X moved outside deadzone
    bool y_changed;       ///< True if Y moved outside deadzone
    bool z_changed;       ///< True if Z moved outside deadzone
    uint16_t deadzone;    ///< Deadzone size
    
    // Internal state (don't modify directly)
    int16_t _last_x;
    int16_t _last_y;
    int16_t _last_z;
    bool _first_run;
} Accelerometer_t;

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * @brief Initialize accelerometer
 * @param accel Pointer to accelerometer structure
 * @param deadzone Deadzone size (recommended: 100)
 */
void Accel_Init(Accelerometer_t *accel, uint16_t deadzone);

/**
 * @brief Update accelerometer values (call every loop iteration)
 * @param accel Pointer to accelerometer structure
 * @param raw_x Raw X ADC value
 * @param raw_y Raw Y ADC value
 * @param raw_z Raw Z ADC value
 */
void Accel_Update(Accelerometer_t *accel, int16_t raw_x, int16_t raw_y, int16_t raw_z);

/**
 * @brief Get tilt direction (for octave shift)
 * @param accel Pointer to accelerometer structure
 * @return -1 = tilted back (low octave), 0 = flat, +1 = tilted forward (high octave)
 * 
 * Uses Y axis with threshold of 300 units from neutral (2849)
 */
int8_t Accel_GetTilt(Accelerometer_t *accel);

/**
 * @brief Get scale position (0-7) from X tilt
 * @param accel Pointer to accelerometer structure
 * @return Position 0-7
 * 
 * Maps X axis tilt to 8 positions:
 *   Far left  = 0
 *   Left      = 1-2
 *   Center    = 3-4
 *   Right     = 5-6
 *   Far right = 7
 */
uint8_t Accel_GetScalePosition(Accelerometer_t *accel);

/**
 * @brief Get X deviation from neutral position
 * @param accel Pointer to accelerometer structure
 * @return Deviation from neutral (2048)
 */
int16_t Accel_GetXDeviation(Accelerometer_t *accel);

/**
 * @brief Get Y deviation from neutral position
 * @param accel Pointer to accelerometer structure
 * @return Deviation from neutral (2849)
 */
int16_t Accel_GetYDeviation(Accelerometer_t *accel);

/**
 * @brief Check if board is flat (not tilted)
 * @param accel Pointer to accelerometer structure
 * @return true if board is flat
 */
bool Accel_IsFlat(Accelerometer_t *accel);

#endif /* EDUMKII_ACCEL_H_ */
