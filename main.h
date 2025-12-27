#ifndef MAIN_H_
#define MAIN_H_

#define GLOBAL_IQ 24

#include <ti/iqmath/include/IQmathLib.h>
#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

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
// AUDIO CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ          8000.0
#define PWM_RESOLUTION          4096.0
#define PWM_CENTER              2048.0
#define WAVETABLE_SIZE          256

#define FREQ_MIN_HZ             20.0
#define FREQ_MAX_HZ             8000.0
#define FREQ_DEFAULT_HZ         440.0
#define VOLUME_DEFAULT          80

//=============================================================================
// MODE DEFINITIONS
//=============================================================================
typedef enum {
    MODE_SYNTH = 0,
    MODE_THEREMIN,
    MODE_DRUMS,
    MODE_COUNT
} SynthMode_t;

//=============================================================================
// SYSTEM STATE STRUCTURE
//=============================================================================
typedef struct {
    float frequency;
    uint32_t phase_increment;
    uint8_t volume;
    Waveform_t waveform;
    SynthMode_t mode;
    volatile bool audio_playing;
    volatile uint32_t phase_accumulator;
    
    volatile uint16_t joy_x;
    volatile uint16_t joy_y;
    volatile uint16_t mic_level;
    volatile int16_t accel_x;
    volatile int16_t accel_y;
    volatile int16_t accel_z;
    
    volatile uint8_t btn_s1;
    volatile uint8_t btn_s2;
    volatile uint8_t joy_pressed;
    
    volatile bool display_update_needed;
    volatile bool force_full_redraw;
    
    volatile uint32_t audio_samples_generated;
    volatile uint32_t cpu_idle_count;
    volatile uint32_t interrupt_count;
    
    volatile uint32_t adc0_count;
    volatile uint32_t adc1_count;
    volatile uint32_t timer_count;
} SynthState_t;

//=============================================================================
// GLOBAL STATE (extern)
//=============================================================================
extern volatile SynthState_t gSynthState;

#endif /* MAIN_H_ */