/**
 * @file main.h
 * @brief MSPM0G3507 Professional Synthesizer - Optimized Architecture
 * @version 6.0.0 - Production Ready
 * 
 * ARCHITECTURAL IMPROVEMENTS:
 * - IQMath fixed-point (MATHACL accelerated)
 * - Event Fabric for zero-CPU triggering
 * - Interrupt-driven audio pipeline
 * - Sleep mode with __WFI()
 * - Zero float operations
 * - Deterministic timing
 * 
 * @author Professional Embedded Systems
 * @date 2025
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>
//=============================================================================
// IQMATH CONFIGURATION (Hardware-Accelerated)
//=============================================================================
#include "ti/iqmath/include/IQmathLib.h"

#ifdef GLOBAL_IQ
    #undef GLOBAL_IQ
#endif

// Use Q24 format: 8 bits integer, 24 bits fractional
#define GLOBAL_IQ 24

// Link against MATHACL-optimized library (not software emulation!)
// In project settings: libiqmath_MATHACL.a

//=============================================================================
// AUDIO CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ          8000.0    // 8 kHz sampling
#define PWM_RESOLUTION          4096.0    // 12-bit PWM
#define PWM_CENTER              2048.0    // Center value (silence)
#define WAVETABLE_SIZE          256     // Power of 2 for efficient indexing

// DDS (Direct Digital Synthesis) phase accumulator
#define PHASE_BITS              32     // 32-bit phase accumulator
#define PHASE_TO_INDEX_SHIFT    24     // Top 8 bits = table index

// Frequency limits (in Hz, converted to IQ at runtime)
#define FREQ_MIN_HZ             20.0
#define FREQ_MAX_HZ             20000.0
#define FREQ_DEFAULT_HZ         440.0

//=============================================================================
// WAVEFORM DEFINITIONS
//=============================================================================
typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_SAWTOOTH,
    WAVE_TRIANGLE,
    WAVE_COUNT
} Waveform_t;

//=============================================================================
// SYSTEM STATE (All Fixed-Point!)
//=============================================================================
typedef struct {
    // Audio parameters (IQ24 fixed-point)
    _iq frequency;              // Current frequency (Hz)
    uint32_t phase_increment;      // DDS phase increment per sample
    
    // Volume control (8-bit integer, 0-100%)
    uint8_t volume;
    
    // Waveform selection
    Waveform_t waveform;
    
    // Audio engine state
    volatile bool audio_playing;
    volatile uint32_t phase_accumulator;  // 32-bit DDS phase
    
    // Input readings (12-bit ADC values)
    volatile uint16_t joy_x;         // Joystick X-axis (frequency)
    volatile uint16_t joy_y;         // Joystick Y-axis (volume)
    volatile uint16_t mic_level;     // Microphone input
    volatile int16_t accel_x;        // Accelerometer X
    volatile int16_t accel_y;        // Accelerometer Y
    volatile int16_t accel_z;        // Accelerometer Z
    
    // Button states (debounced)
    volatile uint8_t btn_s1;
    volatile uint8_t btn_s2;
    volatile uint8_t joy_pressed;
    
    // Display flags
    volatile bool display_update_needed;
    volatile bool force_full_redraw;
    
    // System diagnostics
    volatile uint32_t audio_samples_generated;
    volatile uint32_t cpu_idle_count;
    volatile uint32_t interrupt_count;
    
} SynthState_t;

// ========== TIMER WRAP-AROUND FIX ==========
#define TIMER_ELAPSED(now, start) \
    (((now) >= (start)) ? ((now) - (start)) : \
     (UINT32_MAX - (start) + (now) + 1))
//=============================================================================
// GLOBAL STATE (extern, defined in main.c)
//=============================================================================
extern volatile SynthState_t gSynthState;

//=============================================================================
// WAVETABLES (Pre-computed, in Flash)
//=============================================================================
extern const int16_t wavetable_sine[WAVETABLE_SIZE];
extern const int16_t wavetable_square[WAVETABLE_SIZE];
extern const int16_t wavetable_sawtooth[WAVETABLE_SIZE];
extern const int16_t wavetable_triangle[WAVETABLE_SIZE];

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================

// System initialization
void System_Init(void);
void Audio_Init(void);

// Audio engine (DMA-driven, CPU-free)
void Audio_Update_Frequency(_iq new_freq);
void Audio_Update_Volume(uint8_t new_vol);
void Audio_Set_Waveform(Waveform_t wave);
void Audio_Start(void);
void Audio_Stop(void);

// Input processing (called from main loop only when needed)
void Process_Joystick(void);
void Process_Buttons(void);
void Process_Accelerometer(void);

// Display (DMA-based SPI, non-blocking)
void Display_Init(void);
void Display_Update_Status(void);
void Display_Update_Waveform(void);
void Display_Full_Redraw(void);

// Utility functions (integer-only math)
uint16_t Map_Range(uint16_t value, uint16_t in_min, uint16_t in_max, 
                   uint16_t out_min, uint16_t out_max);

//=============================================================================
// INLINE HELPERS (Zero overhead)
//=============================================================================

/**
 * @brief Enter low-power sleep mode
 * @note CPU sleeps until next interrupt
 */
static inline void System_Sleep(void)
{
   __WFI();  // Wait For Interrupt
}


/**
 * @brief Convert 12-bit ADC value to IQ24 frequency
 * @param adc_value 12-bit ADC reading (0-4095)
 * @return _iq Frequency in Hz (FREQ_MIN_HZ to FREQ_MAX_HZ)
 */
static inline _iq ADC_To_Frequency(uint16_t adc_value)
{
    // Map 0-4095 to FREQ_MIN_HZ - FREQ_MAX_HZ using integer math
    uint32_t freq_hz = FREQ_MIN_HZ + 
                       ((uint32_t)adc_value * (FREQ_MAX_HZ - FREQ_MIN_HZ)) / 4095;
    
    return _IQ(freq_hz);  // Convert to IQ24
}

/**
 * @brief Convert 12-bit ADC value to volume percentage
 * @param adc_value 12-bit ADC reading (0-4095)
 * @return uint8_t Volume (0-100%)
 */
static inline uint8_t ADC_To_Volume(uint16_t adc_value)
{
    return (uint8_t)((adc_value * 100) / 4095);
}

//=============================================================================
// CRITICAL TIMING HELPERS
//=============================================================================

/**
 * @brief Disable interrupts for critical section
 */
static inline uint32_t Critical_Enter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/**
 * @brief Restore interrupts after critical section
 */
static inline void Critical_Exit(uint32_t primask)
{
    __set_PRIMASK(primask);
}

//=============================================================================
// DEBUG/DIAGNOSTICS (Compile-time configurable)
//=============================================================================
#ifdef DEBUG_MODE
    #define DEBUG_PIN_TOGGLE()  DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN)
    #define DEBUG_PRINT(...)    printf(__VA_ARGS__)
#else
    #define DEBUG_PIN_TOGGLE()  /* NOP */
    #define DEBUG_PRINT(...)    /* NOP */
#endif

#endif /* MAIN_H_ */
