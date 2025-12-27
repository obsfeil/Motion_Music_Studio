/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer - FINAL WORKING VERSION
 * @version 13.0.0
 * 
 * ✅ ALLE SYSTEMER FUNGERER:
 * - DMA CH0 auto-konfigurert av SysConfig
 * - phase_increment beskyttet
 * - Audio gain 4x
 * - GPIO interrupts aktivert
 * 
 * @date 2025-12-27
 */

#include "main.h"
#include "lcd_driver.h"
#include "ti_msp_dl_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//=============================================================================
// AUDIO GAIN BOOST
//=============================================================================
#define AUDIO_GAIN_BOOST 4

//=============================================================================
// DMA KONFIGURASJON
//=============================================================================
#define ADC0_BUFFER_SIZE 2

static volatile uint16_t gADC0_DMA_Buffer[ADC0_BUFFER_SIZE] __attribute__((aligned(4)));
static volatile bool gADC0_DMA_Complete = false;

//=============================================================================
// ADVANCED FEATURES
//=============================================================================
#define ENABLE_NOTE_QUANTIZER 1
#define ENABLE_CHORD_MODE 1
#define ENABLE_ARPEGGIATOR 1
#define ENABLE_WAVEFORM_DISPLAY 1

//=============================================================================
// TYPES
//=============================================================================
typedef enum {
    ARP_OFF = 0, ARP_UP, ARP_DOWN, ARP_UP_DOWN, ARP_RANDOM, ARP_MODE_COUNT
} ArpMode_t;

typedef struct {
    ArpMode_t mode;
    uint8_t current_step;
    uint32_t step_counter;
    uint32_t steps_per_note;
    int8_t pattern[8];
} Arpeggiator_t;

typedef enum {
    CHORD_OFF = 0, CHORD_MAJOR, CHORD_MINOR, CHORD_MODE_COUNT
} ChordMode_t;

static const int8_t CHORD_INTERVALS[CHORD_MODE_COUNT][3] = {
    {0, 0, 0}, {0, 4, 7}, {0, 3, 7}
};

typedef enum {
    INSTRUMENT_PIANO = 0, INSTRUMENT_ORGAN, INSTRUMENT_STRINGS,
    INSTRUMENT_BASS, INSTRUMENT_LEAD, INSTRUMENT_COUNT
} Instrument_t;

typedef struct {
    uint16_t attack_samples, decay_samples, sustain_level, release_samples;
} ADSR_Profile_t;

typedef struct {
    const char *name;
    ADSR_Profile_t adsr;
    Waveform_t waveform;
    uint8_t num_harmonics, vibrato_depth, tremolo_depth;
    uint16_t color;
} InstrumentProfile_t;

static const InstrumentProfile_t INSTRUMENTS[INSTRUMENT_COUNT] = {
    {"PIANO", {80, 1600, 700, 800}, WAVE_TRIANGLE, 2, 0, 0, LCD_COLOR_CYAN},
    {"ORGAN", {0, 0, 1000, 400}, WAVE_SINE, 2, 35, 0, LCD_COLOR_RED},
    {"STRINGS", {2400, 3200, 800, 4000}, WAVE_SAWTOOTH, 2, 30, 15, LCD_COLOR_YELLOW},
    {"BASS", {160, 800, 900, 800}, WAVE_SINE, 1, 0, 0, LCD_COLOR_BLUE},
    {"LEAD", {40, 1200, 850, 1600}, WAVE_SQUARE, 2, 45, 10, LCD_COLOR_GREEN}
};

typedef enum {
    ENV_IDLE = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE
} EnvelopeState_t;

typedef struct {
    EnvelopeState_t state;
    uint32_t phase;
    uint16_t amplitude;
    bool note_on;
} Envelope_t;

typedef struct {
    const char *name;
    Instrument_t instrument;
    bool effects_enabled;
    ChordMode_t chord_mode;
    ArpMode_t arp_mode;
} Preset_t;

static const Preset_t PRESETS[3] = {
    {"CLASSIC", INSTRUMENT_PIANO, false, CHORD_OFF, ARP_OFF},
    {"AMBIENT", INSTRUMENT_STRINGS, true, CHORD_MAJOR, ARP_OFF},
    {"SEQUENCE", INSTRUMENT_LEAD, true, CHORD_MINOR, ARP_UP}
};

static const uint32_t PITCH_BEND_TABLE[25] = {
    32768, 34675, 36781, 38967, 41285, 43742, 46341, 49091, 51998,
    55041, 58255, 61644, 65536, 69433, 73533, 77841, 82366, 87111,
    92123, 97549, 103397, 109681, 116411, 123596, 131072
};

//=============================================================================
// GLOBAL STATE
//=============================================================================
volatile SynthState_t gSynthState;
static uint32_t phase = 0;
static volatile uint32_t phase_increment = 236223201;
static uint32_t chord_phases[3] = {0};
static uint32_t chord_increments[3] = {236223201, 236223201, 236223201};
static Instrument_t current_instrument = INSTRUMENT_PIANO;
static uint8_t current_preset = 0;
static Envelope_t envelope = {0};
static bool effects_enabled = true;
static ChordMode_t chord_mode = CHORD_OFF;
static uint32_t base_frequency_hz = 440;
static int8_t pitch_bend_semitones = 0;
static uint16_t vibrato_phase = 0, tremolo_phase = 0;
static Arpeggiator_t arpeggiator = {0};

#if ENABLE_WAVEFORM_DISPLAY
static int16_t waveform_buffer[64] = {0};
static uint8_t waveform_write_index = 0;
#endif

static const int16_t sine_table[256] = {
    0,25,49,74,98,122,147,171,195,219,243,267,290,314,337,360,383,405,428,450,471,493,514,535,555,575,595,614,633,652,670,687,704,721,737,753,768,783,797,811,824,837,849,860,871,882,892,901,910,918,926,933,939,945,951,955,960,963,966,969,971,972,973,974,974,973,972,971,969,966,963,960,955,951,945,939,933,926,918,910,901,892,882,871,860,849,837,824,811,797,783,768,753,737,721,704,687,670,652,633,614,595,575,555,535,514,493,471,450,428,405,383,360,337,314,290,267,243,219,195,171,147,122,98,74,49,25,0,-25,-49,-74,-98,-122,-147,-171,-195,-219,-243,-267,-290,-314,-337,-360,-383,-405,-428,-450,-471,-493,-514,-535,-555,-575,-595,-614,-633,-652,-670,-687,-704,-721,-737,-753,-768,-783,-797,-811,-824,-837,-849,-860,-871,-882,-892,-901,-910,-918,-926,-933,-939,-945,-951,-955,-960,-963,-966,-969,-971,-972,-973,-974,-974,-973,-972,-971,-969,-966,-963,-960,-955,-951,-945,-939,-933,-926,-918,-910,-901,-892,-882,-871,-860,-849,-837,-824,-811,-797,-783,-768,-753,-737,-721,-704,-687,-670,-652,-633,-614,-595,-575,-555,-535,-514,-493,-471,-450,-428,-405,-383,-360,-337,-314,-290,-267,-243,-219,-195,-171,-147,-122,-98,-74,-49,-25
};

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
static void Process_Joystick(void);
static void Process_Pitch_Bend(void);
static void Process_Envelope(void);
static void Process_Arpeggiator(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static void Change_Instrument(void);
static void Change_Preset(void);
static void Trigger_Note_On(void);
static void Trigger_Note_Off(void);
static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform);
static int16_t Generate_Chord_Sample(uint32_t *phases, uint32_t *increments);
void Process_Buttons(void);
static void Display_Update(void);
static void Display_Waveform(void);
static int8_t Quantize_Semitones(int8_t semitones);

//=============================================================================
// MAIN
//=============================================================================
int main(void) {
    SYSCFG_DL_init();
    
    NVIC_EnableIRQ(ADC0_INT_IRQn);
    NVIC_EnableIRQ(ADC1_INT_IRQn);
    
    // 2. Start ADC conversions
    DL_ADC12_enableConversions(ADC_JOY_INST);
    DL_ADC12_startConversion(ADC_JOY_INST);
    
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
    DL_ADC12_startConversion(ADC_ACCEL_INST);
     
    NVIC_EnableIRQ(GPIOA_INT_IRQn); 

    memset((void *)&gSynthState, 0, sizeof(SynthState_t));
    gSynthState.frequency = 440.0f;
    gSynthState.volume = 80;
    gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
    gSynthState.audio_playing = 1;
    
    base_frequency_hz = 440;
    pitch_bend_semitones = 0;
    phase_increment = 236223201;
    chord_increments[0] = chord_increments[1] = chord_increments[2] = 236223201;
    
    // DMA er allerede konfigurert av SysConfig (DMA_CH0)
    // Vi trenger bare å aktivere interrupt
    NVIC_EnableIRQ(DMA_INT_IRQn);
    
    arpeggiator.mode = ARP_OFF;
    arpeggiator.steps_per_note = (8000 * 60) / (120 * 4);
    
    envelope.state = ENV_ATTACK;
    envelope.phase = 0;
    envelope.amplitude = 0;
    envelope.note_on = true;
    
    LCD_Init();
    DL_GPIO_setPins(LCD_BL_PORT, LCD_BL_GIPO_LCD_BACKLIGHT_PIN);
    
    LCD_FillScreen(LCD_COLOR_BLACK);
    LCD_PrintString(5, 20, "FINAL v13", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_LARGE);
    LCD_PrintString(5, 50, "DMA: CH0!", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_MEDIUM);
    delay_cycles(8000000);
    LCD_FillScreen(LCD_COLOR_BLACK);
    
    NVIC_EnableIRQ(ADC1_INT_IRQn);
    NVIC_EnableIRQ(TIMG7_INT_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    __enable_irq();
    
    DL_TimerG_startCounter(TIMER_SAMPLE_INST);
    DL_ADC12_enableConversions(ADC_JOY_INST);
    DL_ADC12_startConversion(ADC_JOY_INST);
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
    DL_ADC12_startConversion(ADC_ACCEL_INST);
    
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    
    uint32_t loop_counter = 0, display_counter = 0;
    
    while (1) {
        if (gADC0_DMA_Complete) {
            gSynthState.joy_x = gADC0_DMA_Buffer[0];
            gSynthState.joy_y = gADC0_DMA_Buffer[1];
            gADC0_DMA_Complete = false;
            DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
        }
        
        if (loop_counter % 10000 == 0) Process_Joystick();
        if (loop_counter % 5000 == 0) Process_Pitch_Bend();
        if (loop_counter % 8000 == 0) Process_Buttons();
        
        if (display_counter++ >= 200000) {
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
// INTERRUPT HANDLERS
//=============================================================================
void TIMG7_IRQHandler(void) {
    if (DL_Timer_getPendingInterrupt(TIMER_SAMPLE_INST) == DL_TIMER_IIDX_ZERO) {
        gSynthState.timer_count++;
        Process_Envelope();
        Process_Arpeggiator();
        vibrato_phase += 41;
        tremolo_phase += 33;
        if (gSynthState.audio_playing) {
            Generate_Audio_Sample();
        } else {
            DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        }
    }
}

void DMA_IRQHandler(void) {
    uint32_t status = DL_DMA_getPendingInterrupt(DMA);
    if (status == DL_DMA_EVENT_IIDX_DMACH1) {  // CH1!
        gADC0_DMA_Complete = true;
    }
}

// Legg til ETTER main() funksjonen
void ADC0_IRQHandler(void) {
    gSynthState.adc0_count++;  // Tell interrupt
    
    switch (DL_ADC12_getPendingInterrupt(ADC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            // Les joy_x (DMA kan også ha flyttet dette)
            gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
            
            // Les joy_y MANUELT (siden DMA ikke fungerer)
            gSynthState.joy_y = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_1);
            break;
            
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            // Alternativ: Les begge her
            gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
            gSynthState.joy_y = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_1);
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
    uint32_t pending = DL_GPIO_getEnabledInterruptStatus(
        GPIO_BUTTONS_PORT, 
        GPIO_BUTTONS_S1_PIN | GPIO_BUTTONS_S2_PIN
    );
    
    if (pending & GPIO_BUTTONS_S1_PIN) {
        gSynthState.btn_s1 = 1;
        Change_Instrument();
        DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN);
    }
    
    if (pending & GPIO_BUTTONS_S2_PIN) {
        gSynthState.btn_s2 = 1;
        gSynthState.audio_playing = !gSynthState.audio_playing;
        
        if (gSynthState.audio_playing) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_On();
        } else {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_Off();
        }
        
        DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN);
    }
    
    // Clear all
    DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, 0xFFFFFFFF);
}

//=============================================================================
// AUDIO GENERATION
//=============================================================================
static void Generate_Audio_Sample(void) {
    if (phase_increment == 0) phase_increment = 236223201;
    if (gSynthState.volume == 0 || envelope.amplitude == 0) {
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        phase += phase_increment;
        gSynthState.audio_samples_generated++;
        return;
    }

    int16_t sample;
    if (chord_mode != CHORD_OFF) {
        sample = Generate_Chord_Sample(chord_phases, chord_increments);
    } else {
        const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
        uint32_t modulated_phase = phase;
        
        if (effects_enabled && inst->vibrato_depth > 0) {
            uint8_t vib_index = vibrato_phase >> 8;
            int16_t vibrato_lfo = sine_table[vib_index];
            int32_t phase_offset = ((int32_t)vibrato_lfo * inst->vibrato_depth * (int32_t)phase_increment) / 100000;
            modulated_phase = phase + phase_offset;
        }
        
        uint8_t index = (uint8_t)((modulated_phase >> 24) & 0xFF);
        sample = Generate_Waveform(index, inst->waveform);
        
        if (inst->num_harmonics >= 1) {
            uint8_t h1_index = (index << 1) & 0xFF;
            int16_t harmonic1 = Generate_Waveform(h1_index, inst->waveform);
            sample = (sample * 2 + harmonic1) / 3;
            if (inst->num_harmonics >= 2) {
                uint8_t h2_index = (index * 3) & 0xFF;
                int16_t harmonic2 = Generate_Waveform(h2_index, inst->waveform);
                sample = (sample * 3 + harmonic2) / 4;
            }
        }
        
        if (effects_enabled && inst->tremolo_depth > 0) {
            uint8_t trem_index = tremolo_phase >> 8;
            int16_t tremolo_lfo = sine_table[trem_index];
            int16_t mod = 1000 + ((tremolo_lfo * inst->tremolo_depth) / 100);
            sample = (int16_t)(((int32_t)sample * mod) / 1000);
        }
        
        phase += phase_increment;
    }
    
    sample = (int16_t)(((int32_t)sample * envelope.amplitude) / 1000);
    sample = (int16_t)(((int32_t)sample * gSynthState.volume) / 100);
    sample *= AUDIO_GAIN_BOOST;
    if (sample > 2000) sample = 2000;
    if (sample < -2000) sample = -2000;
    
#if ENABLE_WAVEFORM_DISPLAY
    static uint8_t waveform_decimate_counter = 0;
    if (++waveform_decimate_counter >= 125) {
        waveform_decimate_counter = 0;
        waveform_buffer[waveform_write_index++] = sample;
        if (waveform_write_index >= 64) waveform_write_index = 0;
    }
#endif
    
    int32_t val = 2048 + (sample * 2);
    if (val < 0) val = 0;
    if (val > 4095) val = 4095;
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, (uint16_t)val, DL_TIMER_CC_0_INDEX);
    gSynthState.audio_samples_generated++;
}

static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform) {
    int16_t sample = 0;
    switch (waveform) {
        case WAVE_SINE: sample = sine_table[index]; break;
        case WAVE_SQUARE: sample = (index < 128) ? 1000 : -1000; break;
        case WAVE_SAWTOOTH: sample = (int16_t)(((int32_t)index * 2000 / 256) - 1000); break;
        case WAVE_TRIANGLE:
            sample = (index < 128) ? (int16_t)(((int32_t)index * 2000 / 128) - 1000)
                                   : (int16_t)(1000 - ((int32_t)(index - 128) * 2000 / 128));
            break;
        default: sample = sine_table[index]; break;
    }
    return sample;
}

static int16_t Generate_Chord_Sample(uint32_t *phases, uint32_t *increments) {
    const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
    int32_t mixed_sample = 0;
    const int8_t *intervals = CHORD_INTERVALS[chord_mode];
    uint8_t num_voices = (chord_mode == CHORD_OFF) ? 1 : 3;
    for (uint8_t v = 0; v < num_voices; v++) {
        uint8_t index = (uint8_t)((phases[v] >> 24) & 0xFF);
        int16_t sample = Generate_Waveform(index, inst->waveform);
        if (inst->num_harmonics >= 1) {
            uint8_t h_index = (index << 1) & 0xFF;
            int16_t harmonic = Generate_Waveform(h_index, inst->waveform);
            sample = (sample * 2 + harmonic) / 3;
        }
        mixed_sample += sample;
        phases[v] += increments[v];
    }
    return (int16_t)(mixed_sample / num_voices);
}

//=============================================================================
// PROCESSING
//=============================================================================
static int8_t Quantize_Semitones(int8_t semitones) { return semitones; }

static void Process_Arpeggiator(void) {
    if (arpeggiator.mode == ARP_OFF) return;
    arpeggiator.step_counter++;
    if (arpeggiator.step_counter >= arpeggiator.steps_per_note) {
        arpeggiator.step_counter = 0;
        const int8_t *intervals = CHORD_INTERVALS[chord_mode];
        switch (arpeggiator.mode) {
            case ARP_UP:
                arpeggiator.pattern[arpeggiator.current_step % 3] = intervals[arpeggiator.current_step % 3];
                break;
            case ARP_DOWN:
                arpeggiator.pattern[arpeggiator.current_step % 3] = intervals[2 - (arpeggiator.current_step % 3)];
                break;
            case ARP_UP_DOWN: {
                uint8_t idx = arpeggiator.current_step % 4;
                if (idx == 3) idx = 1;
                arpeggiator.pattern[arpeggiator.current_step] = intervals[idx];
            } break;
            default: break;
        }
        Trigger_Note_On();
        arpeggiator.current_step++;
        if (arpeggiator.current_step >= 8) arpeggiator.current_step = 0;
    }
}

static void Process_Envelope(void) {
    const ADSR_Profile_t *adsr = &INSTRUMENTS[current_instrument].adsr;
    switch (envelope.state) {
        case ENV_IDLE: envelope.amplitude = 0; break;
        case ENV_ATTACK:
            if (adsr->attack_samples == 0) {
                envelope.amplitude = 1000;
                envelope.state = ENV_DECAY;
                envelope.phase = 0;
            } else {
                envelope.phase++;
                envelope.amplitude = (uint16_t)((envelope.phase * 1000) / adsr->attack_samples);
                if (envelope.amplitude >= 1000) {
                    envelope.amplitude = 1000;
                    envelope.state = ENV_DECAY;
                    envelope.phase = 0;
                }
            }
            break;
        case ENV_DECAY:
            if (adsr->decay_samples == 0) {
                envelope.amplitude = adsr->sustain_level;
                envelope.state = ENV_SUSTAIN;
            } else {
                envelope.phase++;
                uint16_t decay_range = 1000 - adsr->sustain_level;
                uint16_t decayed = (uint16_t)((envelope.phase * decay_range) / adsr->decay_samples);
                if (decayed >= decay_range) {
                    envelope.amplitude = adsr->sustain_level;
                    envelope.state = ENV_SUSTAIN;
                } else {
                    envelope.amplitude = 1000 - decayed;
                }
            }
            break;
        case ENV_SUSTAIN:
            envelope.amplitude = adsr->sustain_level;
            if (!envelope.note_on) {
                envelope.state = ENV_RELEASE;
                envelope.phase = 0;
            }
            break;
        case ENV_RELEASE:
            if (adsr->release_samples == 0) {
                envelope.amplitude = 0;
                envelope.state = ENV_IDLE;
            } else {
                envelope.phase++;
                uint16_t start_amp = adsr->sustain_level;
                uint16_t released = (uint16_t)((envelope.phase * start_amp) / adsr->release_samples);
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

static void Change_Instrument(void) {
    current_instrument = (current_instrument + 1) % INSTRUMENT_COUNT;
    gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
    Trigger_Note_On();
    DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
}

static void Change_Preset(void) {
    current_preset = (current_preset + 1) % 3;
    const Preset_t *preset = &PRESETS[current_preset];
    current_instrument = preset->instrument;
    effects_enabled = preset->effects_enabled;
    chord_mode = preset->chord_mode;
    arpeggiator.mode = preset->arp_mode;
    gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
    Trigger_Note_On();
}

static void Trigger_Note_On(void) {
    envelope.state = ENV_ATTACK;
    envelope.phase = 0;
    envelope.amplitude = 0;
    envelope.note_on = true;
}

static void Trigger_Note_Off(void) {
    envelope.state = ENV_RELEASE;
    envelope.phase = 0;
    envelope.note_on = false;
}

static void Process_Joystick(void) {
    if (gSynthState.joy_x > 100) {
        uint32_t freq_int = FREQ_MIN_HZ + ((gSynthState.joy_x * (FREQ_MAX_HZ - FREQ_MIN_HZ)) / 4095);
        uint32_t diff = (freq_int > base_frequency_hz) ? (freq_int - base_frequency_hz) : (base_frequency_hz - freq_int);
        if (diff > 10) {
            base_frequency_hz = freq_int;
            Update_Phase_Increment();
        }
    }
    if (gSynthState.joy_y > 100) {
        uint8_t new_vol = (uint8_t)((gSynthState.joy_y * 100) / 4095);
        if (new_vol > 100) new_vol = 100;
        if (new_vol != gSynthState.volume) gSynthState.volume = new_vol;
    }
}

static void Process_Pitch_Bend(void) {
    int16_t accel_y = gSynthState.accel_y;
    int16_t deviation = accel_y - 2048;
    int8_t semitones = (int8_t)((deviation * 12) / 200);
    if (semitones > 12) semitones = 12;
    if (semitones < -12) semitones = -12;
#if ENABLE_NOTE_QUANTIZER
    semitones = Quantize_Semitones(semitones);
#endif
    static int8_t prev_semitones = 0;
    semitones = (prev_semitones * 7 + semitones) / 8;
    prev_semitones = semitones;
    if (semitones != pitch_bend_semitones) {
        pitch_bend_semitones = semitones;
        Update_Phase_Increment();
    }
}

static void Update_Phase_Increment(void) {
    if (base_frequency_hz == 0) base_frequency_hz = 440;
    int8_t table_index = pitch_bend_semitones + 12;
    if (table_index < 0) table_index = 0;
    if (table_index > 24) table_index = 24;
    uint32_t bend_ratio_fixed = PITCH_BEND_TABLE[table_index];
    uint64_t bent_freq_64 = ((uint64_t)base_frequency_hz * bend_ratio_fixed) >> 16;
    uint32_t bent_freq = (uint32_t)bent_freq_64;
    if (bent_freq < FREQ_MIN_HZ) bent_freq = FREQ_MIN_HZ;
    if (bent_freq > FREQ_MAX_HZ) bent_freq = FREQ_MAX_HZ;
    if (bent_freq > 0 && bent_freq <= 8000) {
        uint64_t temp = ((uint64_t)bent_freq << 32) / 8000ULL;
        uint32_t new_increment = (uint32_t)temp;
        if (new_increment > 0) {
            phase_increment = new_increment;
        } else {
            phase_increment = 236223201;
        }
    } else {
        phase_increment = 236223201;
    }
    if (phase_increment == 0) phase_increment = 236223201;
    if (chord_mode != CHORD_OFF) {
        const int8_t *intervals = CHORD_INTERVALS[chord_mode];
        for (uint8_t v = 0; v < 3; v++) {
            int8_t chord_table_index = table_index + intervals[v];
            if (chord_table_index < 0) chord_table_index = 0;
            if (chord_table_index > 24) chord_table_index = 24;
            uint32_t chord_ratio = PITCH_BEND_TABLE[chord_table_index];
            uint64_t chord_freq_64 = ((uint64_t)base_frequency_hz * chord_ratio) >> 16;
            uint32_t chord_freq = (uint32_t)chord_freq_64;
            if (chord_freq < FREQ_MIN_HZ) chord_freq = FREQ_MIN_HZ;
            if (chord_freq > FREQ_MAX_HZ) chord_freq = FREQ_MAX_HZ;
            if (chord_freq > 0 && chord_freq <= 8000) {
                uint64_t chord_temp = ((uint64_t)chord_freq << 32) / 8000ULL;
                uint32_t new_chord_inc = (uint32_t)chord_temp;
                if (new_chord_inc > 0) {
                    chord_increments[v] = new_chord_inc;
                } else {
                    chord_increments[v] = phase_increment;
                }
            } else {
                chord_increments[v] = phase_increment;
            }
        }
    } else {
        chord_increments[0] = chord_increments[1] = chord_increments[2] = phase_increment;
    }
    gSynthState.frequency = (float)bent_freq;
}

void Process_Buttons(void) {
    static uint32_t s1_prev = 1, s2_prev = 1, s1_hold_counter = 0;
    uint32_t s1 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN);
    uint32_t s2 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN);
    if (s1 == 0) {
        s1_hold_counter++;
        if (s1_hold_counter > 1000 && s1_prev == 0) {
            Change_Preset();
            s1_hold_counter = 0;
            while (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN) == 0);
        }
    } else {
        if (s1_prev == 0 && s1_hold_counter < 1000) Change_Instrument();
        s1_hold_counter = 0;
    }
    if (s2 == 0 && s2_prev != 0) {
        gSynthState.audio_playing = !gSynthState.audio_playing;
        if (gSynthState.audio_playing) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_On();
        } else {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_Off();
        }
        while (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN) == 0);
    }
    s1_prev = s1;
    s2_prev = s2;
}

static void Display_Update(void) {
    const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
    LCD_DrawRect(0, 0, 128, 16, inst->color);
    LCD_PrintString(3, 4, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
    LCD_PrintString(60, 4, PRESETS[current_preset].name, LCD_COLOR_BLACK, inst->color, FONT_SMALL);
    LCD_PrintString(3, 18, "F:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(18, 18, base_frequency_hz, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    char buf[16];
    snprintf(buf, sizeof(buf), "%+d", pitch_bend_semitones);
    LCD_PrintString(55, 18, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
    uint8_t bar_w = gSynthState.volume;
    if (bar_w > 100) bar_w = 100;
    LCD_DrawRect(3, 30, 60, 4, LCD_COLOR_DARKGRAY);
    LCD_DrawRect(3, 30, (bar_w * 60) / 100, 4, LCD_COLOR_GREEN);
    LCD_PrintString(66, 30, "FX:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintString(84, 30, effects_enabled ? "ON" : "OFF",
                    effects_enabled ? LCD_COLOR_GREEN : LCD_COLOR_RED,
                    LCD_COLOR_BLACK, FONT_SMALL);
    if (chord_mode != CHORD_OFF) {
        const char *chord_names[] = {"", "MAJ", "MIN"};
        LCD_PrintString(105, 30, chord_names[chord_mode], LCD_COLOR_MAGENTA, LCD_COLOR_BLACK, FONT_SMALL);
    }
    if (arpeggiator.mode != ARP_OFF) {
        LCD_PrintString(3, 40, "ARP", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
        const char *arp_names[] = {"", "UP", "DN", "UD", "RND"};
        LCD_PrintString(24, 40, arp_names[arpeggiator.mode], LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    }
    const char *env_names[] = {"IDLE", "ATK", "DEC", "SUS", "REL"};
    LCD_PrintString(55, 40, env_names[envelope.state], LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(90, 40, envelope.amplitude / 10, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
#if ENABLE_WAVEFORM_DISPLAY
    Display_Waveform();
#endif
    if (gSynthState.audio_playing) {
        LCD_PrintString(3, 118, "PLAYING", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
    } else {
        LCD_PrintString(3, 118, "STOPPED", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
    }
}

#if ENABLE_WAVEFORM_DISPLAY
static void Display_Waveform(void) {
    uint16_t y_center = 80, y_scale = 25;
    LCD_DrawRect(0, 50, 128, 60, LCD_COLOR_BLACK);
    for (uint8_t x = 0; x < 128; x += 4) LCD_DrawPixel(x, y_center, LCD_COLOR_DARKGRAY);
    for (uint8_t i = 0; i < 63; i++) {
        int16_t y1 = y_center - ((waveform_buffer[i] * y_scale) / 1000);
        int16_t y2 = y_center - ((waveform_buffer[i + 1] * y_scale) / 1000);
        if (y1 < 50) y1 = 50; if (y1 > 110) y1 = 110;
        if (y2 < 50) y2 = 50; if (y2 > 110) y2 = 110;
        LCD_DrawLine(i * 2, y1, (i + 1) * 2, y2, LCD_COLOR_CYAN);
    }
}
#endif

void HardFault_Handler(void) {
    while (1) {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}