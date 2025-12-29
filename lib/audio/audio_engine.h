/*
 * lib/audio/audio_engine.h
 */
#ifndef AUDIO_ENGINE_H_
#define AUDIO_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>

// ✅ DETTE ER LØSNINGEN: Definer Waveform_t her!
typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_SAWTOOTH,
    WAVE_TRIANGLE,
    WAVE_COUNT
} Waveform_t;

// API Funksjoner
void Audio_Init(uint16_t sample_rate_hz);
void Audio_SetFrequency(uint32_t frequency_hz);
void Audio_SetWaveform(Waveform_t waveform);
int16_t Audio_GenerateWaveform(uint8_t index, Waveform_t waveform);
const int16_t* Audio_GetSineTable(void);
uint16_t Audio_SampleToPWM(int16_t sample, uint16_t center, uint16_t max);

#endif /* AUDIO_ENGINE_H_ */