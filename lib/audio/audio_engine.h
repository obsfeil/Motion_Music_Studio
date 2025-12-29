/**
 * @file audio_engine.h
 * @brief Reusable Audio Synthesis Engine
 * @version 1.0.0
 * 
 * Provides waveform generation and audio utilities.
 * 
 * Features:
 * - Multiple waveforms (sine, square, sawtooth, triangle)
 * - Phase accumulator for smooth frequency generation
 * - Optimized wavetable lookup
 * 
 * Usage:
 *   Audio_Init(8000);  // 8 kHz sample rate
 *   Audio_SetFrequency(440);  // A4 note
 *   
 *   // In timer ISR:
 *   int16_t sample = Audio_GenerateSample();
 */

#ifndef AUDIO_ENGINE_H_
#define AUDIO_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

//=============================================================================
// PUBLIC TYPES
//=============================================================================

/**
 * @brief Waveform types
 */
typedef enum {
    WAVE_SINE = 0,      ///< Sine wave (smooth, pure tone)
    WAVE_SQUARE,        ///< Square wave (bright, harsh)
    WAVE_SAWTOOTH,      ///< Sawtooth wave (bright, buzzy)
    WAVE_TRIANGLE,      ///< Triangle wave (mellow, hollow)
    WAVE_COUNT
} Waveform_t;

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * @brief Initialize audio engine
 * @param sample_rate_hz Sample rate in Hz (e.g., 8000, 16000)
 */
void Audio_Init(uint16_t sample_rate_hz);

/**
 * @brief Set frequency for audio generation
 * @param frequency_hz Frequency in Hz (20-8000)
 */
void Audio_SetFrequency(uint32_t frequency_hz);

/**
 * @brief Set waveform type
 * @param waveform Waveform to use
 */
void Audio_SetWaveform(Waveform_t waveform);

/**
 * @brief Generate single audio sample
 * @return Audio sample (-1000 to +1000)
 * 
 * Call this from your timer ISR at sample_rate_hz
 */
int16_t Audio_GenerateSample(void);

/**
 * @brief Generate waveform value at specific index
 * @param index Wavetable index (0-255)
 * @param waveform Waveform type
 * @return Sample value (-1000 to +1000)
 * 
 * Useful for custom audio generation or visualization
 */
int16_t Audio_GenerateWaveform(uint8_t index, Waveform_t waveform);

/**
 * @brief Get current phase increment
 * @return 32-bit phase increment value
 */
uint32_t Audio_GetPhaseIncrement(void);

/**
 * @brief Get sine table (for custom use)
 * @return Pointer to 256-entry sine table
 */
const int16_t* Audio_GetSineTable(void);

/**
 * @brief Reset phase accumulator to 0
 */
void Audio_ResetPhase(void);

#endif /* AUDIO_ENGINE_H_ */
