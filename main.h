/*
 * main.h
 */

#ifndef MAIN_H_
#define MAIN_H_

// --- Includes ---
#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

//=============================================================================
// AUDIO CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ          16000
#define PWM_RESOLUTION          4096
#define PWM_CENTER              2048
#define WAVETABLE_SIZE          256

#define FREQ_MIN_HZ             20
#define FREQ_MAX_HZ             8000
#define FREQ_DEFAULT_HZ         440
#define VOLUME_DEFAULT          80

//=============================================================================
// ENUMS (✅ DISSE MÅ KOMME FØRST!)
//=============================================================================
typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_SAWTOOTH,
    WAVE_TRIANGLE,
    WAVE_COUNT
} Waveform_t;

typedef enum {
    MODE_SYNTH = 0,
    MODE_THEREMIN,
    MODE_DRUMS,
    MODE_COUNT
} SynthMode_t;

//=============================================================================
// SYSTEM STATE STRUCTURE (✅ NÅ KAN DEN BRUKE Waveform_t!)
//=============================================================================
typedef struct {
    // Audio State
    float frequency;
    uint32_t phase_increment;
    uint8_t volume;
    Waveform_t waveform;           // ← Nå er Waveform_t definert!
    SynthMode_t mode;              // ← Nå er SynthMode_t definert!
    volatile bool audio_playing;
    volatile uint32_t phase_accumulator;
    
    // Inputs
    volatile uint16_t joy_x;
    volatile uint16_t joy_y;
    volatile uint16_t mic_level;
    volatile int16_t accel_x;
    volatile int16_t accel_y;
    volatile int16_t accel_z;
    
    // Buttons
    volatile uint8_t btn_s1_mkii;
    volatile uint8_t btn_s2_mkii;
    volatile uint8_t joy_pressed;
    
    // Display flags
    volatile bool display_update_needed;
    volatile bool force_full_redraw;
    
    // Debug / Counters
    volatile uint32_t audio_samples_generated;
    volatile uint32_t cpu_idle_count;
    volatile uint32_t interrupt_count;
    volatile uint32_t adc0_count;
    volatile uint32_t adc1_count;
    volatile uint32_t timer_count;
    
} SynthState_t;

//=============================================================================
// GLOBAL VARIABLES (EXTERN)
//=============================================================================
extern volatile SynthState_t gSynthState;
extern volatile uint32_t g_phase_increment;  
//=============================================================================
// PUBLIC FUNCTION PROTOTYPES
//=============================================================================
void Change_Instrument(void);
void Change_Preset(void);
void Trigger_Note_On(void);
void Trigger_Note_Off(void);

#endif /* MAIN_H_ */