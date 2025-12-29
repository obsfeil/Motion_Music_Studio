/**
 * @file audio_engine.c
 * @brief Audio Synthesis Engine Implementation
 */

#include "audio_engine.h"

//=============================================================================
// SINE WAVETABLE (256 samples, amplitude ±1000)
//=============================================================================
static const int16_t sine_table[256] = {
    0,    25,   49,   74,   98,   122,  147,  171,  195,  219,  243,  267,
    290,  314,  337,  360,  383,  405,  428,  450,  471,  493,  514,  535,
    555,  575,  595,  614,  633,  652,  670,  687,  704,  721,  737,  753,
    768,  783,  797,  811,  824,  837,  849,  860,  871,  882,  892,  901,
    910,  918,  926,  933,  939,  945,  951,  955,  960,  963,  966,  969,
    971,  972,  973,  974,  974,  973,  972,  971,  969,  966,  963,  960,
    955,  951,  945,  939,  933,  926,  918,  910,  901,  892,  882,  871,
    860,  849,  837,  824,  811,  797,  783,  768,  753,  737,  721,  704,
    687,  670,  652,  633,  614,  595,  575,  555,  535,  514,  493,  471,
    450,  428,  405,  383,  360,  337,  314,  290,  267,  243,  219,  195,
    171,  147,  122,  98,   74,   49,   25,   0,    -25,  -49,  -74,  -98,
    -122, -147, -171, -195, -219, -243, -267, -290, -314, -337, -360, -383,
    -405, -428, -450, -471, -493, -514, -535, -555, -575, -595, -614, -633,
    -652, -670, -687, -704, -721, -737, -753, -768, -783, -797, -811, -824,
    -837, -849, -860, -871, -882, -892, -901, -910, -918, -926, -933, -939,
    -945, -951, -955, -960, -963, -966, -969, -971, -972, -973, -974, -974,
    -973, -972, -971, -969, -966, -963, -960, -955, -951, -945, -939, -933,
    -926, -918, -910, -901, -892, -882, -871, -860, -849, -837, -824, -811,
    -797, -783, -768, -753, -737, -721, -704, -687, -670, -652, -633, -614,
    -595, -575, -555, -535, -514, -493, -471, -450, -428, -405, -383, -360,
    -337, -314, -290, -267, -243, -219, -195, -171, -147, -122, -98,  -74,
    -49,  -25
};

//=============================================================================
// INTERNAL STATE
//=============================================================================
static uint32_t phase_accumulator = 0;
static uint32_t phase_increment = 118111601;  // 440 Hz @ 8 kHz
static uint16_t sample_rate = 8000;
static Waveform_t current_waveform = WAVE_SINE;

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void Audio_Init(uint16_t sample_rate_hz) {
    sample_rate = sample_rate_hz;
    phase_accumulator = 0;
    phase_increment = 118111601;
    current_waveform = WAVE_SINE;
}

void Audio_SetFrequency(uint32_t frequency_hz) {
    // Clamp frequency
    if (frequency_hz < 20) frequency_hz = 20;
    if (frequency_hz > 8000) frequency_hz = 8000;
    
    // Calculate phase increment: (frequency * 2^32) / sample_rate
    uint64_t temp = ((uint64_t)frequency_hz << 32) / sample_rate;
    phase_increment = (uint32_t)temp;
    
    // Safety check
    if (phase_increment == 0) {
        phase_increment = 118111601;  // Fallback to 440 Hz
    }
}

void Audio_SetWaveform(Waveform_t waveform) {
    if (waveform < WAVE_COUNT) {
        current_waveform = waveform;
    }
}


int16_t Audio_GenerateWaveform(uint8_t index, Waveform_t waveform) {
    switch (waveform) {
        case WAVE_SINE:
            return sine_table[index];
            
        case WAVE_SQUARE:
            // Square wave with soft edges
            if (index < 118)
                return 900;
            if (index < 138)
                return 900 - (int16_t)(((index - 118) * 1800) / 20);
            return -900;
            
        case WAVE_SAWTOOTH:
            // Linear ramp
            return (int16_t)(((int32_t)index * 1800 / 256) - 900);
            
        case WAVE_TRIANGLE:
            // Triangle wave
            if (index < 128)
                return (int16_t)(((int32_t)index * 1800 / 128) - 900);
            return (int16_t)(900 - ((int32_t)(index - 128) * 1800 / 128));
            
        default:
            return sine_table[index];
    }
}

int16_t Audio_GenerateSample(void) {
    // Get wavetable index from top 8 bits of phase
    uint8_t index = (uint8_t)((phase_accumulator >> 24) & 0xFF);
    
    // Generate waveform
    int16_t sample = Audio_GenerateWaveform(index, current_waveform);
    
    // Advance phase
    phase_accumulator += phase_increment;
    
    return sample;
}


uint32_t Audio_GetPhaseIncrement(void) {
    return phase_increment;
}

const int16_t* Audio_GetSineTable(void) {
    return sine_table;
}

void Audio_ResetPhase(void) {
    phase_accumulator = 0;
}
