/**
 * @file main.h
 * @brief Main definitions for MSPM0G3507 Synthesizer
 * 
 * Global types, constants, and state structures
 * 
 * @version 1.1.0
 * @date 2025-12-17
 * @note Fixed volatile declarations and added LCD positioning constants
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// SYSTEM CONSTANTS
//=============================================================================

#define SYSCLK_FREQUENCY    80000000UL  // 80 MHz
#define SAMPLE_RATE         8000        // 8 kHz audio
#define SENSOR_UPDATE_HZ    20          // 20 Hz sensor polling

//=============================================================================
// LCD LAYOUT CONSTANTS (NEW)
//=============================================================================

#define LCD_MARGIN_LEFT     5
#define LCD_MARGIN_RIGHT    5
#define LCD_LINE_HEIGHT     20
#define LCD_CHAR_HEIGHT     8

// Y-positions for display elements
#define LCD_Y_TITLE         5
#define LCD_Y_FREQ          25
#define LCD_Y_WAVEFORM      45
#define LCD_Y_VOLUME        95
#define LCD_Y_VOLUME_BAR    105
#define LCD_Y_STATUS        130
#define LCD_Y_HELP1         145
#define LCD_Y_HELP2         155

//=============================================================================
// SYNTHESIZER PARAMETERS
//=============================================================================

// Frequency range
#define FREQ_MIN            20.0f       // 20 Hz
#define FREQ_MAX            2000.0f     // 2 kHz
#define FREQ_DEFAULT        440.0f      // A4 note

// Volume range
#define VOLUME_MIN          0
#define VOLUME_MAX          100
#define VOLUME_DEFAULT      50

// Pitch bend range (semitones)
#define PITCH_BEND_RANGE    24          // ±2 octaves

//=============================================================================
// ADC CONSTANTS
//=============================================================================

// Joystick ADC values
#define JOY_ADC_MIN         0
#define JOY_ADC_MAX         4095
#define JOY_ADC_CENTER      2048
#define JOY_DEADZONE        200

// Accelerometer ADC values
#define ACCEL_ZERO_G        2048
#define ACCEL_1G_VALUE      819         // ADC counts per g

//=============================================================================
// TIMING CONSTANTS (NEW)
//=============================================================================

#define TIMER_MAX_VALUE     UINT32_MAX
#define DISPLAY_UPDATE_HZ   10

//=============================================================================
// ENUMERATIONS
//=============================================================================

/**
 * @brief Waveform types
 */
typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_SAWTOOTH,
    WAVE_TRIANGLE,
    WAVE_COUNT
} Waveform_t;

/**
 * @brief Operating modes
 */
typedef enum {
    MODE_SYNTH = 0,
    MODE_MICROPHONE,
    MODE_COUNT
} Mode_t;

//=============================================================================
// GLOBAL STATE STRUCTURE
//=============================================================================

/**
 * @brief Global synthesizer state
 * 
 * @note Variables modified in ISRs are marked as volatile to prevent
 *       compiler optimization issues
 */
typedef struct {
    // === Audio Parameters ===
    Waveform_t waveform;        // Current waveform
    Mode_t mode;                // Operating mode
    float frequency;            // Frequency in Hz
    uint8_t volume;             // Volume 0-100
    int8_t pitchBend;           // Pitch bend in semitones
    volatile bool audio_playing;         // Audio enabled flag (modified in ISR)
    
    // === UI State ===
    volatile bool display_update_needed; // Flag to update LCD (modified in ISR)
    
    // === Sensor Values (Raw ADC) - Modified in ISRs ===
    volatile uint16_t joy_x;             // Joystick X axis (0-4095)
    volatile uint16_t joy_y;             // Joystick Y axis (0-4095)
    volatile bool joy_pressed;           // Joystick button pressed
    
    volatile bool btn_s1;                // S1 button pressed
    volatile bool btn_s2;                // S2 button pressed
    
    volatile uint16_t accel_x;           // Accelerometer X (0-4095)
    volatile uint16_t accel_y;           // Accelerometer Y (0-4095)
    volatile uint16_t accel_z;           // Accelerometer Z (0-4095)
    
    volatile uint16_t mic_level;         // Microphone level (0-4095)
    float light_lux;                     // Light sensor (lux)
} SynthState_t;

//=============================================================================
// GLOBAL VARIABLES
//=============================================================================

extern SynthState_t g_synthState;

//=============================================================================
// UTILITY MACROS
//=============================================================================

#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max)  (MIN(MAX((x), (min)), (max)))
#define ABS(x)              ((x) < 0 ? -(x) : (x))

//=============================================================================
// TIMING UTILITY MACROS (NEW)
//=============================================================================

/**
 * @brief Calculate elapsed time handling timer wrap-around
 * @param now Current timer value
 * @param start Starting timer value
 * @return Elapsed ticks (handles wrap-around)
 */
#define TIMER_ELAPSED(now, start) \
    ((now) >= (start) ? ((now) - (start)) : (TIMER_MAX_VALUE - (start) + (now)))

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================

void delay_ms(uint32_t milliseconds);
void delay_us(uint32_t microseconds);

#endif /* MAIN_H_ */
