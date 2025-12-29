/**
 * @file edumkii_buttons.c
 * @brief EDUMKII Button State Machine Implementation
 * @details FIXED: Updated types to match DriverLib (GPIO_Regs *)
 */

#include "edumkii_buttons.h"
#include <ti/driverlib/dl_gpio.h> // Viktig: Definerer GPIO_Regs

//=============================================================================
// TIMING CONSTANTS (based on 100 Hz SysTick)
//=============================================================================
#define BTN_SHORT_TIME  20   // 200ms
#define BTN_LONG_TIME   50   // 500ms
#define BTN_DOUBLE_TIME 50   // 500ms between clicks

//=============================================================================
// INTERNAL STATE MACHINE STATES
//=============================================================================
typedef enum {
    STATE_IDLE = 0,
    STATE_FIRST_CLICK,
    STATE_WAIT_DOUBLE,
    STATE_DOUBLE_CLICK
} ButtonState_t;

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void Button_Init(Button_t *btn) {
    btn->state = STATE_IDLE;
    btn->value = BTN_EVENT_NONE;
    btn->cnt = 0;
    btn->raw_state = 0;
    btn->last_raw_state = 0;
}

// ✅ FIKSET HER: Endret 'uint32_t gpio_port' til 'GPIO_Regs *gpio_port'
void Button_Update(Button_t *btn, GPIO_Regs *gpio_port, uint32_t gpio_pin) {
    
    // Read GPIO (Active Low: 0 = pressed)
    // DL_GPIO_readPins krever (GPIO_Regs *, uint32_t)
    bool is_low = (DL_GPIO_readPins(gpio_port, gpio_pin) == 0);
    
    // Vi lagrer rå-statusen (0 for trykket, 1 for slippet i denne logikken)
    btn->raw_state = is_low ? 0 : 1; 
    
    // Detect edges (Logic: 0 is pressed)
    uint8_t pressed = (btn->last_raw_state == 1) && (btn->raw_state == 0);
    uint8_t released = (btn->last_raw_state == 0) && (btn->raw_state == 1);
    
    btn->last_raw_state = btn->raw_state;
    
    // State machine
    switch (btn->state) {
        case STATE_IDLE:
            if (pressed) {
                btn->state = STATE_FIRST_CLICK;
                btn->cnt = 0;
                btn->value = BTN_EVENT_NONE;
            }
            break;
            
        case STATE_FIRST_CLICK:
            btn->cnt++;
            if (released) {
                if (btn->cnt < BTN_SHORT_TIME) {
                    // Short press - wait for possible double click
                    btn->state = STATE_WAIT_DOUBLE;
                    btn->cnt = 0;
                } else if (btn->cnt < BTN_LONG_TIME) {
                    // Medium press - treat as short click immediately
                    btn->state = STATE_IDLE;
                    btn->value = BTN_EVENT_SHORT_CLICK;
                    btn->cnt = 0;
                } else {
                    // Released after long press (already handled)
                    btn->state = STATE_IDLE;
                    btn->cnt = 0;
                }
            } else if (btn->cnt >= BTN_LONG_TIME) {
                // Long press detected while still held
                btn->state = STATE_IDLE;
                btn->value = BTN_EVENT_LONG_PRESS;
                btn->cnt = 0;
            }
            break;
            
        case STATE_WAIT_DOUBLE:
            btn->cnt++;
            if (pressed) {
                // Second click detected!
                btn->state = STATE_DOUBLE_CLICK;
                btn->cnt = 0;
            } else if (btn->cnt >= BTN_DOUBLE_TIME) {
                // Timeout - was just a single click
                btn->state = STATE_IDLE;
                btn->value = BTN_EVENT_SHORT_CLICK;
                btn->cnt = 0;
            }
            break;
            
        case STATE_DOUBLE_CLICK:
            if (released) {
                // Double click complete
                btn->state = STATE_IDLE;
                btn->value = BTN_EVENT_DOUBLE_CLICK;
                btn->cnt = 0;
            }
            break;
            
        default:
            btn->state = STATE_IDLE;
            btn->cnt = 0;
            break;
    }
}

ButtonEvent_t Button_GetEvent(Button_t *btn) {
    ButtonEvent_t event = (ButtonEvent_t)btn->value;
    btn->value = BTN_EVENT_NONE;  // Clear after reading
    return event;
}

bool Button_IsPressed(Button_t *btn) {
    return (btn->raw_state == 0);  // Active low logic (0 = pressed)
}