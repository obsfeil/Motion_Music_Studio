/**
 * @file audio_envelope.h
 * @brief ADSR Envelope Generator
 * @version 1.0.0
 * 
 * Provides Attack-Decay-Sustain-Release envelope for shaping audio.
 * 
 * Usage:
 *   Envelope_t env;
 *   ADSR_Profile_t profile = {80, 1600, 700, 800};  // Piano-like
 *   Envelope_Init(&env, &profile);
 *   
 *   Envelope_NoteOn(&env);
 *   
 *   // In timer ISR:
 *   Envelope_Process(&env);
 *   int16_t amplitude = Envelope_GetAmplitude(&env);
 *   sample = (sample * amplitude) / 1000;
 */

#ifndef AUDIO_ENVELOPE_H_
#define AUDIO_ENVELOPE_H_

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// PUBLIC TYPES
//=============================================================================

/**
 * @brief Envelope states
 */
typedef enum {
    ENV_IDLE = 0,    ///< No sound
    ENV_ATTACK,      ///< Rising to peak
    ENV_DECAY,       ///< Falling to sustain
    ENV_SUSTAIN,     ///< Holding at sustain level
    ENV_RELEASE      ///< Fading out
} EnvelopeState_t;

/**
 * @brief ADSR profile (times in samples)
 */
typedef struct {
    uint16_t attack_samples;   ///< Attack time (samples)
    uint16_t decay_samples;    ///< Decay time (samples)
    uint16_t sustain_level;    ///< Sustain level (0-1000)
    uint16_t release_samples;  ///< Release time (samples)
} ADSR_Profile_t;

/**
 * @brief Envelope structure
 */
typedef struct {
    EnvelopeState_t state;     ///< Current state
    uint32_t phase;            ///< Current phase in state
    uint16_t amplitude;        ///< Current amplitude (0-1000)
    bool note_on;              ///< Note on flag
    const ADSR_Profile_t *profile;  ///< ADSR profile
} Envelope_t;

//=============================================================================
// PREDEFINED ADSR PROFILES
//=============================================================================

// Piano-like: Fast attack, medium decay
extern const ADSR_Profile_t ADSR_PIANO;

// Organ-like: Instant attack, no decay
extern const ADSR_Profile_t ADSR_ORGAN;

// Strings-like: Slow attack, long sustain
extern const ADSR_Profile_t ADSR_STRINGS;

// Bass-like: Medium attack, short decay
extern const ADSR_Profile_t ADSR_BASS;

// Lead-like: Fast attack, long sustain
extern const ADSR_Profile_t ADSR_LEAD;

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * @brief Initialize envelope
 * @param env Pointer to envelope structure
 * @param profile Pointer to ADSR profile
 */
void Envelope_Init(Envelope_t *env, const ADSR_Profile_t *profile);

/**
 * @brief Trigger note on (start attack)
 * @param env Pointer to envelope structure
 */
void Envelope_NoteOn(Envelope_t *env);

/**
 * @brief Trigger note off (start release)
 * @param env Pointer to envelope structure
 */
void Envelope_NoteOff(Envelope_t *env);

/**
 * @brief Process envelope (call at sample rate)
 * @param env Pointer to envelope structure
 */
void Envelope_Process(Envelope_t *env);

/**
 * @brief Get current amplitude (0-1000)
 * @param env Pointer to envelope structure
 * @return Amplitude (0-1000)
 */
uint16_t Envelope_GetAmplitude(Envelope_t *env);

/**
 * @brief Get current state
 * @param env Pointer to envelope structure
 * @return Current envelope state
 */
EnvelopeState_t Envelope_GetState(Envelope_t *env);

/**
 * @brief Check if envelope is active (not idle)
 * @param env Pointer to envelope structure
 * @return true if envelope is active
 */
bool Envelope_IsActive(Envelope_t *env);

/**
 * @brief Reset envelope to idle
 * @param env Pointer to envelope structure
 */
void Envelope_Reset(Envelope_t *env);

#endif /* AUDIO_ENVELOPE_H_ */