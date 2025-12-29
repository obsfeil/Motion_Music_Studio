/**
 * @file edumkii_joystick.h
 * @brief EDUMKII Joystick Library with Deadzone
 * @version 1.0.0
 * 
 * Provides joystick reading with deadzone filtering to prevent:
 * - Jitter and noise
 * - Accidental changes
 * - Value drift
 * 
 * Features:
 * - Configurable deadzone
 * - Change detection flags
 * - Helper functions for common use cases (key selection, volume)
 * 
 * Usage:
 *   Joystick_t joy;
 *   Joystick_Init(&joy, 100);  // 100 = deadzone
 *   
 *   // In main loop:
 *   Joystick_Update(&joy, gSynthState.joy_x, gSynthState.joy_y);
 *   
 *   if (joy.x_changed) {
 *       uint8_t key = Joystick_GetKeyIndex(&joy, 7);
 *   }
 */

#ifndef EDUMKII_JOYSTICK_H_
#define EDUMKII_JOYSTICK_H_

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// PUBLIC TYPES
//=============================================================================

/**
 * @brief Joystick structure with deadzone filtering
 */
typedef struct {
    uint16_t x;           ///< Current X value (0-4095)
    uint16_t y;           ///< Current Y value (0-4095)
    bool x_changed;       ///< True if X moved outside deadzone since last update
    bool y_changed;       ///< True if Y moved outside deadzone since last update
    uint16_t deadzone;    ///< Deadzone size (recommended: 100)
    
    // Internal state (don't modify directly)
    uint16_t _last_x;
    uint16_t _last_y;
    bool _first_run;
} Joystick_t;

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * @brief Initialize joystick
 * @param joy Pointer to joystick structure
 * @param deadzone Deadzone size (0-500, recommended: 100)
 */
void Joystick_Init(Joystick_t *joy, uint16_t deadzone);

/**
 * @brief Update joystick values (call every loop iteration)
 * @param joy Pointer to joystick structure
 * @param raw_x Raw X ADC value (0-4095)
 * @param raw_y Raw Y ADC value (0-4095)
 */
void Joystick_Update(Joystick_t *joy, uint16_t raw_x, uint16_t raw_y);

/**
 * @brief Get X position relative to center (-2048 to +2047)
 * @param joy Pointer to joystick structure
 * @return X deviation from center (negative = left, positive = right)
 */
int16_t Joystick_GetX(Joystick_t *joy);

/**
 * @brief Get Y position relative to center (-2048 to +2047)
 * @param joy Pointer to joystick structure
 * @return Y deviation from center (negative = down, positive = up)
 */
int16_t Joystick_GetY(Joystick_t *joy);

/**
 * @brief Get key index for key selection (0 to num_keys-1)
 * @param joy Pointer to joystick structure
 * @param num_keys Number of keys (e.g., 7 for C-B)
 * @return Key index (0 to num_keys-1)
 * 
 * Example: 7 keys (C, D, E, F, G, A, B)
 *   Left = 0 (C), Center = 3 (F), Right = 6 (B)
 */
uint8_t Joystick_GetKeyIndex(Joystick_t *joy, uint8_t num_keys);

/**
 * @brief Get volume from Y axis (0-100%)
 * @param joy Pointer to joystick structure
 * @return Volume 0-100%, or 255 if in center deadzone (no change)
 * 
 * Note: Returns 255 if joystick is within ±200 of center,
 *       indicating volume should not be changed.
 */
uint8_t Joystick_GetVolume(Joystick_t *joy);

/**
 * @brief Check if joystick is centered (within deadzone)
 * @param joy Pointer to joystick structure
 * @return true if both X and Y are centered
 */
bool Joystick_IsCentered(Joystick_t *joy);

#endif /* EDUMKII_JOYSTICK_H_ */
