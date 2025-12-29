/*
 * main.h
 */
#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

// ✅ INKLUDER BIBLIOTEKENE HER (Dette kobler Waveform_t til main.c)
#include "lib/audio/audio_engine.h"
#include "lib/audio/audio_envelope.h"
#include "lib/edumkii/edumkii_buttons.h"
#include "lib/edumkii/edumkii_joystick.h"
#include "lib/edumkii/edumkii_accel.h"

// System Struct
typedef struct {
    float frequency;
    uint32_t phase_increment;
    uint8_t volume;
    uint8_t audio_playing;
    Waveform_t waveform; // Nå vet den hva dette er!
    
    volatile int16_t joy_x;
    volatile int16_t joy_y;
    volatile int16_t accel_x;
    volatile int16_t accel_y;
    volatile int16_t accel_z;
    
    volatile uint32_t timer_count;
    volatile uint32_t adc0_count;
    volatile uint32_t adc1_count;
    volatile uint32_t audio_samples_generated;
} SynthState_t;

extern volatile SynthState_t gSynthState;

#endif /* MAIN_H_ */