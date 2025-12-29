/**
 * @file audio_filters.c
 * @brief Audio Filters Implementation
 */

#include "audio_filters.h"

//=============================================================================
// INTERNAL STATE
//=============================================================================
static int16_t prev_sample = 0;
static int16_t prev_input = 0;

//=============================================================================
// FILTERS
//=============================================================================

int16_t Filter_LowPass(int16_t new_sample) {
    // Simple 50/50 mix
    int16_t filtered = (prev_sample + new_sample) / 2;
    prev_sample = filtered;
    return filtered;
}

int16_t Filter_LowPassAlpha(int16_t new_sample, uint8_t alpha) {
    // filtered = (prev * alpha + new * (256-alpha)) / 256
    int32_t filtered = ((int32_t)prev_sample * alpha + 
                       (int32_t)new_sample * (256 - alpha)) / 256;
    prev_sample = (int16_t)filtered;
    return (int16_t)filtered;
}

int16_t Filter_HighPass(int16_t new_sample) {
    // High-pass = input - low-pass
    int16_t low_pass = (prev_sample + new_sample) / 2;
    int16_t high_pass = new_sample - low_pass;
    prev_sample = low_pass;
    return high_pass;
}

void Filter_Reset(void) {
    prev_sample = 0;
    prev_input = 0;
}

//=============================================================================
// EFFECTS
//=============================================================================

int16_t Filter_SoftClip(int16_t sample, int16_t threshold) {
    if (sample > threshold) {
        // Gentle compression above threshold
        sample = threshold + ((sample - threshold) / 2);
        if (sample > 2000) sample = 2000;  // Hard limit
    } else if (sample < -threshold) {
        // Gentle compression below threshold
        sample = -threshold + ((sample + threshold) / 4);
        if (sample < -2000) sample = -2000;  // Hard limit
    }
    return sample;
}

int16_t Filter_HardClip(int16_t sample, int16_t limit) {
    if (sample > limit) return limit;
    if (sample < -limit) return -limit;
    return sample;
}

int16_t Filter_GainWithFreqCompensation(int16_t sample, uint8_t gain, uint32_t frequency_hz) {
    // Reduce gain for very low frequencies to prevent clipping
    uint8_t adjusted_gain = gain;
    
    if (frequency_hz < 200) {
        // Very low: reduce gain significantly
        adjusted_gain = gain / 2;
    } else if (frequency_hz < 400) {
        // Low: reduce gain moderately
        adjusted_gain = (gain * 3) / 4;
    }
    
    return sample * adjusted_gain;
}

//=============================================================================
// UTILITIES
//=============================================================================

uint16_t Audio_SampleToPWM(int16_t sample, uint16_t pwm_center, uint16_t pwm_max) {
    // Scale sample to PWM range
    int32_t val = pwm_center + (sample * 2);
    
    // Clamp to valid range
    if (val < 0) val = 0;
    if (val > pwm_max) val = pwm_max;
    
    return (uint16_t)val;
}