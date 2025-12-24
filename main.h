/**
 * @file main.h
 * @brief MSPM0G3507 Synthesizer - CORRECTED VERSION
 * @version 4.0.0 - With LCD Support
 * 
 * FIXES:
 * - Removed duplicate variable declarations
 * - Corrected LCD function names
 * - Simplified to match actual implementation
 * 
 * @date 2025-12-23
 */

 #define GLOBAL_IQ 24

#ifndef MAIN_H_
#define MAIN_H_

 #include <ti/iqmath/include/IQmathLib.h>
#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

//=============================================================================
// AUDIO CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ          20000.0    // 8 kHz sampling
#define PWM_RESOLUTION          4096.0    // 12-bit PWM
#define PWM_CENTER              2048.0    // Center value (silence)
#define WAVETABLE_SIZE          256       // Power of 2 for efficient indexing

// Frequency limits (in Hz)
#define FREQ_MIN_HZ             100.0
#define FREQ_MAX_HZ             10000.0
#define FREQ_DEFAULT_HZ         440.0
#define VOLUME_DEFAULT          50


//=============================================================================
// WAVEFORM DEFINITIONS
//=============================================================================
typedef enum {
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_SAWTOOTH,
    WAVE_TRIANGLE,
    WAVE_COUNT
} Waveform_t;

//=============================================================================
// MODE DEFINITIONS
//=============================================================================
typedef enum {
    MODE_SYNTH = 0,
    MODE_THEREMIN
} SynthMode_t;

//=============================================================================
// SYSTEM STATE STRUCTURE
//=============================================================================
typedef struct {
    // Audio parameters
    float frequency;              // Current frequency (Hz)
    uint32_t phase_increment;     // DDS phase increment
    
    // Volume control (0-100%)
    uint8_t volume;
    
    // Waveform selection
    Waveform_t waveform;
    
    // Operating mode
    SynthMode_t mode;
    
    // Audio engine state
    volatile bool audio_playing;
    volatile uint32_t phase_accumulator;
    
    // Input readings (12-bit ADC values)
    volatile uint16_t joy_x;         // Joystick X-axis (frequency control)
    volatile uint16_t joy_y;         // Joystick Y-axis (volume control)
    volatile uint16_t mic_level;     // Microphone input
    volatile int16_t accel_x;        // Accelerometer X
    volatile int16_t accel_y;        // Accelerometer Y
    volatile int16_t accel_z;        // Accelerometer Z
    
    // Button states (debounced)
    volatile uint8_t btn_s1;
    volatile uint8_t btn_s2;
    volatile uint8_t joy_pressed;
    
    // Display update flags
    volatile bool display_update_needed;
    volatile bool force_full_redraw;
    
    // System diagnostics
    volatile uint32_t audio_samples_generated;
    volatile uint32_t cpu_idle_count;
    volatile uint32_t interrupt_count;
    
    // Interrupt counters (for debugging)
    volatile uint32_t adc0_count;    // ADC0 interrupt count
    volatile uint32_t adc1_count;    // ADC1 interrupt count
    volatile uint32_t timer_count;   // Timer interrupt count
    
} SynthState_t;

//=============================================================================
// GLOBAL STATE (extern, defined in main.c)
//=============================================================================
extern volatile SynthState_t gSynthState;

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================

// Input processing (called from main loop)
// Note: Process_Input is static (internal to main.c), so not declared here


//=============================================================================
// UTILITY MACROS
//=============================================================================

/**
 * @brief Simple delay (blocking)
 * @param cycles Number of CPU cycles to delay
 * @note At 32MHz, 32000 cycles = 1ms
 */

/**
 * @brief Delay in milliseconds
 * @param ms Milliseconds to delay
 * @note Assumes 32MHz CPU clock
 */
#define LCD_DELAY_MS(ms) delay_cycles((uint32_t)((ms) * 32000UL))

//=============================================================================
// INLINE HELPER FUNCTIONS
//=============================================================================

/**
 * @brief Map a value from one range to another
 * @param value Input value
 * @param in_min Input range minimum
 * @param in_max Input range maximum
 * @param out_min Output range minimum
 * @param out_max Output range maximum
 * @return Mapped value in output range
 */
static inline uint16_t Map_Range(uint16_t value, uint16_t in_min, uint16_t in_max,
                                  uint16_t out_min, uint16_t out_max) {
    if (value <= in_min) return out_min;
    if (value >= in_max) return out_max;
    
    uint32_t range_in = in_max - in_min;
    uint32_t range_out = out_max - out_min;
    uint32_t scaled = ((uint32_t)(value - in_min) * range_out) / range_in;
    
    return out_min + (uint16_t)scaled;
}

/**
 * @brief Convert 12-bit ADC value to frequency
 * @param adc_value ADC reading (0-4095)
 * @return Frequency in Hz (FREQ_MIN_HZ to FREQ_MAX_HZ)
 */
static inline float ADC_To_Frequency(uint16_t adc_value) {
    // Map 0-4095 to FREQ_MIN_HZ - FREQ_MAX_HZ
    uint32_t freq_int = (uint32_t)FREQ_MIN_HZ + 
                        ((adc_value * (uint32_t)(FREQ_MAX_HZ - FREQ_MIN_HZ)) / 4095);
    return (float)freq_int;
}

/**
 * @brief Convert 12-bit ADC value to volume percentage
 * @param adc_value ADC reading (0-4095)
 * @return Volume (0-100%)
 */
static inline uint8_t ADC_To_Volume(uint16_t adc_value) {
    return (uint8_t)((adc_value * 100) / 4095);
}

/**
 * @brief Enter low-power sleep mode
 * @note CPU sleeps until next interrupt (WFI instruction)
 */
static inline void System_Sleep(void) {
    __WFI();  // Wait For Interrupt
}

/**
 * @brief Disable interrupts (enter critical section)
 * @return Previous interrupt state (for restoration)
 */
static inline uint32_t Critical_Enter(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/**
 * @brief Restore interrupts (exit critical section)
 * @param primask Previous interrupt state from Critical_Enter()
 */
static inline void Critical_Exit(uint32_t primask) {
    __set_PRIMASK(primask);
}

//=============================================================================
// DEBUG MACROS (Compile-time configurable)
//=============================================================================
#ifdef DEBUG_MODE
    #define DEBUG_PIN_TOGGLE()  DL_GPIO_togglePins(GPIO_RGB_BLUE_PORT, GPIO_RGB_BLUE_PIN)
    #define DEBUG_PRINT(...)    printf(__VA_ARGS__)
#else
    #define DEBUG_PIN_TOGGLE()  /* NOP */
    #define DEBUG_PRINT(...)    /* NOP */
#endif

#endif /* MAIN_H_ */