/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer - Complete with LCD Display
 * @version 5.0.0 - Full synth with LCD, loud audio, all features working
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "main.h"
#include "lcd/lcd_driver.h"
// Uncomment if you have lcd_driver.h
// #include "lcd/lcd_driver.h"

#define SAMPLE_RATE 8000
#define VOLUME_DEFAULT 75

//=============================================================================
// LCD PLACEHOLDER FUNCTIONS (if you don't have lcd_driver yet)
//=============================================================================
#ifndef LCD_DRIVER_H  // If you don't have LCD driver, use these stubs

typedef uint16_t Color_t;
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

void LCD_Init(void) {
    // TODO: Implement LCD initialization
    // For now, just delay to simulate init time
    DL_Common_delayCycles(80000000 / 10);  // 100ms
}

void LCD_Clear(Color_t color) {
    // TODO: Implement LCD clear
    (void)color;
}

void LCD_DrawString(uint16_t x, uint16_t y, const char* str, Color_t color) {
    // TODO: Implement LCD string drawing
    (void)x; (void)y; (void)str; (void)color;
}

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color_t color) {
    // TODO: Implement LCD rectangle
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color_t color) {
    // TODO: Implement LCD filled rectangle
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

#endif // LCD_DRIVER_H

//=============================================================================
// GLOBAL VARIABLES
//=============================================================================
volatile SynthState_t gSynthState; 

static uint32_t phase = 0;
static uint32_t phase_increment = 0;
static bool lcd_enabled = false;  // Set to true when LCD driver is ready

// Sine lookup table (256 entries, -1000 to +1000 range)
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

static const char* waveform_names[] = {
    "SINE",
    "SQUARE", 
    "SAWTOOTH",
    "TRIANGLE"
};

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
static void Process_Input(void);
static void Update_Display(void);
static void Generate_Audio_Sample(void);
static void Update_Audio_Params(void);
static void Process_Buttons(void);
static void Draw_UI(void);

//=============================================================================
// MAIN FUNCTION
//=============================================================================
int main(void) {
    SYSCFG_DL_init();
    
    // Initialize state
    memset((void*)&gSynthState, 0, sizeof(SynthState_t));
    gSynthState.frequency = FREQ_DEFAULT;
    gSynthState.volume = VOLUME_DEFAULT;
    gSynthState.waveform = WAVE_SINE;
    gSynthState.mode = MODE_SYNTH;
    gSynthState.audio_playing = 1;
    
    Update_Audio_Params();
    
    // Initialize LCD
    LCD_Init();
    LCD_Clear(COLOR_BLACK);
    lcd_enabled = true;
    
    // Draw initial UI
    Draw_UI();
    
    // Start PWM for audio
    DL_TimerG_startCounter(PWM_AUDIO_INST);
    
    // Start ADCs
    DL_ADC12_enableConversions(ADC_MIC_JOY_INST);
    DL_ADC12_startConversion(ADC_MIC_JOY_INST);
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
    DL_ADC12_startConversion(ADC_ACCEL_INST);
    
    // Enable interrupts
    NVIC_EnableIRQ(ADC0_INT_IRQn);
    NVIC_EnableIRQ(ADC1_INT_IRQn);
    NVIC_EnableIRQ(TIMG7_INT_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    
    // LED feedback - green = ready
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    
    uint32_t loop_counter = 0;
    uint32_t last_update = 0;
    
    while (1) {
        // Process input less frequently to avoid float crashes
        if (loop_counter % 10000 == 0) {
            Process_Input();
        }
        
        // Manual button polling
        if (loop_counter % 5000 == 0) {
            Process_Buttons();
        }
        
        // Update display every 50000 loops (~50-100ms)
        if (lcd_enabled && (loop_counter - last_update) > 50000) {
            if (gSynthState.display_update_needed) {
                Update_Display();
                gSynthState.display_update_needed = false;
                last_update = loop_counter;
            }
        }
        
        // Heartbeat LED
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
        
        if (gSynthState.audio_playing) {
            Generate_Audio_Sample();
        } else {
            // Mute output
            DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        }
    }
}

void ADC0_IRQHandler(void) {
    gSynthState.adc0_count++;
    
    switch (DL_ADC12_getPendingInterrupt(ADC_MIC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            gSynthState.mic_level = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
            break;
            
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            gSynthState.joy_y = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_1);
            break;
            
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            gSynthState.joy_x = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_2);
            gSynthState.display_update_needed = true;
            break;
            
        default:
            break;
    }
}

void ADC1_IRQHandler(void) {
    gSynthState.adc1_count++;
    
    switch (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            gSynthState.accel_x = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
            break;
            
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            gSynthState.accel_y = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
            break;
            
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            gSynthState.accel_z = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
            break;
            
        default:
            break;
    }
}

void GPIOA_IRQHandler(void) {
    switch (DL_GPIO_getPendingInterrupt(GPIO_BUTTONS_PORT)) {
        case GPIO_BUTTONS_S1_IIDX:
            gSynthState.btn_s1 = 1;
            gSynthState.waveform = (gSynthState.waveform + 1) % WAVE_COUNT;
            gSynthState.display_update_needed = true;
            DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
            break;
            
        case GPIO_BUTTONS_S2_IIDX:
            gSynthState.btn_s2 = 1;
            gSynthState.audio_playing = !gSynthState.audio_playing;
            gSynthState.display_update_needed = true;
            
            if (gSynthState.audio_playing) {
                DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            } else {
                DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            }
            break;
            
        case GPIO_BUTTONS_JOY_SEL_IIDX:
            gSynthState.joy_pressed = 1;
            gSynthState.frequency = FREQ_DEFAULT;
            Update_Audio_Params();
            gSynthState.display_update_needed = true;
            break;
            
        default:
            break;
    }
}

//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

static void Update_Audio_Params(void) {
    // Calculate phase increment for DDS
    phase_increment = (uint32_t)((gSynthState.frequency * 4294967296.0) / SAMPLE_RATE);
}

static void Generate_Audio_Sample(void) {
    if (gSynthState.volume == 0) { 
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        return;
    }

    int16_t sample = 0;
    uint8_t index = (uint8_t)((phase >> 24) & 0xFF);
    
    // Generate waveform
    switch (gSynthState.waveform) {
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
            if (index < 128) {
                sample = (int16_t)(((int32_t)index * 2000 / 128) - 1000);
            } else {
                sample = (int16_t)(1000 - ((int32_t)(index - 128) * 2000 / 128));
            }
            break;
            
        default:
            sample = sine_table[index];
            break;
    }

    // Apply volume (integer math only!)
    sample = (int16_t)((sample * gSynthState.volume) / 100);
    
    // Convert to 12-bit PWM value with MAXIMUM amplitude
    // Map -1000..1000 to 0..4095 for maximum volume
    int32_t val = 2048 + (sample * 2);  // Scale up for louder output
    
    // Clamp to valid range
    if(val < 0) val = 0;
    if(val > 4095) val = 4095;
    
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, (uint16_t)val, DL_TIMER_CC_0_INDEX);
    
    // Update phase
    phase += phase_increment;
}

static void Process_Input(void) {
    // Use integer math to avoid floating-point crashes
    
    // Joystick X -> Frequency (100 Hz - 2000 Hz)
    if (gSynthState.joy_x > 100) {
        // Map 0-4095 to 100-2000 Hz
        uint32_t freq_int = 100 + ((gSynthState.joy_x * 1900) / 4095);
        uint32_t curr_freq = (uint32_t)gSynthState.frequency;
        uint32_t diff = (freq_int > curr_freq) ? (freq_int - curr_freq) : (curr_freq - freq_int);
        
        if (diff > 10) {  // Only update if changed significantly
            gSynthState.frequency = (float)freq_int;
            Update_Audio_Params();
            gSynthState.display_update_needed = true;
        }
    }
    
    // Joystick Y -> Volume (0 - 100%)
    if (gSynthState.joy_y > 100) {
        uint8_t new_vol = (uint8_t)((gSynthState.joy_y * 100) / 4095);
        if (new_vol != gSynthState.volume) {
            gSynthState.volume = new_vol;
            gSynthState.display_update_needed = true;
        }
    }
}

static void Process_Buttons(void) {
    static uint32_t s1_prev = 1, s2_prev = 1, joy_prev = 1;
    
    uint32_t s1 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN);
    uint32_t s2 = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN);
    uint32_t joy = DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
    
    // S1: Cycle waveform (falling edge)
    if (s1 == 0 && s1_prev != 0) {
        gSynthState.waveform = (gSynthState.waveform + 1) % WAVE_COUNT;
        gSynthState.display_update_needed = true;
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        
        // Debounce
        while (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN) == 0) {
            __NOP();
        }
    }
    
    // S2: Toggle audio on/off
    if (s2 == 0 && s2_prev != 0) {
        gSynthState.audio_playing = !gSynthState.audio_playing;
        gSynthState.display_update_needed = true;
        
        if (gSynthState.audio_playing) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        }
        
        while (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN) == 0) {
            __NOP();
        }
    }
    
    // JOY_SEL: Reset to default frequency
    if (joy == 0 && joy_prev != 0) {
        gSynthState.frequency = FREQ_DEFAULT;
        Update_Audio_Params();
        gSynthState.display_update_needed = true;
        
        while (DL_GPIO_readPins(GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN) == 0) {
            __NOP();
        }
    }
    
    s1_prev = s1;
    s2_prev = s2;
    joy_prev = joy;
}

static void Draw_UI(void) {
    if (!lcd_enabled) return;
    
    char buffer[32];
    
    // Title
    LCD_DrawString(5, 5, "MSPM0 SYNTH", COLOR_CYAN);
    LCD_DrawRect(0, 0, 127, 20, COLOR_CYAN);
    
    // Waveform
    LCD_DrawString(5, 25, "Wave:", COLOR_YELLOW);
    LCD_DrawString(50, 25, waveform_names[gSynthState.waveform], COLOR_WHITE);
    
    // Frequency
    LCD_DrawString(5, 45, "Freq:", COLOR_YELLOW);
    sprintf(buffer, "%d Hz", (int)gSynthState.frequency);
    LCD_DrawString(50, 45, buffer, COLOR_WHITE);
    
    // Volume
    LCD_DrawString(5, 65, "Vol:", COLOR_YELLOW);
    sprintf(buffer, "%d %%", gSynthState.volume);
    LCD_DrawString(50, 65, buffer, COLOR_WHITE);
    
    // Volume bar
    uint16_t bar_width = (gSynthState.volume * 100) / 100;
    LCD_FillRect(5, 85, bar_width, 10, COLOR_GREEN);
    LCD_DrawRect(5, 85, 100, 10, COLOR_WHITE);
    
    // Status
    if (gSynthState.audio_playing) {
        LCD_DrawString(5, 105, "Status: PLAYING", COLOR_GREEN);
    } else {
        LCD_DrawString(5, 105, "Status: MUTED  ", COLOR_RED);
    }
}

static void Update_Display(void) {
    if (!lcd_enabled) return;
    
    char buffer[32];
    
    // Update waveform (erase old and draw new)
    LCD_FillRect(50, 25, 70, 10, COLOR_BLACK);
    LCD_DrawString(50, 25, waveform_names[gSynthState.waveform], COLOR_WHITE);
    
    // Update frequency
    LCD_FillRect(50, 45, 70, 10, COLOR_BLACK);
    sprintf(buffer, "%d Hz", (int)gSynthState.frequency);
    LCD_DrawString(50, 45, buffer, COLOR_WHITE);
    
    // Update volume
    LCD_FillRect(50, 65, 70, 10, COLOR_BLACK);
    sprintf(buffer, "%d %%", gSynthState.volume);
    LCD_DrawString(50, 65, buffer, COLOR_WHITE);
    
    // Update volume bar
    LCD_FillRect(5, 85, 100, 10, COLOR_BLACK);
    uint16_t bar_width = (gSynthState.volume * 100) / 100;
    LCD_FillRect(5, 85, bar_width, 10, COLOR_GREEN);
    LCD_DrawRect(5, 85, 100, 10, COLOR_WHITE);
    
    // Update status
    LCD_FillRect(5, 105, 120, 10, COLOR_BLACK);
    if (gSynthState.audio_playing) {
        LCD_DrawString(5, 105, "Status: PLAYING", COLOR_GREEN);
    } else {
        LCD_DrawString(5, 105, "Status: MUTED  ", COLOR_RED);
    }
    
    // LED feedback based on frequency
    if (gSynthState.frequency < 300.0f) {
        DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
    } else if (gSynthState.frequency < 1000.0f) {
        DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
    } else {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
    }
}