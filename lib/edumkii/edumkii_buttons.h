/**
 * @file edumkii_buttons.h
 * @brief EDUMKII Button State Machine Library
 * @version 1.0.0
 * 
 * Provides button debouncing and event detection:
 * - Short click
 * - Long press
 * - Double click
 * 
 * Usage:
 *   Button_t btn;
 *   Button_Init(&btn);
 *   
 *   // In SysTick @ 100 Hz:
 *   Button_Update(&btn, GPIO_PORT, GPIO_PIN);
 *   
 *   // In main loop:
 *   ButtonEvent_t event = Button_GetEvent(&btn);
 *   if (event == BTN_EVENT_SHORT_CLICK) { ... }
 */

#ifndef EDUMKII_BUTTONS_H_
#define EDUMKII_BUTTONS_H_

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"  // For GPIO_Regs type

//=============================================================================
// PUBLIC TYPES
//=============================================================================

/**
 * @brief Button events that can be detected
 */
typedef enum {
    BTN_EVENT_NONE = 0,          ///< No event
    BTN_EVENT_SHORT_CLICK,       ///< Short click (< 200ms)
    BTN_EVENT_LONG_PRESS,        ///< Long press (> 500ms)
    BTN_EVENT_DOUBLE_CLICK       ///< Double click (< 500ms between clicks)
} ButtonEvent_t;

/**
 * @brief Button state machine structure (opaque to user)
 */
typedef struct {
    uint8_t state;           ///< Internal state
    uint8_t value;           ///< Current event value
    uint16_t cnt;            ///< Tick counter
    uint8_t raw_state;       ///< Current GPIO state
    uint8_t last_raw_state;  ///< Previous GPIO state
} Button_t;

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * @brief Initialize button state machine
 * @param btn Pointer to button structure
 */
void Button_Init(Button_t *btn);

/**
 * @brief Update button state (call from SysTick @ 100 Hz)
 * @param btn Pointer to button structure
 * @param gpio_port GPIO port pointer (e.g., GPIO_BUTTONS_PORT)
 * @param gpio_pin GPIO pin (e.g., GPIO_BUTTONS_S1_MKII_PIN)
 * 
 * Note: gpio_port must be a pointer (GPIO_Regs*), not uint32_t!
 */
void Button_Update(Button_t *btn, GPIO_Regs *gpio_port, uint32_t gpio_pin);

/**
 * @brief Get button event (clears event after reading)
 * @param btn Pointer to button structure
 * @return Button event (or BTN_EVENT_NONE)
 */
ButtonEvent_t Button_GetEvent(Button_t *btn);

/**
 * @brief Check if button is currently pressed (does not clear event)
 * @param btn Pointer to button structure
 * @return true if button is pressed
 */
bool Button_IsPressed(Button_t *btn);

#endif /* EDUMKII_BUTTONS_H_ */