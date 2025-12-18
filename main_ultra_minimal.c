/**
 * @file main_ultra_minimal.c
 * @brief Ultra minimal ADC test - just ADC, nothing else
 */

#include <stdint.h>
#include "ti_msp_dl_config.h"

// Test result variables
volatile uint16_t adc0_result = 0;
volatile uint32_t adc0_count = 0;

void ADC0_IRQHandler(void) {
    adc0_count++;
    
    // Read any result just to test
    adc0_result = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
    
    // Clear interrupt
    DL_ADC12_clearInterruptStatus(ADC_MIC_JOY_INST, 
        DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
}

int main(void) {
    // 1. Init system
    SYSCFG_DL_init();
    
    // 2. Enable interrupt
    NVIC_EnableIRQ(ADC_MIC_JOY_INST_INT_IRQN);
    __enable_irq();
    
    // 3. Enable ADC (CRITICAL!)
    DL_ADC12_enableConversions(ADC_MIC_JOY_INST);
    
    // 4. Start conversion
    DL_ADC12_startConversion(ADC_MIC_JOY_INST);
    
    // 5. Loop forever
    while (1) {
        // Just wait - interrupts should fire
        __WFI();
        
        // Breakpoint here and check:
        // - adc0_count (should increase)
        // - adc0_result (should have value)
    }
}
