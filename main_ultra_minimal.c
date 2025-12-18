**
 * DIAGNOSTIC VERSION - Minimal Test
 * Test hver del systematisk
 */

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "lcd/lcd_driver.h"

// Test 1: Blink LED
void test_led_blink(void) {
    for(int i = 0; i < 10; i++) {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        for(volatile uint32_t d = 0; d < 1000000; d++);
    }
}

// Test 2: LCD Test
void test_lcd(void) {
    LCD_Init();
    LCD_Clear(COLOR_BLACK);
    LCD_DrawString(10, 10, "LCD TEST", COLOR_WHITE);
    LCD_DrawString(10, 30, "Line 2", COLOR_RED);
    LCD_DrawString(10, 50, "Line 3", COLOR_GREEN);
    LCD_FillRect(10, 70, 50, 20, COLOR_BLUE);
}

// Test 3: Button Test (polling)
void test_buttons(void) {
    while(1) {
        // Read S1 button (active low with pull-up)
        if(DL_GPIO_readPins(GPIOA, GPIO_BUTTONS_S1_PIN) == 0) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        }
        
        // Read S2 button
        if(DL_GPIO_readPins(GPIOA, GPIO_BUTTONS_S2_PIN) == 0) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        }
        
        for(volatile uint32_t d = 0; d < 10000; d++);
    }
}

// Test 4: PWM Audio Test (simple tone)
void test_audio(void) {
    // Set 50% duty cycle
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
    
    // Blink LED to show we're here
    for(int i = 0; i < 5; i++) {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
        for(volatile uint32_t d = 0; d < 1000000; d++);
    }
}

int main(void) {
    SYSCFG_DL_init();
    
    // Wait a bit for power to stabilize
    for(volatile uint32_t d = 0; d < 1000000; d++);
    
    // TEST 1: LED Blink (simplest test)
    // Uncomment ONE test at a time:
    
    test_led_blink();     // ← Start med denne først!
    
    // test_lcd();        // ← Test LCD etter LED virker
    
    // test_buttons();    // ← Test buttons etter LCD virker
    
    // test_audio();      // ← Test audio til slutt
    
    while(1) {
        // Keep LED blinking to show we're alive
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        for(volatile uint32_t d = 0; d < 1000000; d++);
    }
}