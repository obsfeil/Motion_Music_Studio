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
// (48 kHz / 16 kHz = 3x)

const ADSR_Profile_t ADSR_PIANO = {
    .attack_samples = 480,       // 10ms attack  (160 * 3)
    .decay_samples = 9600,       // 200ms decay  (3200 * 3)
    .sustain_level = 700,        // 70% sustain  (Unchanged)
    .release_samples = 4800      // 100ms release (1600 * 3)
};

const ADSR_Profile_t ADSR_ORGAN = {
    .attack_samples = 0,         // Instant attack
    .decay_samples = 0,          // No decay
    .sustain_level = 1000,       // 100% sustain (Unchanged)
    .release_samples = 2400      // 50ms release (800 * 3)
};

const ADSR_Profile_t ADSR_STRINGS = {
    .attack_samples = 14400,     // 300ms attack (4800 * 3)
    .decay_samples = 19200,      // 400ms decay  (6400 * 3)
    .sustain_level = 800,        // 80% sustain  (Unchanged)
    .release_samples = 24000     // 500ms release (8000 * 3)
};

const ADSR_Profile_t ADSR_BASS = {
    .attack_samples = 960,       // 20ms attack  (320 * 3)
    .decay_samples = 4800,       // 100ms decay  (1600 * 3)
    .sustain_level = 900,        // 90% sustain  (Unchanged)
    .release_samples = 4800      // 100ms release (1600 * 3)
};

const ADSR_Profile_t ADSR_LEAD = {
    .attack_samples = 240,       // 5ms attack   (80 * 3)
    .decay_samples = 7200,       // 150ms decay  (2400 * 3)
    .sustain_level = 850,        // 85% sustain  (Unchanged)
    .release_samples = 9600      // 200ms release (3200 * 3)
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