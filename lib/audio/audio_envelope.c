/**
 * @file audio_envelope.c
 * @brief ADSR Envelope Implementation
 * 
 * ✨ UPDATED FOR 48 KHZ SAMPLE RATE (v31.0)
 */

#include "audio_envelope.h"

//=============================================================================
// PREDEFINED ADSR PROFILES (for 48 kHz sample rate) ✨
//=============================================================================
// All sample counts are 3x compared to 16 kHz version
// (25 kHz / 16 kHz = 3x)

const ADSR_Profile_t ADSR_PIANO = {
    .attack_samples = 250,       // 10ms  (0.010 * 25000)
    .decay_samples = 5000,       // 200ms (0.200 * 25000)
    .sustain_level = 700,        // 70%   (Uendret - nivå er ikke tidsavhengig)
    .release_samples = 2500      // 100ms (0.100 * 25000)
};

const ADSR_Profile_t ADSR_ORGAN = {
    .attack_samples = 0,         // Instant
    .decay_samples = 0,          // No decay
    .sustain_level = 1000,       // 100%
    .release_samples = 1250      // 50ms  (0.050 * 25000)
};

const ADSR_Profile_t ADSR_STRINGS = {
    .attack_samples = 7500,      // 300ms (0.300 * 25000)
    .decay_samples = 10000,      // 400ms (0.400 * 25000)
    .sustain_level = 800,        // 80%
    .release_samples = 12500     // 500ms (0.500 * 25000)
};

const ADSR_Profile_t ADSR_BASS = {
    .attack_samples = 500,       // 20ms  (0.020 * 25000)
    .decay_samples = 2500,       // 100ms (0.100 * 25000)
    .sustain_level = 900,        // 90%
    .release_samples = 2500      // 100ms (0.100 * 25000)
};

const ADSR_Profile_t ADSR_LEAD = {
    .attack_samples = 125,       // 5ms   (0.005 * 25000)
    .decay_samples = 3750,       // 150ms (0.150 * 25000)
    .sustain_level = 850,        // 85%
    .release_samples = 5000      // 200ms (0.200 * 25000)
};

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void Envelope_Init(Envelope_t *env, const ADSR_Profile_t *profile) {
    env->state = ENV_IDLE;
    env->phase = 0;
    env->amplitude = 0;
    env->note_on = false;
    env->profile = profile;
}

void Envelope_NoteOn(Envelope_t *env) {
    env->state = ENV_ATTACK;
    env->phase = 0;
    env->amplitude = 0;
    env->note_on = true;
}

void Envelope_NoteOff(Envelope_t *env) {
    env->note_on = false;
    if (env->state != ENV_IDLE) {
        env->state = ENV_RELEASE;
        env->phase = 0;
    }
}

void Envelope_Process(Envelope_t *env) {
    const ADSR_Profile_t *adsr = env->profile;
    
    switch (env->state) {
        case ENV_IDLE:
            env->amplitude = 0;
            break;
            
        case ENV_ATTACK:
            if (adsr->attack_samples == 0) {
                // Instant attack
                env->amplitude = 1000;
                env->state = ENV_DECAY;
                env->phase = 0;
            } else {
                env->phase++;
                env->amplitude = (uint16_t)((env->phase * 1000) / adsr->attack_samples);
                if (env->amplitude >= 1000) {
                    env->amplitude = 1000;
                    env->state = ENV_DECAY;
                    env->phase = 0;
                }
            }
            break;
            
        case ENV_DECAY:
            if (adsr->decay_samples == 0) {
                // No decay
                env->amplitude = adsr->sustain_level;
                env->state = ENV_SUSTAIN;
            } else {
                env->phase++;
                uint16_t range = 1000 - adsr->sustain_level;
                uint16_t decayed = (uint16_t)((env->phase * range) / adsr->decay_samples);
                if (decayed >= range) {
                    env->amplitude = adsr->sustain_level;
                    env->state = ENV_SUSTAIN;
                } else {
                    env->amplitude = 1000 - decayed;
                }
            }
            break;
            
        case ENV_SUSTAIN:
            env->amplitude = adsr->sustain_level;
            if (!env->note_on) {
                env->state = ENV_RELEASE;
                env->phase = 0;
            }
            break;
            
        case ENV_RELEASE:
            if (adsr->release_samples == 0) {
                // Instant release
                env->amplitude = 0;
                env->state = ENV_IDLE;
            } else {
                env->phase++;
                uint16_t start = adsr->sustain_level;
                uint16_t released = (uint16_t)((env->phase * start) / adsr->release_samples);
                if (released >= start) {
                    env->amplitude = 0;
                    env->state = ENV_IDLE;
                } else {
                    env->amplitude = start - released;
                }
            }
            break;
    }
}

uint16_t Envelope_GetAmplitude(Envelope_t *env) {
    return env->amplitude;
}

EnvelopeState_t Envelope_GetState(Envelope_t *env) {
    return env->state;
}

bool Envelope_IsActive(Envelope_t *env) {
    return (env->state != ENV_IDLE);
}

void Envelope_Reset(Envelope_t *env) {
    env->state = ENV_IDLE;
    env->phase = 0;
    env->amplitude = 0;
    env->note_on = false;
}