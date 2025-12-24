/**
 * @file main_MULTISYNTH.c  
 * @brief MSPM0G3507 Multi-Instrument Synthesizer
 * @version 7.0.0 - Professional Instrument System
 * 
 * ARCHITECTURE (følger MSPM0 beste praksis fra rapporten):
 * - Event-driven design med DMA
 * - ADSR envelope per instrument  
 * - Effekter: Vibrato, Tremolo
 * - Pitch bend via akselerometer
 * - Ingen blokkerende kode
 * 
 * INSTRUMENTER:
 * 1. PIANO   - Plucked sound, fast attack
 * 2. ORGAN   - Sustained, with vibrato
 * 3. STRINGS - Slow attack, rich harmonics
 * 4. BASS    - Deep, punchy
 * 5. LEAD    - Bright, expressive
 * 
 * KONTROLLER:
 * - Joystick X: Base frequency (100-2000 Hz)
 * - Joystick Y: Volume (0-100%)
 * - Accelerometer Y: Pitch bend (±1 octave)
 * - S1: Bytt instrument (Piano → Organ → Strings → Bass → Lead)
 * - S2: Toggle audio ON/OFF
 * - JOY_SEL: Toggle effects ON/OFF
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "ti_msp_dl_config.h"
#include "main.h"
#include "lcd_driver.h"
#include <stdio.h>
//=============================================================================
// INSTRUMENT SYSTEM
//=============================================================================
typedef enum {
    INSTRUMENT_PIANO = 0,
    INSTRUMENT_ORGAN,
    INSTRUMENT_STRINGS,
    INSTRUMENT_BASS,
    INSTRUMENT_LEAD,
    INSTRUMENT_COUNT
} Instrument_t;

// ADSR Envelope parameters per instrument
typedef struct {
    const char* name;           // Display name
    uint16_t attack_ms;         // Attack time (0-1000ms)
    uint16_t decay_ms;          // Decay time (0-1000ms)
    uint8_t sustain_level;      // Sustain level (0-100%)
    uint16_t release_ms;        // Release time (0-1000ms)
    Waveform_t waveform;        // Base waveform
    uint8_t harmonics;          // Number of harmonics (0-3)
    uint8_t vibrato_depth;      // Vibrato depth (0-100)
    uint8_t tremolo_depth;      // Tremolo depth (0-100)
    uint16_t color;             // LCD color for instrument
} InstrumentProfile_t;

// ✅ INSTRUMENT PRESETS (basert på synth teori)
static const InstrumentProfile_t INSTRUMENTS[INSTRUMENT_COUNT] = {
    // PIANO: Rask attack, medium decay, ingen sustain, medium release
    {
        .name = "PIANO",
        .attack_ms = 10,
        .decay_ms = 200,
        .sustain_level = 0,
        .release_ms = 150,
        .waveform = WAVE_TRIANGLE,
        .harmonics = 2,
        .vibrato_depth = 0,
        .tremolo_depth = 0,
        .color = LCD_COLOR_CYAN
    },
    // ORGAN: Ingen attack, ingen decay, full sustain, rask release
    {
        .name = "ORGAN",
        .attack_ms = 0,
        .decay_ms = 0,
        .sustain_level = 100,
        .release_ms = 50,
        .waveform = WAVE_SINE,
        .harmonics = 3,
        .vibrato_depth = 20,
        .tremolo_depth = 0,
        .color = LCD_COLOR_RED
    },
    // STRINGS: Langsom attack, lang decay, høy sustain, lang release
    {
        .name = "STRINGS",
        .attack_ms = 300,
        .decay_ms = 400,
        .sustain_level = 80,
        .release_ms = 500,
        .waveform = WAVE_SAWTOOTH,
        .harmonics = 2,
        .vibrato_depth = 30,
        .tremolo_depth = 10,
        .color = LCD_COLOR_YELLOW
    },
    // BASS: Medium attack, kort decay, høy sustain, kort release
    {
        .name = "BASS",
        .attack_ms = 20,
        .decay_ms = 100,
        .sustain_level = 90,
        .release_ms = 100,
        .waveform = WAVE_SINE,
        .harmonics = 1,
        .vibrato_depth = 0,
        .tremolo_depth = 0,
        .color = LCD_COLOR_BLUE
    },
    // LEAD: Rask attack, medium decay, høy sustain, medium release
    {
        .name = "LEAD",
        .attack_ms = 5,
        .decay_ms = 150,
        .sustain_level = 85,
        .release_ms = 200,
        .waveform = WAVE_SQUARE,
        .harmonics = 2,
        .vibrato_depth = 40,
        .tremolo_depth = 5,
        .color = LCD_COLOR_GREEN
    }
};

//=============================================================================
// ADSR ENVELOPE STATE
//=============================================================================
typedef enum {
    ENV_IDLE = 0,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvelopeState_t;

typedef struct {
    EnvelopeState_t state;
    uint32_t phase;         // Current phase in envelope
    uint16_t amplitude;     // Current envelope amplitude (0-1000)
    bool note_on;           // Is note currently playing?
} Envelope_t;

//=============================================================================
// GLOBAL STATE
//=============================================================================
volatile SynthState_t gSynthState; 
static uint32_t phase = 0;
static uint32_t phase_increment = 0;

// Instrument state
static Instrument_t current_instrument = INSTRUMENT_PIANO;
static Envelope_t envelope = {0};
static bool effects_enabled = true;

// Pitch bend state
static float base_frequency = FREQ_DEFAULT_HZ;
static float bent_frequency = FREQ_DEFAULT_HZ;
static float pitch_bend_semitones = 0.0f;

// Effect oscillators
static float vibrato_phase = 0.0f;
static float tremolo_phase = 0.0f;

//=============================================================================
// CONFIGURATION
//=============================================================================
#define PITCH_BEND_RANGE        12      // ±12 semitones (±1 octave)
#define PITCH_BEND_SMOOTHING    0.15f   // Smoothing factor
#define ACCEL_CENTER            2048    // Accelerometer center value
#define ACCEL_SENSITIVITY       200     // Sensitivity

#define VIBRATO_RATE_HZ         5.0f    // Vibrato speed (Hz)
#define TREMOLO_RATE_HZ         4.0f    // Tremolo speed (Hz)

//=============================================================================
// SINE WAVETABLE (256 samples)
//=============================================================================
static const int16_t sine_table[256] = {
    0, 25, 49, 74, 98, 122, 147, 171, 195, 219, 243, 267, 290, 314, 337, 360,
    383, 405, 428, 450, 471, 493, 514, 535, 555, 575, 595, 614, 633, 652, 670, 687,
    704, 721, 737, 753, 768, 783, 797, 811, 824, 837, 849, 860, 871, 882, 892, 901,
    910, 918, 926, 933, 939, 945, 951, 955, 960, 963, 966, 969, 971, 972, 973, 974,
    974, 973, 972, 971, 969, 966, 963, 960, 955, 951, 945, 939, 933, 926, 918, 910,
    901, 892, 882, 871, 860, 849, 837, 824, 811, 797, 783, 768, 753, 737, 721, 704,
    687, 670, 652, 633, 614, 595, 575, 555, 535, 514, 493, 471, 450, 428, 405, 383,
    360, 337, 314, 290, 267, 243, 219, 195, 171, 147, 122, 98, 74, 49, 25, 0,
    -25, -49, -74, -98, -122, -147, -171, -195, -219, -243, -267, -290, -314, -337, -360, -383,
    -405, -428, -450, -471, -493, -514, -535, -555, -575, -595, -614, -633, -652, -670, -687, -704,
    -721, -737, -753, -768, -783, -797, -811, -824, -837, -849, -860, -871, -882, -892, -901, -910,
    -918, -926, -933, -939, -945, -951, -955, -960, -963, -966, -969, -971, -972, -973, -974, -974,
    -973, -972, -971, -969, -966, -963, -960, -955, -951, -945, -939, -933, -926, -918, -910, -901,
    -892, -882, -871, -860, -849, -837, -824, -811, -797, -783, -768, -753, -737, -721, -704, -687,
    -670, -652, -633, -614, -595, -575, -555, -535, -514, -493, -471, -450, -428, -405, -383, -360,
    -337, -314, -290, -267, -243, -219, -195, -171, -147, -122, -98, -74, -49, -25
};

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
static void Process_Joystick(void);
static void Process_Pitch_Bend(void);
static void Process_Envelope(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static void Trigger_Note_On(void);
static void Trigger_Note_Off(void);
static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform);
static int16_t Apply_Harmonics(uint8_t index, Waveform_t base_waveform, uint8_t num_harmonics);
void Process_Buttons(void);
static void Display_Update(void);
static void Display_Splash(void);
static void Change_Instrument(void);

//=============================================================================
// MAIN
//=============================================================================
int main(void) {
    SYSCFG_DL_init();
    
    memset((void*)&gSynthState, 0, sizeof(SynthState_t));
    gSynthState.frequency = FREQ_DEFAULT_HZ;
    gSynthState.volume = VOLUME_DEFAULT;
    gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
    gSynthState.mode = MODE_SYNTH;
    gSynthState.audio_playing = 1;
    
    base_frequency = FREQ_DEFAULT_HZ;
    bent_frequency = FREQ_DEFAULT_HZ;
    
    // ✅ Initialize envelope PROPERLY
    envelope.state = ENV_ATTACK;  // Start in attack phase
    envelope.amplitude = 0;       // Start at 0
    envelope.phase = 0;
    envelope.note_on = true;      // Note is ON
    
    // ✅ CRITICAL: Calculate initial phase increment
    Update_Phase_Increment();
    
    // ✅ VERIFY: Debug check
    if (phase_increment == 0) {
        // Emergency: set to 440 Hz manually
        phase_increment = 236223201;  // 440 Hz at 8 kHz
    }
    
    LCD_Init();
    DL_GPIO_setPins(LCD_BACKLIGHT_PORT, LCD_BACKLIGHT_PIN_0_PIN);
    
    Display_Splash();
    LCD_DELAY_MS(2000);
    LCD_FillScreen(LCD_COLOR_BLACK);
    
    NVIC_EnableIRQ(ADC0_INT_IRQn);
    NVIC_EnableIRQ(ADC1_INT_IRQn);
    NVIC_EnableIRQ(TIMG7_INT_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    __enable_irq();
    
    DL_TimerG_startCounter(TIMER_SAMPLE_INST);
    DL_ADC12_enableConversions(ADC_MIC_JOY_INST);
    DL_ADC12_startConversion(ADC_MIC_JOY_INST);
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
    DL_ADC12_startConversion(ADC_ACCEL_INST);
    
    // Startup LED blink
    for (int i = 0; i < 3; i++) {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        LCD_DELAY_MS(200);
    }
    
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_BLUE_PIN);
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    
    uint32_t loop_counter = 0;
    uint32_t display_counter = 0;
    
    while (1) {
        if (loop_counter % 10000 == 0) {
            Process_Joystick();
        }
        
        if (loop_counter % 5000 == 0) {
            Process_Pitch_Bend();
        }
        
        if (loop_counter % 8000 == 0) {
            Process_Buttons();
        }
        
        if (display_counter++ >= 320000) {
            Display_Update();
            display_counter = 0;
        }
        
        if (loop_counter % 100000 == 0) {
            DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
        }
        
        loop_counter++;
    }
}

//=============================================================================
// DISPLAY FUNCTIONS
//=============================================================================

static void Display_Splash(void) {
    LCD_FillScreen(LCD_COLOR_BLACK);
    LCD_PrintString(5, 10, "MSPM0 SYNTH", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_LARGE);
    LCD_PrintString(10, 40, "Multi-Instrument", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_MEDIUM);
    LCD_PrintString(30, 70, "v7.0.0", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintString(10, 100, "5 Instruments!", LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
}

static void Display_Update(void) {
    const InstrumentProfile_t* inst = &INSTRUMENTS[current_instrument];
    
    // Header with instrument name (colored)
    LCD_DrawRect(0, 0, 128, 20, inst->color);
    LCD_PrintString(5, 6, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
    
    // Envelope state indicator
    const char* env_names[] = {"IDLE", "ATK", "DEC", "SUS", "REL"};
    LCD_PrintString(80, 6, env_names[envelope.state], 
                    LCD_COLOR_YELLOW, inst->color, FONT_SMALL);
    
    // Base Frequency
    LCD_PrintString(5, 25, "Base:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(40, 25, (int32_t)base_frequency, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintString(90, 25, "Hz", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    
    // Bent Frequency
    LCD_PrintString(5, 40, "Bent:", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(40, 40, (int32_t)bent_frequency, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintString(90, 40, "Hz", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
    
    // Pitch Bend Amount
    LCD_PrintString(5, 55, "Bend:", LCD_COLOR_MAGENTA, LCD_COLOR_BLACK, FONT_SMALL);
    char bend_str[16];
    snprintf(bend_str, sizeof(bend_str), "%+d", (int)pitch_bend_semitones);
    LCD_PrintString(40, 55, bend_str, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintString(65, 55, "semi", LCD_COLOR_MAGENTA, LCD_COLOR_BLACK, FONT_SMALL);
    
    // Volume
    LCD_PrintString(5, 70, "Vol:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(35, 70, gSynthState.volume, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintString(55, 70, "%", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    
    // Envelope Amplitude
    LCD_PrintString(70, 70, "Env:", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(100, 70, envelope.amplitude / 10, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    
    // Volume bar
    uint8_t bar_width = gSynthState.volume;
    if (bar_width > 100) bar_width = 100;
    LCD_DrawRect(5, 85, 100, 6, LCD_COLOR_DARKGRAY);
    LCD_DrawRect(5, 85, bar_width, 6, LCD_COLOR_GREEN);
    
    // Effects status
    LCD_PrintString(5, 95, "FX:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    if (effects_enabled) {
        LCD_PrintString(30, 95, "ON ", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
    } else {
        LCD_PrintString(30, 95, "OFF", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
    }
    
    // Instrument details
    char details[32];
    snprintf(details, sizeof(details), "H:%d V:%d T:%d", 
             inst->harmonics, inst->vibrato_depth, inst->tremolo_depth);
    LCD_PrintString(60, 95, details, LCD_COLOR_GRAY, LCD_COLOR_BLACK, FONT_SMALL);
    
    // Status
    if (gSynthState.audio_playing) {
        LCD_PrintString(5, 110, "PLAYING", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
    } else {
        LCD_PrintString(5, 110, "STOPPED", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
    }
}

//=============================================================================
// INTERRUPT HANDLERS
//=============================================================================

void TIMG7_IRQHandler(void) {
    if (DL_Timer_getPendingInterrupt(TIMER_SAMPLE_INST) == DL_TIMER_IIDX_ZERO) {
        gSynthState.timer_count++;
        
        // Update envelope (ADSR)
        Process_Envelope();
        
        if (gSynthState.audio_playing) {
            Generate_Audio_Sample();
        } else {
            DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        }
    }
}

void ADC0_IRQHandler(void) {
    gSynthState.adc0_count++;
    
    switch (DL_ADC12_getPendingInterrupt(ADC_MIC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            gSynthState.joy_y = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
            break;
            
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            gSynthState.joy_x = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_1);
            break;
            
        default:
            break;
    }
}

void ADC1_IRQHandler(void) {
    gSynthState.adc1_count++;
    
    if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) == DL_ADC12_IIDX_MEM3_RESULT_LOADED) {
        gSynthState.mic_level = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
        gSynthState.accel_x = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
        gSynthState.accel_y = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
        gSynthState.accel_z = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_3);
    }
}

void GPIOA_IRQHandler(void) {
    switch (DL_GPIO_getPendingInterrupt(GPIO_BUTTONS_PORT)) {
        case GPIO_BUTTONS_S1_IIDX:
            Change_Instrument();
            break;
            
        case GPIO_BUTTONS_S2_IIDX:
            gSynthState.audio_playing = !gSynthState.audio_playing;
            if (gSynthState.audio_playing) {
                DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
                Trigger_Note_On();
            } else {
                DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
                Trigger_Note_Off();
            }
            break;
            
        default:
            break;
    }
}

//=============================================================================
// INSTRUMENT FUNCTIONS
//=============================================================================

static void Change_Instrument(void) {
    current_instrument = (current_instrument + 1) % INSTRUMENT_COUNT;
    gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
    
    // Re-trigger envelope with new instrument
    Trigger_Note_On();
    
    // Visual feedback
    DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
}

static void Trigger_Note_On(void) {
    envelope.state = ENV_ATTACK;
    envelope.phase = 0;
    envelope.note_on = true;
}

static void Trigger_Note_Off(void) {
    envelope.state = ENV_RELEASE;
    envelope.phase = 0;
    envelope.note_on = false;
}

//=============================================================================
// ADSR ENVELOPE PROCESSING (Called at 8 kHz from ISR)
//=============================================================================

static void Process_Envelope(void) {
    const InstrumentProfile_t* inst = &INSTRUMENTS[current_instrument];
    
    // VIKTIG: 20 samples per millisekund (siden 20kHz = 20/ms)
    // Din gamle kode brukte 8, som gjorde lyden treg/feil.
    uint32_t attack_samples  = (inst->attack_ms * 20);
    uint32_t decay_samples   = (inst->decay_ms * 20);
    uint32_t release_samples = (inst->release_ms * 20);
    uint16_t sustain_amp     = (inst->sustain_level * 10);

    // ... (Resten av switch-casen din kan være lik som før) ...
    switch (envelope.state) {
        case ENV_IDLE:
            envelope.amplitude = 0;
            break;
        case ENV_ATTACK:
            if (attack_samples == 0) {
                envelope.amplitude = 1000;
                envelope.state = ENV_DECAY;
                envelope.phase = 0;
            } else {
                envelope.phase++;
                // Bruk uint64_t for å unngå overflow under beregningen
                envelope.amplitude = (uint16_t)(((uint64_t)envelope.phase * 1000) / attack_samples);
                if (envelope.amplitude >= 1000) {
                    envelope.amplitude = 1000;
                    envelope.state = ENV_DECAY;
                    envelope.phase = 0;
                }
            }
            break;
        // ... (Behold resten av decay, sustain, release logikken din) ...
        case ENV_DECAY:
           if (decay_samples == 0) {
               envelope.amplitude = sustain_amp;
               envelope.state = ENV_SUSTAIN;
           } else {
               envelope.phase++;
               uint16_t decay_range = 1000 - sustain_amp;
               uint16_t decayed = (uint16_t)(((uint64_t)envelope.phase * decay_range) / decay_samples);
               envelope.amplitude = 1000 - decayed;
               if (envelope.amplitude <= sustain_amp) {
                   envelope.amplitude = sustain_amp;
                   envelope.state = ENV_SUSTAIN;
               }
           }
           break;
        case ENV_SUSTAIN:
           envelope.amplitude = sustain_amp;
           if (!envelope.note_on) {
               envelope.state = ENV_RELEASE;
               envelope.phase = 0;
           }
           break;
        case ENV_RELEASE:
           if (release_samples == 0) {
               envelope.amplitude = 0;
               envelope.state = ENV_IDLE;
           } else {
               envelope.phase++;
               uint16_t start_amp = sustain_amp; // Forenkling: antar release fra sustain
               uint16_t released = (uint16_t)(((uint64_t)envelope.phase * start_amp) / release_samples);
               if (released >= start_amp) {
                   envelope.amplitude = 0;
                   envelope.state = ENV_IDLE;
               } else {
                   envelope.amplitude = start_amp - released;
               }
           }
           break;
    }
}
//=============================================================================
// WAVEFORM GENERATION
//=============================================================================

static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform) {
    int16_t sample = 0;
    
    switch (waveform) {
        case WAVE_SINE:
            sample = sine_table[index];
            break;
        case WAVE_SQUARE:
            sample = (index < 128) ? 1000 : -1000;
            break;
        case WAVE_SAWTOOTH:
            sample = (int16_t)(((int32_t)index * 2000 / 256) - 1000);
            break;
        case WAVE_TRIANGLE:
            sample = (index < 128) ? (int16_t)(((int32_t)index * 2000 / 128) - 1000) 
                                   : (int16_t)(1000 - ((int32_t)(index - 128) * 2000 / 128));
            break;
        default:
            sample = sine_table[index];
            break;
    }
    
    return sample;
}

static int16_t Apply_Harmonics(uint8_t index, Waveform_t base_waveform, uint8_t num_harmonics) {
    int32_t sample = Generate_Waveform(index, base_waveform);
    
    // Add harmonics (2nd, 3rd, 4th overtones)
    for (uint8_t h = 1; h <= num_harmonics; h++) {
        uint8_t harmonic_index = (index * (h + 1)) & 0xFF;
        int16_t harmonic = Generate_Waveform(harmonic_index, base_waveform);
        sample += harmonic / (2 * h);  // Decreasing amplitude
    }
    
    // Normalize to prevent clipping
    sample = sample / (num_harmonics + 1);
    if (sample > 1000) sample = 1000;
    if (sample < -1000) sample = -1000;
    
    return (int16_t)sample;
}


//=============================================================================
// AUDIO GENERATION (Called at 8 kHz from ISR)
//=============================================================================

static void Generate_Audio_Sample(void) {
    if (gSynthState.volume == 0) {
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        return;
    }

    // Hvis envelope er ferdig, spar CPU ved å bare oppdatere fase
    if (envelope.amplitude == 0) {
        phase += phase_increment;
        gSynthState.audio_samples_generated++;
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        return;
    }

    const InstrumentProfile_t* inst = &INSTRUMENTS[current_instrument];
    uint8_t index = (uint8_t)((phase >> 24) & 0xFF);
    
    // Hent grunnlyd med harmonier
    int32_t sample = Apply_Harmonics(index, inst->waveform, inst->harmonics);

    // --- EFFEKTER (Konvertert til heltall for 80MHz ytelse) ---
    if (effects_enabled) {
        // Vibrato (Forenklet: bruker timer_count for hastighet)
        if (inst->vibrato_depth > 0) {
            // Bruker en "fake" sinus ved å se på timer bits for å spare tid
            // Dette er en veldig rask måte å lage en LFO på
            uint8_t lfo_idx = (gSynthState.timer_count >> 8) & 0xFF; 
            int16_t vib_val = sine_table[lfo_idx];
            // Modulerer indeksen litt
            index += (vib_val * inst->vibrato_depth) / 2000;
            // Hent sample på nytt med ny indeks
            sample = Apply_Harmonics(index, inst->waveform, inst->harmonics);
        }

        // Tremolo (Volum-modulering)
        if (inst->tremolo_depth > 0) {
            uint8_t lfo_idx = (gSynthState.timer_count >> 7) & 0xFF; // Litt raskere LFO
            int16_t trem_val = sine_table[lfo_idx]; // -1000 til 1000
            // Skalerer volumet ned og opp
            // 1024 er "1.0" i fixed point
            int32_t gain = 1024 - ((trem_val + 1000) * inst->tremolo_depth) / 200;
            sample = (sample * gain) / 1024;
        }
    }

    // --- ADSR ENVELOPE ---
    sample = (sample * envelope.amplitude) / 1000;

    // --- MASTER VOLUME ---
    sample = (sample * gSynthState.volume) / 100;

    // PWM Output (Senter 2048)
    int32_t val = 2048 + (sample * 2); // *2 for høyere volum
    
    // Klipping (Clamping)
    if(val < 1) val = 1;
    if(val > 4095) val = 4095;

    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, (uint16_t)val, DL_TIMER_CC_0_INDEX);
    phase += phase_increment;
    gSynthState.audio_samples_generated++;
}

//=============================================================================
// CONTROL FUNCTIONS
//=============================================================================

static void Process_Joystick(void) {
    // Joystick X -> Base Frequency
    if (gSynthState.joy_x > 100) {
        uint32_t freq_int = FREQ_MIN_HZ + 
                            ((gSynthState.joy_x * (FREQ_MAX_HZ - FREQ_MIN_HZ)) / 4095);
        
        uint32_t curr_freq = (uint32_t)base_frequency;
        uint32_t diff = (freq_int > curr_freq) ? (freq_int - curr_freq) : (curr_freq - freq_int);
        
        if (diff > 10) {
            base_frequency = (float)freq_int;
        }
    }
    
    // Joystick Y -> Volume
    if (gSynthState.joy_y > 100) {
        uint8_t new_vol = (uint8_t)((gSynthState.joy_y * 100) / 4095);
        if (new_vol > 100) new_vol = 100;
        
        if (new_vol != gSynthState.volume) {
            gSynthState.volume = new_vol;
        }
    }
}

static void Process_Pitch_Bend(void) {
    int16_t accel_y_local = gSynthState.accel_y;
    int16_t deviation = accel_y_local - ACCEL_CENTER;
    float normalized = (float)deviation / (float)ACCEL_SENSITIVITY;
    
    if (normalized > 1.0f) normalized = 1.0f;
    if (normalized < -1.0f) normalized = -1.0f;
    
    float target_semitones = normalized * PITCH_BEND_RANGE;
    pitch_bend_semitones = pitch_bend_semitones * (1.0f - PITCH_BEND_SMOOTHING) + 
                           target_semitones * PITCH_BEND_SMOOTHING;
    
    float bend_ratio = powf(2.0f, pitch_bend_semitones / 12.0f);
    bent_frequency = base_frequency * bend_ratio;
    
    if (bent_frequency < FREQ_MIN_HZ) bent_frequency = FREQ_MIN_HZ;
    if (bent_frequency > FREQ_MAX_HZ) bent_frequency = FREQ_MAX_HZ;
    
    gSynthState.frequency = bent_frequency;
    Update_Phase_Increment();
}

static void Update_Phase_Increment(void) {
    // Sjekk grenser
    if (bent_frequency < FREQ_MIN_HZ) bent_frequency = FREQ_MIN_HZ;
    if (bent_frequency > FREQ_MAX_HZ) bent_frequency = FREQ_MAX_HZ;

    // Bruk float KUN HER (i main loop, ikke i interrupt)
    // 20000.0 er samplingsraten din
    double inc = ((double)bent_frequency * 4294967296.0) / 20000.0;
    
    phase_increment = (uint32_t)inc;

    // Nødløsning
    if (phase_increment == 0) phase_increment = 214748;
}

void Process_Buttons(void) {
    // Statiske variabler husker verdien til neste gang funksjonen kjøres
    static uint32_t s1_prev = 1; 
    static uint32_t s2_prev = 1;
    static uint32_t joy_prev = 1;

    // Les nåværende status (1 = ikke trykket, 0 = trykket)
    uint32_t s1 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN);
    uint32_t s2 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN);
    
    // Hent Joy Select hvis den finnes
    #ifdef GPIO_BUTTONS_JOY_SEL_PIN
    uint32_t joy = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
    #else
    uint32_t joy = 1;
    #endif

    // --- S1: Change Instrument ---
    // Sjekk: Er knappen nede NÅ (0) OG var den oppe SIST (1)?
    if ((s1 == 0) && (s1_prev != 0)) {
        Change_Instrument();
        // Vi har fjernet while-løkken her!
    }

    // --- S2: Play/Stop ---
    if ((s2 == 0) && (s2_prev != 0)) {
        gSynthState.audio_playing = !gSynthState.audio_playing;
        
        if (gSynthState.audio_playing) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_On();
        } else {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_Off();
        }
        // Vi har fjernet while-løkken her også!
    }
    
    // --- Joy Select: Toggle Effects ---
    #ifdef GPIO_BUTTONS_JOY_SEL_PIN
    if ((joy == 0) && (joy_prev != 0)) {
        effects_enabled = !effects_enabled;
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    }
    #endif

    // Oppdater historikken for neste runde
    s1_prev = s1;
    s2_prev = s2;
    joy_prev = joy;
}

void HardFault_Handler(void) {
    while (1) {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}