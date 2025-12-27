/*
 * buttons.c - SAFE VERSION
 */

#include "buttons.h"
#include "main.h"

// Initialisering
void Buttons_Init(void) {
    DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, 
        GPIO_BUTTONS_S1_MKII_PIN | GPIO_BUTTONS_S2_MKII_PIN);
    
    DL_GPIO_enableInterrupt(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_MKII_PIN);
    DL_GPIO_enableInterrupt(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_MKII_PIN);
    
    NVIC_ClearPendingIRQ(GPIOA_INT_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
}

// ✅ TRYGG: Sett bare flagg, gjør arbeid i main loop
void Handle_GPIO_Interrupt(void) {
    static uint32_t last_s1_mkii = 0;
    static uint32_t last_s2_mkii = 0;
    uint32_t now = g_system_ticks;
    
    uint32_t pending = DL_GPIO_getEnabledInterruptStatus(
        GPIO_BUTTONS_PORT, 
        GPIO_BUTTONS_S1_MKII_PIN | GPIO_BUTTONS_S2_MKII_PIN
    );

    // S1 - Debounce 100ms
    if ((pending & GPIO_BUTTONS_S1_MKII_PIN) && (now - last_s1_mkii > 800)) {
        last_s1_mkii = now;
        gSynthState.btn_s1_mkii++;  // ← BARE SETT FLAGG!
        DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_MKII_PIN);
    }

    // S2 - Debounce 100ms
    if ((pending & GPIO_BUTTONS_S2_MKII_PIN) && (now - last_s2_mkii > 800)) {
        last_s2_mkii = now;
        gSynthState.btn_s2_mkii++;  // ← BARE SETT FLAGG!
        DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_MKII_PIN);
    }
    
    // Safety
    DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, 0xFFFFFFFF);
}