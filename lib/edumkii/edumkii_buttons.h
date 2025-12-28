//=============================================================================
// FILE 1: edumkii_buttons.h
//=============================================================================

#ifndef EDUMKII_BUTTONS_H_
#define EDUMKII_BUTTONS_H_

#include <stdint.h>
#include <stdbool.h>
#include <ti/driverlib/dl_gpio.h>
// Button event types
typedef enum {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_SHORT_CLICK,
    BTN_EVENT_LONG_PRESS,
    BTN_EVENT_DOUBLE_CLICK
} ButtonEvent_t;

// Button state machine (opaque to user)
typedef struct {
    uint8_t state;
    uint8_t value;
    uint16_t cnt;
    uint8_t raw_state;
    uint8_t last_raw_state;
} Button_t;

// Public API
void Button_Init(Button_t *btn);
void Button_Update(Button_t *btn, GPIO_Regs *gpio_port, uint32_t gpio_pin);
ButtonEvent_t Button_GetEvent(Button_t *btn);

#endif
