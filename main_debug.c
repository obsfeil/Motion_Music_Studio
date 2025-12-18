/**
 * @file main_debug.c
 * @brief Debug version for testing ADC
 * 
 * Bruk denne for å teste om ADC fungerer:
 * 1. Bygg prosjektet
 * 2. Debug (F11)
 * 3. Sett breakpoint i main loop
 * 4. Sjekk at adc0_interrupt_count øker
 * 5. Sjekk at test_adc0_memX har verdier
 */

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

// Test variables - watch these in debugger
volatile uint16_t test_adc0_mem0 = 0;
volatile uint16_t test_adc0_mem1 = 0;
volatile uint16_t test_adc0_mem2 = 0;
volatile uint32_t adc0_interrupt_count = 0;

/**
 * @brief ADC0 Interrupt Handler
 */
void ADC0_IRQHandler(void) {
    adc0_interrupt_count++;  // Count interrupts
    
    switch (DL_ADC12_getPendingInterrupt(ADC_MIC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            test_adc0_mem0 = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
            break;
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            test_adc0_mem1 = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_1);
            break;
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            test_adc0_mem2 = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_2);
            break;
        default:
            break;
    }
}

/**
 * @brief Main program - minimal test
 */
int main(void) {
    // Initialize system
    SYSCFG_DL_init();
    
    // Enable ADC interrupt
    NVIC_EnableIRQ(ADC_MIC_JOY_INST_INT_IRQN);
    __enable_irq();
    
    // Enable and start ADC conversions
    DL_ADC12_enableConversions(ADC_MIC_JOY_INST);     // Enable ADC (power on)
    DL_ADC12_startConversion(ADC_MIC_JOY_INST);       // Start conversions
    
    // Main loop - minimal code
    while (1) {
        // ← SET BREAKPOINT HERE
        // Check these variables in debugger:
        // - adc0_interrupt_count (should increase rapidly)
        // - test_adc0_mem0 (mic value)
        // - test_adc0_mem1 (joystick Y)
        // - test_adc0_mem2 (joystick X)
        
        __WFI();  // Wait for interrupt
    }
}
