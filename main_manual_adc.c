/**
 * @file main_manual_adc.c
 * @brief Manual ADC read test - no interrupts
 */

#include <stdint.h>
#include "ti_msp_dl_config.h"

int main(void) {
    // Init
    SYSCFG_DL_init();
    
    // Enable ADC
    DL_ADC12_enableConversions(ADC_MIC_JOY_INST);
    
    // Main loop
    while (1) {
        // Start conversion
        DL_ADC12_startConversion(ADC_MIC_JOY_INST);
        
        // Wait for conversion (busy wait)
        while (!DL_ADC12_isConversionStarted(ADC_MIC_JOY_INST)) {
            // Wait for SC bit to set
        }
        
        // Wait a bit for conversion to complete
        for (volatile uint32_t i = 0; i < 1000; i++);
        
        // Read result manually
        uint16_t result = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
        
        // Breakpoint here - check 'result'
        // Should NOT be 0!
        
        // Delay before next read
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}
