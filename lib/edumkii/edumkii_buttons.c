//=============================================================================
// FILE 2: edumkii_buttons.c
//=============================================================================

#include "edumkii_buttons.h"
#include "ti_msp_dl_config.h"

#define BTN_SHORT_TIME  20
#define BTN_LONG_TIME   50
#define BTN_DOUBLE_TIME 50

// Internal states
typedef enum {
    STATE_IDLE = 0,
    STATE_FIRST_CLICK,
    STATE_WAIT_DOUBLE,
    STATE_DOUBLE_CLICK
} ButtonState_t;

void Button_Init(Button_t *btn) {
    btn->state = STATE_IDLE;
    btn->value = BTN_EVENT_NONE;
    btn->cnt = 0;
    btn->raw_state = 0;
    btn->last_raw_state = 0;
}

void Button_Update(Button_t *btn, GPIO_Regs *gpio_port, uint32_t gpio_pin) {
    // Read GPIO
    btn->raw_state = (DL_GPIO_readPins(gpio_port, gpio_pin) == 0) ? 0 : 1;
    
    uint8_t pressed = (btn->last_raw_state == 1) && (btn->raw_state == 0);
    uint8_t released = (btn->last_raw_state == 0) && (btn->raw_state == 1);
    btn->last_raw_state = btn->raw_state;
    
    // State machine (your existing code)
    switch (btn->state) {
        case STATE_IDLE:
            if (pressed) {
                btn->state = STATE_FIRST_CLICK;
                btn->cnt = 0;
                btn->value = BTN_EVENT_NONE;
            }
            break;
        // ... rest of state machine ...
    }
}

ButtonEvent_t Button_GetEvent(Button_t *btn) {
    ButtonEvent_t event = (ButtonEvent_t)btn->value;
    btn->value = BTN_EVENT_NONE;
    return event;
}
