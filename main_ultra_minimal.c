/**
 * @file main_v10.5_DUAL_ADC.c
 * @brief ULTIMATE Synthesizer - DUAL ADC VERSION (Best Audio!)
 * @version 10.5.0
 * 
 * CONFIGURATION:
 * - ADC0 (ADC_JOY): Joystick Y → Volume
 * - ADC1 (ADC_ACCEL): Accelerometer X, Y, Z
 *   - Accel X → Frequency (with smoothing!)
 *   - Accel Y → Pitch bend
 *   - Accel Z → Unused
 * 
 * CONTROLS:
 * - Tilt LEFT/RIGHT: Frequency
 * - Joystick UP/DOWN: Volume
 * - Tilt FORWARD/BACK: Pitch bend
 * - S1: Change instrument
 * - S2: Play/Stop
 * - JOY_SEL: Toggle effects
 * 
 * WHY DUAL ADC IS BETTER:
 * - Separate sampling reduces crosstalk
 * - Better audio quality
 * - More stable readings
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "main.h"
#include "lcd_driver.h"
#include <stdio.h>

//=============================================================================
// CONFIGURATION
//=============================================================================
#define ENABLE_WAVEFORM_DISPLAY 1
#define LED_BRIGHTNESS_LOW 2

// Accelerometer X smoothing filter
#define ACCEL_X_SAMPLES 16
static int16_t accel_x_buffer[ACCEL_X_SAMPLES] = {2048};
static uint8_t accel_x_index = 0;

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

typedef struct {
    uint16_t attack_samples;
    uint16_t decay_samples;
    uint16_t sustain_level;
    uint16_t release_samples;
} ADSR_Profile_t;

typedef struct {
    const char* name;
    ADSR_Profile_t adsr;
    Waveform_t waveform;
    uint16_t color;
} InstrumentProfile_t;

static const InstrumentProfile_t INSTRUMENTS[INSTRUMENT_COUNT] = {
    {"PIANO", {80, 1600, 700, 800}, WAVE_TRIANGLE, LCD_COLOR_CYAN},
    {"ORGAN", {0, 0, 900, 400}, WAVE_SINE, LCD_COLOR_RED},
    {"STRINGS", {2400, 3200, 850, 16000}, WAVE_SAWTOOTH, LCD_COLOR_YELLOW},
    {"BASS", {160, 800, 900, 800}, WAVE_SINE, LCD_COLOR_BLUE},
    {"LEAD", {40, 1200, 850, 1600}, WAVE_SQUARE, LCD_COLOR_GREEN}
};

typedef enum {
    ENV_IDLE = 0,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvelopeState_t;

typedef struct {
    EnvelopeState_t state;
    uint32_t phase;
    uint16_t amplitude;
    bool note_on;
} Envelope_t;

//=============================================================================
// PITCH BEND TABLE
//=============================================================================
static const uint32_t PITCH_BEND_TABLE[25] = {
    32768, 34675, 36781, 38967, 41285, 43742, 46341, 49091, 51998,
    55041, 58255, 61644, 65536, 69433, 73533, 77841, 82366, 87111,
    92123, 97549, 103397, 109681, 116411, 123596, 131072
};

//=============================================================================
// SINE TABLE
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
// GLOBAL STATE
//=============================================================================
volatile SynthState_t gSynthState;
static uint32_t phase = 0;
static uint32_t phase_increment = 0;

static Instrument_t current_instrument = INSTRUMENT_ORGAN;
static Envelope_t envelope = {0};
static bool effects_enabled = true;

static uint32_t base_frequency_hz = 440;
static int8_t pitch_bend_semitones = 0;

static uint8_t led_brightness = LED_BRIGHTNESS_LOW;
static uint32_t led_pwm_counter = 0;

#if ENABLE_WAVEFORM_DISPLAY
static int16_t waveform_buffer[64] = {0};
static uint8_t waveform_write_index = 0;
#endif

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
static void Process_Accelerometer_X(void);
static void Process_Joystick_Y(void);
static void Process_Pitch_Bend(void);
static void Process_Envelope(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static void Change_Instrument(void);
static void Trigger_Note_On(void);
static void Trigger_Note_Off(void);
static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform);
void Process_Buttons(void);
static void Display_Update(void);
static void Display_Waveform(void);
static void LED_PWM_Update(void);

//=============================================================================
// LED CONTROL
//=============================================================================
static void LED_PWM_Update(void) {
    led_pwm_counter++;
    if (led_pwm_counter >= 10) led_pwm_counter = 0;
    
    if (led_pwm_counter < led_brightness) {
        if (gSynthState.audio_playing) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        }
        if (effects_enabled) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
        }
    } else {
        if (led_brightness < 10) {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
        }
    }
}

//=============================================================================
// MAIN
//=============================================================================
int main(void) {
    SYSCFG_DL_init();
    
    memset((void*)&gSynthState, 0, sizeof(SynthState_t));
    gSynthState.frequency = 440.0f;
    gSynthState.volume = 50;
    gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
    gSynthState.audio_playing = 1;
    
    // Initialize accelerometer X filter with center value
    for (uint8_t i = 0; i < ACCEL_X_SAMPLES; i++) {
        accel_x_buffer[i] = 2048;
    }
    
    // base_frequency_hz = 440;
    pitch_bend_semitones = 0;
    
    envelope.state = ENV_ATTACK;
    envelope.phase = 0;
    envelope.amplitude = 0;
    envelope.note_on = true;
    
    Update_Phase_Increment();
    
   
    LCD_Init();
    DL_GPIO_setPins(LCD_BL_PORT, LCD_BL_GIPO_LCD_BACKLIGHT_PIN);
    
    LCD_FillScreen(LCD_COLOR_BLACK);
    LCD_PrintString(5, 10, "ULTIMATE", LCD_COLOR_MAGENTA, LCD_COLOR_BLACK, FONT_LARGE);
    LCD_PrintString(10, 40, "Synthesizer", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_MEDIUM);
    LCD_PrintString(25, 70, "v10.5.0", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintString(5, 100, "DUAL ADC MODE", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    DL_Common_delayCycles(20000);
    LCD_FillScreen(LCD_COLOR_BLACK);
    
    // Enable both ADC interrupts
    NVIC_EnableIRQ(ADC0_INT_IRQn);
    NVIC_EnableIRQ(ADC1_INT_IRQn);
    NVIC_EnableIRQ(TIMG7_INT_IRQn);
    __enable_irq();
    
    DL_TimerG_startCounter(TIMER_SAMPLE_INST);
    
    // Start both ADCs
    DL_ADC12_enableConversions(ADC_JOY_INST);
    DL_ADC12_startConversion(ADC_JOY_INST);
    
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
    DL_ADC12_startConversion(ADC_ACCEL_INST);
    
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    
    uint32_t loop_counter = 0;
    uint32_t display_counter = 0;
    
    while (1) {
        if (loop_counter % 1000 == 0) {
            Process_Accelerometer_X();
        }
        
        if (loop_counter % 2000 == 0) {
            Process_Joystick_Y();
        }
        
        if (loop_counter % 3000 == 0) {
            Process_Pitch_Bend();
        }
        
        if (loop_counter % 100 == 0) {
            Process_Buttons();
        }
        
        if (loop_counter % 500 == 0) {
            LED_PWM_Update();
        }
        
        if (display_counter++ >= 200000) {
            Display_Update();
            display_counter = 0;
        }
        
        loop_counter++;
    }
}

//=============================================================================
// ADC INTERRUPT HANDLERS
//=============================================================================
void ADC0_IRQHandler(void) {
    if (DL_ADC12_getPendingInterrupt(ADC_JOY_INST) == 
        DL_ADC12_IIDX_MEM0_RESULT_LOADED) {
        
        // joy_x er allerede i buffer via DMA
        // Les joy_y manuelt
        gSynthState.joy_y = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_1);
    }
}

void ADC1_IRQHandler(void) {
    gSynthState.adc1_count++;
    
    if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) == DL_ADC12_IIDX_MEM2_RESULT_LOADED) {
        gSynthState.accel_x = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
        gSynthState.accel_y = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
        gSynthState.accel_z = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
    }
}

//=============================================================================
// SENSOR PROCESSING WITH SMOOTHING
//=============================================================================
static void Process_Accelerometer_X(void) {
    // Add new sample to circular buffer
    accel_x_buffer[accel_x_index++] = gSynthState.accel_x;
    if (accel_x_index >= ACCEL_X_SAMPLES) accel_x_index = 0;
    
    // Calculate average (smoothed value)
    int32_t sum = 0;
    for (uint8_t i = 0; i < ACCEL_X_SAMPLES; i++) {
        sum += accel_x_buffer[i];
    }
    int16_t accel_x_smooth = (int16_t)(sum / ACCEL_X_SAMPLES);
    
    // Map to frequency with deadzone
    int16_t deviation = accel_x_smooth - 2048;
    
    // Deadzone: ±200 ADC counts
    if (deviation > -400 && deviation < 400) {
        deviation = 0;  // Center zone = 440 Hz
    }
    

    
    // Limit range
    if (freq_int < FREQ_MIN_HZ) freq_int = FREQ_MIN_HZ;
    if (freq_int > FREQ_MAX_HZ) freq_int = FREQ_MAX_HZ;
    
    // Only update if changed significantly (>5 Hz)
    if (freq_int > base_frequency_hz + 2 || freq_int < base_frequency_hz - 2) {
        base_frequency_hz = freq_int;
        Update_Phase_Increment();
    }
}

static void Process_Joystick_Y(void) {
    if (gSynthState.joy_y > 200) {
        uint32_t mapped = gSynthState.joy_y - 200;
        uint8_t new_vol = (uint8_t)((mapped * 80) / 3895);  // Max 80% to prevent clipping
        if (new_vol > 80) new_vol = 80;
        if (new_vol < 10) new_vol = 10;
        
        if (new_vol != gSynthState.volume) {
            gSynthState.volume = new_vol;
        }
    }
}

static void Process_Pitch_Bend(void) {
    int16_t accel_y = gSynthState.accel_y;
    int16_t deviation = accel_y - 2048;
    int8_t semitones = (int8_t)((deviation * 12) / 200);
    
    if (semitones > 12) semitones = 12;
    if (semitones < -12) semitones = -12;
    
    // Smooth pitch bend
    static int8_t prev_semitones = 0;
    semitones = (prev_semitones * 7 + semitones) / 8;
    prev_semitones = semitones;
    
    if (semitones != pitch_bend_semitones) {
        pitch_bend_semitones = semitones;
        Update_Phase_Increment();
    }
}

//=============================================================================
// TIMER INTERRUPT
//=============================================================================
void TIMG7_IRQHandler(void) {
    if (DL_Timer_getPendingInterrupt(TIMER_SAMPLE_INST) == DL_TIMER_IIDX_ZERO) {
        gSynthState.timer_count++;
        
        Process_Envelope();
        
        if (gSynthState.audio_playing) {
            Generate_Audio_Sample();
        } else {
            DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        }
    }
}

//=============================================================================
// AUDIO GENERATION (SIMPLIFIED FOR CLEAN SOUND)
//=============================================================================
static void Generate_Audio_Sample(void) {
    if (gSynthState.volume == 0 || envelope.amplitude == 0) {
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        phase += phase_increment;
        gSynthState.audio_samples_generated++;
        return;
    }

    const InstrumentProfile_t* inst = &INSTRUMENTS[current_instrument];
    
    uint8_t index = (uint8_t)((phase >> 12) & 0xFF);
    int16_t sample = Generate_Waveform(index, inst->waveform);
    
    // Apply envelope
    sample = (int16_t)(((int32_t)sample * envelope.amplitude) / 1000);
    
    // Apply volume (max 80% to prevent clipping)
    sample = (int16_t)(((int32_t)sample * gSynthState.volume) / 100);
    
    phase += phase_increment;
    
#if ENABLE_WAVEFORM_DISPLAY
    static uint8_t waveform_decimate_counter = 0;
    if (++waveform_decimate_counter >= 125) {
        waveform_decimate_counter = 0;
        waveform_buffer[waveform_write_index++] = sample;
        if (waveform_write_index >= 64) waveform_write_index = 0;
    }
#endif
    
    // Convert to PWM (with soft limiting)
    int32_t val = 2048 + (sample * 2);
    
    // Soft clip to prevent harsh distortion
    if (val < 512) val = 512;
    if (val > 3584) val = 3584;
    
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, (uint16_t)val, DL_TIMER_CC_0_INDEX);
    gSynthState.audio_samples_generated++;
}

static void Process_Envelope(void) {
    const ADSR_Profile_t* adsr = &INSTRUMENTS[current_instrument].adsr;
    
    switch (envelope.state) {
        case ENV_IDLE:
            envelope.amplitude = 0;
            break;
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

static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform) {
    int16_t sample = 0;
    switch (waveform) {
        case WAVE_SINE:
            sample = sine_table[index];
            break;
        case WAVE_SQUARE:
            sample = (index < 128) ? 800 : -800;  // Reduced amplitude
            break;
        case WAVE_SAWTOOTH:
            sample = (int16_t)(((int32_t)index * 1600 / 256) - 800);  // Reduced
            break;
        case WAVE_TRIANGLE:
            sample = (index < 128) ? (int16_t)(((int32_t)index * 1600 / 128) - 800)
                                   : (int16_t)(800 - ((int32_t)(index - 128) * 1600 / 128));
            break;
        default:
            sample = sine_table[index];
            break;
    }
    return sample;
}

static void Change_Instrument(void) {
    current_instrument = (current_instrument + 1) % INSTRUMENT_COUNT;
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

static void Update_Phase_Increment(void) {
    if (base_frequency_hz == 0) base_frequency_hz = 440;
    
    int8_t table_index = pitch_bend_semitones + 12;
    if (table_index < 0) table_index = 0;
    if (table_index > 24) table_index = 24;
    
    uint32_t bend_ratio = PITCH_BEND_TABLE[table_index];
    uint64_t bent_freq = ((uint64_t)base_frequency_hz * bend_ratio) >> 16;
    uint32_t freq = (uint32_t)bent_freq;
    
    if (freq < FREQ_MIN_HZ) freq = FREQ_MIN_HZ;
    if (freq > FREQ_MAX_HZ) freq = FREQ_MAX_HZ;
    
    if (freq > 0 && freq <= 8000) {
        uint64_t temp = ((uint64_t)freq << 32) / 8000ULL;
        phase_increment = (uint32_t)temp;
    } else {
        phase_increment = 236223201;
    }
    
    if (phase_increment == 0) phase_increment = 236223201;
    
    gSynthState.frequency = (float)freq;
}

//=============================================================================
// BUTTON HANDLING (ULTRA-SIMPLE)
//=============================================================================
void Process_Buttons(void) {
    static uint32_t s1_prev = 1, s2_prev = 1, joy_prev = 1;
    
    uint32_t s1 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN);
    uint32_t s2 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN);
    
    #ifdef GPIO_BUTTONS_JOY_SEL_PIN
    uint32_t joy = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
    #else
    uint32_t joy = 1;
    #endif
    
    // S1: Change instrument (simple edge detection)
    if (s1 == 0 && s1_prev != 0) {
        delay_cycles(5000);  // Debounce
        if (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN) == 0) {
            Change_Instrument();
        }
    }
    
    // S2: Play/Stop
    if (s2 == 0 && s2_prev != 0) {
        delay_cycles(5000);
        if (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN) == 0) {
            gSynthState.audio_playing = !gSynthState.audio_playing;
            if (gSynthState.audio_playing) {
                Trigger_Note_On();
            } else {
                Trigger_Note_Off();
            }
        }
    }
    
    // JOY_SEL: Toggle effects
    #ifdef GPIO_BUTTONS_JOY_SEL_PIN
    if (joy == 0 && joy_prev != 0) {
        delay_cycles(5000);
        if (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN) == 0) {
            effects_enabled = !effects_enabled;
        }
    }
    #endif
    
    s1_prev = s1;
    s2_prev = s2;
    joy_prev = joy;
}

//=============================================================================
// DISPLAY
//=============================================================================
static void Display_Update(void) {
    const InstrumentProfile_t* inst = &INSTRUMENTS[current_instrument];
    
    LCD_DrawRect(0, 0, 128, 16, inst->color);
    LCD_PrintString(3, 4, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
    
    // Show ADC mode
    LCD_PrintString(60, 4, "2ADC", LCD_COLOR_BLACK, inst->color, FONT_SMALL);
    
    LCD_PrintString(3, 18, "F:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(18, 18, base_frequency_hz, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "%+d", pitch_bend_semitones);
    LCD_PrintString(70, 18, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
    
    uint8_t bar_w = gSynthState.volume;
    if (bar_w > 100) bar_w = 100;
    LCD_DrawRect(3, 30, 80, 4, LCD_COLOR_DARKGRAY);
    LCD_DrawRect(3, 30, (bar_w * 80) / 100, 4, LCD_COLOR_GREEN);
    
    LCD_PrintString(90, 30, effects_enabled ? "FX" : "--",
                    effects_enabled ? LCD_COLOR_GREEN : LCD_COLOR_RED,
                    LCD_COLOR_BLACK, FONT_SMALL);
    
    const char* env_names[] = {"IDLE", "ATK", "DEC", "SUS", "REL"};
    LCD_PrintString(3, 40, env_names[envelope.state],
                    LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
    LCD_PrintNumber(50, 40, envelope.amplitude / 10,
                    LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
    
#if ENABLE_WAVEFORM_DISPLAY
    Display_Waveform();
#endif
    
    LCD_PrintString(3, 118, gSynthState.audio_playing ? "PLAYING" : "STOPPED",
                    gSynthState.audio_playing ? LCD_COLOR_GREEN : LCD_COLOR_RED,
                    LCD_COLOR_BLACK, FONT_SMALL);
}

#if ENABLE_WAVEFORM_DISPLAY
static void Display_Waveform(void) {
    uint16_t y_center = 80;
    uint16_t y_scale = 25;
    
    LCD_DrawRect(0, 50, 128, 60, LCD_COLOR_BLACK);
    
    for (uint8_t x = 0; x < 128; x += 4) {
        LCD_DrawPixel(x, y_center, LCD_COLOR_DARKGRAY);
    }
    
    for (uint8_t i = 0; i < 63; i++) {
        int16_t y1 = y_center - ((waveform_buffer[i] * y_scale) / 1000);
        int16_t y2 = y_center - ((waveform_buffer[i+1] * y_scale) / 1000);
        
        if (y1 < 50) y1 = 50;
        if (y1 > 110) y1 = 110;
        if (y2 < 50) y2 = 50;
        if (y2 > 110) y2 = 110;
        
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