/**
 * @file main.h
 * @brief Header for v28 Single-File Synthesizer
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
#include <stdbool.h>
#include "lib/audio/audio_engine.h"
#include "ti_msp_dl_config.h"

//=============================================================================
// MODE TYPES
//=============================================================================
typedef enum {
    MODE_SYNTH = 0,
    MODE_VOCODER,
    MODE_EFFECTS
} SynthMode_t;

//=============================================================================
// SYNTH STATE
//=============================================================================
typedef struct {
    // Audio state
    float frequency;
    uint32_t phase_increment;
    uint8_t volume;
    Waveform_t waveform;
    SynthMode_t mode;
    uint8_t audio_playing;
    uint32_t phase_accumulator;
    
    // Input sensors
    uint16_t joy_x;
    uint16_t joy_y;
    uint16_t mic_level;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    
    // Buttons
    uint8_t btn_s1_mkii;
    uint8_t btn_s2_mkii;
    uint8_t joy_pressed;
    
    // Display flags
    uint8_t display_update_needed;
    uint8_t force_full_redraw;
    
    // Statistics
    uint32_t audio_samples_generated;
    uint32_t cpu_idle_count;
    uint32_t interrupt_count;
    uint32_t adc0_count;
    uint32_t adc1_count;
    uint32_t timer_count;
} SynthState_t;

//=============================================================================
// GLOBAL STATE DECLARATION
//=============================================================================
extern volatile SynthState_t gSynthState;

//=============================================================================
// FREQUENCY LIMITS
//=============================================================================
#define FREQ_MIN_HZ 20
#define FREQ_MAX_HZ 8000

#endif /* MAIN_H_ */