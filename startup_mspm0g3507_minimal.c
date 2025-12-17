/*
 * Ultra-Minimal Startup File for MSPM0G3507
 * Relies on TI C Runtime Library for .data/.bss initialization
 * 
 * This startup file ONLY provides:
 * 1. Interrupt vector table
 * 2. Reset handler that calls main()
 * 3. Default interrupt handlers
 */

#include <stdint.h>

// Forward declarations
void ResetISR(void);
void Default_Handler(void);

// Stack (512 bytes, aligned to 8 bytes)
static uint8_t stack[512] __attribute__((aligned(8)));

// Core Cortex-M0+ exceptions
void NMI_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)      __attribute__((weak, alias("Default_Handler")));

// MSPM0G3507 peripheral interrupts (using hardware names from TI documentation)
void GROUP0_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void GPIOA_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));  // GPIO_BUTTONS
void GPIOB_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIMG0_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));  // PWM_AUDIO
void TIMG4_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIMG6_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIMG7_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));  // TIMER_SAMPLE
void TIMG8_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIMG12_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void TIMG14_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void ADC0_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));  // ADC_MIC_JOY
void ADC1_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));  // ADC_ACCEL
void SPI0_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));  // SPI_LCD
void UART0_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void UART1_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void UART2_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void UART3_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C0_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));  // I2C_SENSORS
void I2C1_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C2_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void DMA_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void AES_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void CANFD0_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));

// Interrupt vector table
__attribute__((section(".intvecs")))
void (*const interruptVectors[])(void) = {
    (void (*)(void))((uint32_t)stack + sizeof(stack)),  // 0: Initial stack pointer
    ResetISR,                           // 1: Reset handler
    NMI_Handler,                        // 2: NMI
    HardFault_Handler,                  // 3: Hard fault
    0,                                  // 4: Reserved
    0,                                  // 5: Reserved
    0,                                  // 6: Reserved
    0,                                  // 7: Reserved
    0,                                  // 8: Reserved
    0,                                  // 9: Reserved
    0,                                  // 10: Reserved
    SVC_Handler,                        // 11: SVCall
    0,                                  // 12: Reserved
    0,                                  // 13: Reserved
    PendSV_Handler,                     // 14: PendSV
    SysTick_Handler,                    // 15: SysTick
    
    // External interrupts (16-47)
    GROUP0_IRQHandler,                  // 16
    GPIOA_IRQHandler,                   // 17: GPIO_BUTTONS (GROUP1)
    TIMG0_IRQHandler,                   // 18: PWM_AUDIO
    TIMG4_IRQHandler,                   // 19
    TIMG7_IRQHandler,                   // 20: TIMER_SAMPLE
    TIMG8_IRQHandler,                   // 21
    TIMG12_IRQHandler,                  // 22
    TIMG14_IRQHandler,                  // 23
    ADC0_IRQHandler,                    // 24: ADC_MIC_JOY
    ADC1_IRQHandler,                    // 25: ADC_ACCEL
    0,                                  // 26
    0,                                  // 27
    SPI0_IRQHandler,                    // 28
    SPI1_IRQHandler,                    // 29: SPI_LCD
    0,                                  // 30
    0,                                  // 31
    I2C0_IRQHandler,                    // 32: I2C_SENSORS
    I2C1_IRQHandler,                    // 33
    UART0_IRQHandler,                   // 34
    UART1_IRQHandler,                   // 35
    UART2_IRQHandler,                   // 36
    UART3_IRQHandler,                   // 37
    AES_IRQHandler,                     // 38
    DMA_IRQHandler,                     // 39
    RTC_IRQHandler,                     // 40
    TIMG6_IRQHandler,                   // 41
    GPIOB_IRQHandler,                   // 42
    I2C2_IRQHandler,                    // 43
    0,                                  // 44
    CANFD0_IRQHandler,                  // 45
    0,                                  // 46
    0                                   // 47
};

// Reset handler - minimal version
// TI's C runtime library (libc.a) will handle .data/.bss initialization
void ResetISR(void) {
    // Simply call main - TI libc handles everything else
    extern int main(void);
    main();
    
    // Hang if main returns
    while (1);
}

// Default handler for unhandled interrupts
void Default_Handler(void) {
    // Trap unhandled interrupt in infinite loop
    while (1);
}