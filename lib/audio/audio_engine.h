//=============================================================================
// FILE 7: audio_engine.h (Reusable audio)
//=============================================================================

#ifndef AUDIO_ENGINE_H_
#define AUDIO_ENGINE_H_

#include <stdint.h>

typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_SAWTOOTH,
    WAVE_TRIANGLE
} Waveform_t;

// Public API
void Audio_Init(uint16_t sample_rate_hz);
int16_t Audio_GenerateWaveform(uint8_t index, Waveform_t waveform);
void Audio_SetFrequency(uint32_t frequency_hz);
uint32_t Audio_GetPhaseIncrement(void);

#endif