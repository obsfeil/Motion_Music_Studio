/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer with LCD Display
 *
 * PHASE 1 + PHASE 2 INTEGRATED
 * - Audio synthesis (Phase 1) ✅
 * - LCD display (Phase 2) ✅
 *
 * Features:
 * - 12-bit PWM audio output
 * - 4 waveforms with visual preview
 * - Real-time frequency/volume display
 * - Joystick control with visual feedback
 * - RGB LED status indicators
 * 
 * @version 1.1.0
 * @date 2025-12-17
 * @note Fixed volatile declarations, timer wrap-around, and integer overflow
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TI DriverLib
#include "lcd/lcd_driver.h"
#include "main.h"
#include "ti_msp_dl_config.h" // Project headers

//=============================================================================
// GLOBAL STATE
//=============================================================================

SynthState_t g_synthState = {
    .waveform = WAVE_SINE,
    .mode = MODE_SYNTH,
    .frequency = FREQ_DEFAULT,
    .volume = VOLUME_DEFAULT,
    .pitchBend = 0,
    .audio_playing = false,
    .display_update_needed = false,
    .joy_x = JOY_ADC_CENTER,
    .joy_y = JOY_ADC_CENTER,
    .joy_pressed = false,
    .btn_s1 = false,
    .btn_s2 = false,
    .accel_x = ACCEL_ZERO_G,
    .accel_y = ACCEL_ZERO_G,
    .accel_z = ACCEL_ZERO_G,
    .mic_level = 0,
    .light_lux = 0.0f
};

//=============================================================================
// AUDIO SYNTHESIS
//=============================================================================

// Sine wave lookup table (256 samples)
static const int16_t sine_table[256] = {
    0,    50,   100,  150,  200,  249,  297,  344,  391,  436,  481,  524,
    565,  606,  644,  681,  716,  749,  780,  809,  836,  861,  883,  903,
    921,  936,  949,  959,  967,  972,  975,  975,  973,  968,  961,  951,
    939,  924,  907,  887,  865,  841,  814,  785,  754,  721,  686,  649,
    611,  571,  529,  486,  442,  397,  350,  303,  255,  206,  157,  108,
    58,   9,    -41,  -90,  -139, -187, -235, -282, -328, -373, -417, -460,
    -501, -541, -579, -616, -651, -684, -715, -744, -771, -796, -819, -839,
    -858, -874, -888, -900, -909, -916, -921, -923, -923, -920, -915, -908,
    -898, -886, -872, -855, -836, -815, -792, -766, -739, -709, -678, -645,
    -610, -573, -535, -495, -454, -412, -368, -323, -278, -231, -184, -136,
    -88,  -39,  9,    58,   107,  155,  203,  250,  296,  341,  385,  428,
    470,  510,  549,  586,  622,  656,  688,  718,  747,  773,  798,  820,
    841,  859,  875,  889,  901,  911,  918,  923,  926,  927,  925,  922,
    916,  908,  898,  886,  871,  855,  836,  816,  793,  769,  743,  715,
    686,  655,  622,  588,  552,  515,  477,  438,  397,  356,  313,  270,
    226,  182,  137,  91,   46,   0,    -46,  -91,  -137, -182, -226, -270,
    -313, -356, -397, -438, -477, -515, -552, -588, -622, -655, -686, -715,
    -743, -769, -793, -816, -836, -855, -871, -886, -898, -908, -916, -922,
    -925, -927, -926, -923, -918, -911, -901, -889, -875, -859, -841, -820,
    -798, -773, -747, -718, -688, -656, -622, -586, -549, -510, -470, -428,
    -385, -341, -296, -250, -203, -155, -107, -58
};

static uint32_t phase = 0;
static uint32_t phase_increment = 0;

/**
 * @brief Update phase increment based on current frequency
 */
static void Update_Phase_Increment(void) {
    // Use double precision to avoid rounding errors
    phase_increment = (uint32_t)((g_synthState.frequency * 4294967296.0) / SAMPLE_RATE);
}

/**
 * @brief Generate single audio sample and update PWM duty cycle
 * @note Called from timer interrupt at SAMPLE_RATE Hz
 */
static void Generate_Audio_Sample(void) {
    // Center PWM if audio is stopped
    if (!g_synthState.audio_playing) {
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        return;
    }

    // Extract waveform index from phase accumulator (top 8 bits)
    uint8_t index = (uint8_t)((phase >> 24) & 0xFF);  // Guaranteed 0-255
    int16_t sample = 0;

    // Generate waveform sample
    switch (g_synthState.waveform) {
        case WAVE_SINE:
            sample = sine_table[index];
            break;

        case WAVE_SQUARE:
            sample = (index < 128) ? 1000 : -1000;
            break;

        case WAVE_SAWTOOTH:
            sample = (int16_t)((index * 8) - 1024);
            break;

        case WAVE_TRIANGLE:
            if (index < 128) {
                sample = (int16_t)((index * 16) - 1024);
            } else {
                sample = (int16_t)(1024 - ((index - 128) * 16));
            }
            break;

        default:
            sample = 0;
            break;
    }

    // Apply volume scaling
    sample = (int16_t)((sample * g_synthState.volume) / 100);

    // Convert to PWM duty cycle (12-bit: 0-4095)
    // Center is 2048, sample adds ±offset
    int32_t duty_temp = 2048 + sample;
    uint16_t duty = (uint16_t)CLAMP(duty_temp, 1, 4095);

    // Update PWM
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, duty, DL_TIMER_CC_0_INDEX);

    // Advance phase
    phase += phase_increment;
}

//=============================================================================
// USER INPUT HANDLING
//=============================================================================

/**
 * @brief Process user input from joystick and buttons
 * @note Rate-limited to SENSOR_UPDATE_HZ
 */
static void Process_Input(void) {
    static uint32_t last_update = 0;
    uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);

    // Rate limiting with proper wrap-around handling
    uint32_t elapsed = TIMER_ELAPSED(now, last_update);
    if (elapsed < (SYSCLK_FREQUENCY / SENSOR_UPDATE_HZ)) {
        return;
    }
    last_update = now;

    // Read volatile values once to avoid race conditions
    uint16_t joy_x_local = g_synthState.joy_x;
    uint16_t joy_y_local = g_synthState.joy_y;

    // Joystick X: Frequency control
    if (joy_x_local > (JOY_ADC_CENTER + JOY_DEADZONE) ||
        joy_x_local < (JOY_ADC_CENTER - JOY_DEADZONE)) {

        float ratio = (float)joy_x_local / (float)JOY_ADC_MAX;
        g_synthState.frequency = FREQ_MIN + (ratio * (FREQ_MAX - FREQ_MIN));
        Update_Phase_Increment();
        g_synthState.display_update_needed = true;
    }

    // Joystick Y: Volume control
    if (joy_y_local > (JOY_ADC_CENTER + JOY_DEADZONE) ||
        joy_y_local < (JOY_ADC_CENTER - JOY_DEADZONE)) {

        // Safe calculation with explicit cast
        g_synthState.volume = (uint8_t)((joy_y_local * 100UL) / JOY_ADC_MAX);
        g_synthState.display_update_needed = true;
    }

    // Button S1: Cycle waveform
    static bool last_s1 = false;
    bool btn_s1_local = g_synthState.btn_s1;
    
    if (btn_s1_local && !last_s1) {
        // Cycle to next waveform
        g_synthState.waveform = (Waveform_t)((g_synthState.waveform + 1) % WAVE_COUNT);
        g_synthState.display_update_needed = true;
        g_synthState.btn_s1 = false;  // Clear flag

        // RGB LED feedback for each waveform
        DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
        switch (g_synthState.waveform) {
            case WAVE_SINE:
                DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
                break;
            case WAVE_SQUARE:
                DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
                break;
            case WAVE_SAWTOOTH:
                DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
                break;
            case WAVE_TRIANGLE:
                DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_GREEN_PIN);
                break;
            default:
                break;
        }
    }
    last_s1 = btn_s1_local;

    // Button S2 or Joystick Press: Toggle audio playback
    static bool last_s2 = false;
    static bool last_joy = false;
    
    bool btn_s2_local = g_synthState.btn_s2;
    bool joy_pressed_local = g_synthState.joy_pressed;

    if ((btn_s2_local && !last_s2) || (joy_pressed_local && !last_joy)) {
        g_synthState.audio_playing = !g_synthState.audio_playing;
        g_synthState.display_update_needed = true;
        
        // Clear button flags
        g_synthState.btn_s2 = false;
        g_synthState.joy_pressed = false;

        // Turn off RGB LED when stopped
        if (!g_synthState.audio_playing) {
            DL_GPIO_clearPins(GPIO_RGB_PORT, 
                             GPIO_RGB_RED_PIN | GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
        }
    }
    last_s2 = btn_s2_local;
    last_joy = joy_pressed_local;
}

//=============================================================================
// DISPLAY UPDATE
//=============================================================================

/**
 * @brief Update LCD display with current synthesizer state
 * @note Uses constants from main.h for positioning
 */
static void Update_Display(void) {
    char str[32];

    // Clear screen
    LCD_Clear(COLOR_BLACK);

    // === TITLE BAR ===
    const char *wave_names[] = {"SINE", "SQUARE", "SAW", "TRI"};
    snprintf(str, sizeof(str), "SYNTH - %s", wave_names[g_synthState.waveform]);
    LCD_DrawString(LCD_MARGIN_LEFT, LCD_Y_TITLE, str, COLOR_YELLOW);

    // Play/Stop indicator (top right)
    if (g_synthState.audio_playing) {
        LCD_FillCircle(115, 10, 4, COLOR_RED);
    } else {
        LCD_DrawRect(111, 6, 8, 8, COLOR_GRAY);
    }

    // === FREQUENCY DISPLAY ===
    snprintf(str, sizeof(str), "F: %.1f Hz", g_synthState.frequency);
    LCD_DrawString(LCD_MARGIN_LEFT, LCD_Y_FREQ, str, COLOR_CYAN);

    // === WAVEFORM VISUALIZATION ===
    LCD_DrawWaveform(10, LCD_Y_WAVEFORM, 108, 40, g_synthState.waveform);

    // === VOLUME DISPLAY ===
    snprintf(str, sizeof(str), "Vol: %u%%", g_synthState.volume);
    LCD_DrawString(LCD_MARGIN_LEFT, LCD_Y_VOLUME, str, COLOR_GREEN);

    // Volume bar (horizontal)
    LCD_DrawRect(4, LCD_Y_VOLUME_BAR, 120, 12, COLOR_GREEN);
    uint8_t vol_width = (uint8_t)((g_synthState.volume * 118UL) / 100UL);
    LCD_FillRect(5, LCD_Y_VOLUME_BAR + 1, vol_width, 10, COLOR_GREEN);

    // === STATUS ===
    if (g_synthState.audio_playing) {
        LCD_DrawString(35, LCD_Y_STATUS, "PLAYING", COLOR_RED);
    } else {
        LCD_DrawString(35, LCD_Y_STATUS, "STOPPED", COLOR_GRAY);
    }

    // === INSTRUCTIONS ===
    LCD_DrawString(10, LCD_Y_HELP1, "Joy: Freq/Vol", COLOR_DARKGRAY);
    LCD_DrawString(10, LCD_Y_HELP2, "S1: Wave S2: Play", COLOR_DARKGRAY);
}

//=============================================================================
// INTERRUPT HANDLERS
//=============================================================================

/**
 * @brief Timer interrupt handler for audio sample generation
 * @note Fires at SAMPLE_RATE (8 kHz)
 */
void TIMG7_IRQHandler(void) {
    switch (DL_Timer_getPendingInterrupt(TIMER_SAMPLE_INST)) {
        case DL_TIMER_IIDX_ZERO:
            Generate_Audio_Sample();
            break;
        default:
            break;
    }
}

/**
 * @brief ADC0 interrupt handler for joystick and microphone
 * Note: With AUTO sampling, this fires continuously after startConversion()
 */
void ADC0_IRQHandler(void) {
    switch (DL_ADC12_getPendingInterrupt(ADC_MIC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            g_synthState.mic_level = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
            break;
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            g_synthState.joy_y = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_1);
            break;
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            g_synthState.joy_x = DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_2);
            break;
        default:
            break;
    }
}

/**
 * @brief ADC1 interrupt handler for accelerometer
 * Note: With AUTO sampling, this fires continuously after startConversion()
 */
void ADC1_IRQHandler(void) {
    switch (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            g_synthState.accel_x = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
            break;
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            g_synthState.accel_y = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
            break;
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            g_synthState.accel_z = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
            break;
        default:
            break;
    }
}


/**
 * @brief GPIO interrupt handler for buttons
 */
void GPIOA_IRQHandler(void) {
    uint32_t status = DL_GPIO_getEnabledInterruptStatus(
        GPIOA,
        GPIO_BUTTONS_S1_PIN | GPIO_BUTTONS_S2_PIN | GPIO_BUTTONS_JOY_SEL_PIN);

    if (status & GPIO_BUTTONS_S1_PIN) {
        g_synthState.btn_s1 = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_S1_PIN);
    }

    if (status & GPIO_BUTTONS_S2_PIN) {
        g_synthState.btn_s2 = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_S2_PIN);
    }

    if (status & GPIO_BUTTONS_JOY_SEL_PIN) {
        g_synthState.joy_pressed = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_JOY_SEL_PIN);
    }
}

//=============================================================================
// MAIN PROGRAM
//=============================================================================

/**
 * @brief Main program entry point
 */
int main(void) {
    // Initialize system peripherals
    SYSCFG_DL_init();

    // Enable interrupts
    NVIC_EnableIRQ(TIMER_SAMPLE_INST_INT_IRQN);
    NVIC_EnableIRQ(ADC_MIC_JOY_INST_INT_IRQN);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);

    __enable_irq();

    // Initialize LCD display
    LCD_Init();
    LCD_Clear(COLOR_BLACK);

    // Splash screen
    LCD_DrawString(20, 50, "MSPM0G3507", COLOR_CYAN);
    LCD_DrawString(15, 70, "Synthesizer", COLOR_WHITE);
    LCD_DrawString(35, 90, "v1.1.0", COLOR_YELLOW);
    delay_ms(2000);

    // Initialize audio engine
    Update_Phase_Increment();

    // Start ADC conversions
    DL_ADC12_startConversion(ADC_MIC_JOY_INST);

    // Display initial state
    Update_Display();

    // Main loop
    while (1) {
        // Process user input
        Process_Input();

        // Update display at reduced rate (10 Hz)
        static uint32_t last_display = 0;
        uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);

        uint32_t display_elapsed = TIMER_ELAPSED(now, last_display);
        if (display_elapsed > (SYSCLK_FREQUENCY / DISPLAY_UPDATE_HZ)) {
            last_display = now;

            if (g_synthState.display_update_needed) {
                Update_Display();
                g_synthState.display_update_needed = false;
            }
        }
    }
}

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

/**
 * @brief Millisecond delay using timer
 * @param milliseconds Delay time in milliseconds
 * @note Safe up to ~53 seconds before overflow protection kicks in
 */
void delay_ms(uint32_t milliseconds) {
    // Use 64-bit arithmetic to prevent overflow
    uint64_t ticks = ((uint64_t)SYSCLK_FREQUENCY / 1000ULL) * milliseconds;
    
    // Cap at max uint32_t for safety
    if (ticks > TIMER_MAX_VALUE) {
        ticks = TIMER_MAX_VALUE;
    }
    
    uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
    uint32_t target_ticks = (uint32_t)ticks;
    
    while (TIMER_ELAPSED(DL_Timer_getTimerCount(TIMER_SAMPLE_INST), start) < target_ticks) {
        // Wait
    }
}

/**
 * @brief Microsecond delay using timer
 * @param microseconds Delay time in microseconds
 * @note Accurate for delays up to ~53,000 microseconds (53ms)
 */
void delay_us(uint32_t microseconds) {
    // Use 64-bit arithmetic to prevent overflow
    uint64_t ticks = ((uint64_t)SYSCLK_FREQUENCY / 1000000ULL) * microseconds;
    
    // Cap at max uint32_t for safety
    if (ticks > TIMER_MAX_VALUE) {
        ticks = TIMER_MAX_VALUE;
    }
    
    uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
    uint32_t target_ticks = (uint32_t)ticks;
    
    while (TIMER_ELAPSED(DL_Timer_getTimerCount(TIMER_SAMPLE_INST), start) < target_ticks) {
        // Wait
    }
}
