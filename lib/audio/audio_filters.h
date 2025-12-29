/**
 * @file audio_filters.h
 * @brief Audio Filters and Effects
 * @version 1.0.0
 * 
 * Provides digital filters and audio effects.
 * 
 * Usage:
 *   int16_t filtered = Filter_LowPass(sample);
 *   int16_t clipped = Filter_SoftClip(sample, 1600);
 */

#ifndef AUDIO_FILTERS_H_
#define AUDIO_FILTERS_H_

#include <stdint.h>

//=============================================================================
// PUBLIC API - FILTERS
//=============================================================================

/**
 * @brief Simple low-pass filter (50/50 mix with previous sample)
 * @param new_sample New input sample
 * @return Filtered sample
 */
int16_t Filter_LowPass(int16_t new_sample);

/**
 * @brief Configurable low-pass filter
 * @param new_sample New input sample
 * @param alpha Filter coefficient (0-256, 128=50% mix, 192=75% old)
 * @return Filtered sample
 */
int16_t Filter_LowPassAlpha(int16_t new_sample, uint8_t alpha);

/**
 * @brief High-pass filter
 * @param new_sample New input sample
 * @return Filtered sample
 */
int16_t Filter_HighPass(int16_t new_sample);

/**
 * @brief Reset filter state
 */
void Filter_Reset(void);

//=============================================================================
// PUBLIC API - EFFECTS
//=============================================================================

/**
 * @brief Soft clipping with gentle compression
 * @param sample Input sample
 * @param threshold Clipping threshold (e.g., 1600)
 * @return Clipped sample
 */
int16_t Filter_SoftClip(int16_t sample, int16_t threshold);

/**
 * @brief Hard clipping (brickwall limiter)
 * @param sample Input sample
 * @param limit Maximum amplitude (e.g., 2000)
 * @return Clipped sample
 */
int16_t Filter_HardClip(int16_t sample, int16_t limit);

/**
 * @brief Apply gain with frequency compensation
 * @param sample Input sample
 * @param gain Gain multiplier (e.g., 8)
 * @param frequency_hz Current frequency (for compensation)
 * @return Gained sample
 */
int16_t Filter_GainWithFreqCompensation(int16_t sample, uint8_t gain, uint32_t frequency_hz);

//=============================================================================
// PUBLIC API - UTILITIES
//=============================================================================

/**
 * @brief Scale sample to PWM range
 * @param sample Audio sample (-2000 to +2000)
 * @param pwm_center PWM center value (e.g., 2048)
 * @param pwm_max Maximum PWM value (e.g., 4095)
 * @return PWM value (0 to pwm_max)
 */
uint16_t Audio_SampleToPWM(int16_t sample, uint16_t pwm_center, uint16_t pwm_max);

#endif /* AUDIO_FILTERS_H_ */
