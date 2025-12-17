/**
 * @file main.h
 * @brief Main definitions for MSPM0G3507 Synthesizer
 * 
 * Global types, constants, and state structures
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
 */
typedef struct {
    // === Audio Parameters ===
    Waveform_t waveform;        // Current waveform
    Mode_t mode;                // Operating mode
    float frequency;            // Frequency in Hz
    uint8_t volume;             // Volume 0-100
    int8_t pitchBend;           // Pitch bend in semitones
    bool audio_playing;         // Audio enabled flag
    
    // === UI State ===
    bool display_update_needed; // Flag to update LCD
    
    // === Sensor Values (Raw ADC) ===
    uint16_t joy_x;             // Joystick X axis (0-4095)
    uint16_t joy_y;             // Joystick Y axis (0-4095)
    bool joy_pressed;           // Joystick button pressed
    
    bool btn_s1;                // S1 button pressed
    bool btn_s2;                // S2 button pressed
    
    uint16_t accel_x;           // Accelerometer X (0-4095)
    uint16_t accel_y;           // Accelerometer Y (0-4095)
    uint16_t accel_z;           // Accelerometer Z (0-4095)
    
    uint16_t mic_level;         // Microphone level (0-4095)
    float light_lux;            // Light sensor (lux)
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
// FUNCTION PROTOTYPES
//=============================================================================

void delay_ms(uint32_t milliseconds);
void delay_us(uint32_t microseconds);

#endif /* MAIN_H_ */
