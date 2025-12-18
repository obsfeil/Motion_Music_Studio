/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer with LCD Display
 * @version 1.3.1
 * @date 2025-12-18
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lcd/lcd_driver.h"
#include "main.h"
#include "ti_msp_dl_config.h"

//=============================================================================
// STACK MONITORING
//=============================================================================

#define STACK_CANARY_VALUE 0xDEADBEEF
volatile uint32_t stack_canary __attribute__((section(".stack"))) = STACK_CANARY_VALUE;

// Forward declarations
void check_stack_canary(void);
void print_memory_usage(void);

//=============================================================================
// GLOBAL STATE
//=============================================================================

SynthState_t g_synthState = {.waveform = WAVE_SINE,
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
                             .light_lux = 0.0f};

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
    -385, -341, -296, -250, -203, -155, -107, -58};

static uint32_t phase = 0;
static uint32_t phase_increment = 0;

static void Update_Phase_Increment(void) {
  phase_increment =
      (uint32_t)((g_synthState.frequency * 4294967296.0) / SAMPLE_RATE);
}

static void Generate_Audio_Sample(void) {
  if (!g_synthState.audio_playing) {
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
    return;
  }

  uint8_t index = (uint8_t)((phase >> 24) & 0xFF);
  int16_t sample = 0;

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

  sample = (int16_t)((sample * g_synthState.volume) / 100);
  int32_t duty_temp = 2048 + sample;
  uint16_t duty = (uint16_t)CLAMP(duty_temp, 1, 4095);
  DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, duty, DL_TIMER_CC_0_INDEX);
  phase += phase_increment;
}

//=============================================================================
// INPUT PROCESSING
//=============================================================================

static void Process_Input(void) {
  static uint32_t last_update = 0;
  uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);

  uint32_t elapsed = TIMER_ELAPSED(now, last_update);
  if (elapsed < (SYSCLK_FREQUENCY / SENSOR_UPDATE_HZ)) {
    return;
  }
  last_update = now;

  uint16_t joy_x_local = g_synthState.joy_x;
  uint16_t joy_y_local = g_synthState.joy_y;

  if (joy_x_local > (JOY_ADC_CENTER + JOY_DEADZONE) ||
      joy_x_local < (JOY_ADC_CENTER - JOY_DEADZONE)) {
    float ratio = (float)joy_x_local / (float)JOY_ADC_MAX;
    g_synthState.frequency = FREQ_MIN + (ratio * (FREQ_MAX - FREQ_MIN));
    Update_Phase_Increment();
    g_synthState.display_update_needed = true;
  }

  if (joy_y_local > (JOY_ADC_CENTER + JOY_DEADZONE) ||
      joy_y_local < (JOY_ADC_CENTER - JOY_DEADZONE)) {
    g_synthState.volume = (uint8_t)((joy_y_local * 100UL) / JOY_ADC_MAX);
    g_synthState.display_update_needed = true;
  }

  static bool last_s1 = false;
  bool btn_s1_local = g_synthState.btn_s1;

  if (btn_s1_local && !last_s1) {
    g_synthState.waveform =
        (Waveform_t)((g_synthState.waveform + 1) % WAVE_COUNT);
    g_synthState.display_update_needed = true;
    g_synthState.btn_s1 = false;

    // Clear all RGB LEDs
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);

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
      DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
      DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
      break;
    case WAVE_COUNT:
    default:
      break;
    }
  }
  last_s1 = btn_s1_local;

  static bool last_s2 = false;
  static bool last_joy = false;

  bool btn_s2_local = g_synthState.btn_s2;
  bool joy_pressed_local = g_synthState.joy_pressed;

  if ((btn_s2_local && !last_s2) || (joy_pressed_local && !last_joy)) {
    g_synthState.audio_playing = !g_synthState.audio_playing;
    g_synthState.display_update_needed = true;
    g_synthState.btn_s2 = false;
    g_synthState.joy_pressed = false;

    if (!g_synthState.audio_playing) {
      DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
      DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
      DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    }
  }
  last_s2 = btn_s2_local;
  last_joy = joy_pressed_local;
}

//=============================================================================
// DISPLAY UPDATE
//=============================================================================

static void Update_Display(void) {
  char str[32];

  LCD_Clear(COLOR_BLACK);

  const char *wave_names[] = {"SINE", "SQUARE", "SAW", "TRI"};
  snprintf(str, sizeof(str), "SYNTH - %s", wave_names[g_synthState.waveform]);
  LCD_DrawString(LCD_MARGIN_LEFT, LCD_Y_TITLE, str, COLOR_YELLOW);

  if (g_synthState.audio_playing) {
    LCD_FillCircle(115, 10, 4, COLOR_RED);
  } else {
    LCD_DrawRect(111, 6, 8, 8, COLOR_GRAY);
  }

  snprintf(str, sizeof(str), "F: %.1f Hz", g_synthState.frequency);
  LCD_DrawString(LCD_MARGIN_LEFT, LCD_Y_FREQ, str, COLOR_CYAN);

  LCD_DrawWaveform(10, LCD_Y_WAVEFORM, 108, 40, g_synthState.waveform);

  snprintf(str, sizeof(str), "Vol: %u%%", g_synthState.volume);
  LCD_DrawString(LCD_MARGIN_LEFT, LCD_Y_VOLUME, str, COLOR_GREEN);

  LCD_DrawRect(4, LCD_Y_VOLUME_BAR, 120, 12, COLOR_GREEN);
  uint8_t vol_width = (uint8_t)((g_synthState.volume * 118UL) / 100UL);
  LCD_FillRect(5, LCD_Y_VOLUME_BAR + 1, vol_width, 10, COLOR_GREEN);

  if (g_synthState.audio_playing) {
    LCD_DrawString(35, LCD_Y_STATUS, "PLAYING", COLOR_RED);
  } else {
    LCD_DrawString(35, LCD_Y_STATUS, "STOPPED", COLOR_GRAY);
  }

  LCD_DrawString(10, LCD_Y_HELP1, "Joy: Freq/Vol", COLOR_DARKGRAY);
  LCD_DrawString(10, LCD_Y_HELP2, "S1: Wave S2: Play", COLOR_DARKGRAY);
}

//=============================================================================
// INTERRUPT HANDLERS
//=============================================================================

void TIMG7_IRQHandler(void) {
  switch (DL_Timer_getPendingInterrupt(TIMER_SAMPLE_INST)) {
  case DL_TIMER_IIDX_ZERO:
    Generate_Audio_Sample();
    break;
  default:
    break;
  }
}

void ADC0_IRQHandler(void) {
  switch (DL_ADC12_getPendingInterrupt(ADC_MIC_JOY_INST)) {
  case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
    g_synthState.mic_level =
        DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_0);
    break;
  case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
    g_synthState.joy_y =
        DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_1);
    break;
  case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
    g_synthState.joy_x =
        DL_ADC12_getMemResult(ADC_MIC_JOY_INST, DL_ADC12_MEM_IDX_2);
    break;
  default:
    break;
  }
}

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

// Forbedret HardFault handler
void HardFault_Handler(void) {
    // Disable alle interrupts
    __disable_irq();
    
    // Prøv å vise feilinfo på LCD (hvis mulig)
    LCD_Clear(COLOR_BLACK);
    LCD_DrawString(10, 40, "HARD FAULT!", COLOR_RED);
    LCD_DrawString(10, 60, "Stack overflow?", COLOR_YELLOW);
    LCD_DrawString(10, 80, "Check stack size", COLOR_WHITE);
    
    // Blink rød LED for å signalisere feil
    while(1) {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        
        // Software delay (siden timers kan være korrupte)
        for(volatile uint32_t i = 0; i < 100000; i++);
    }
}

//=============================================================================
// STACK MONITORING FUNCTIONS
//=============================================================================

void check_stack_canary(void) {
    if (stack_canary != STACK_CANARY_VALUE) {
        // Stack overflow detektert!
        LCD_Clear(COLOR_BLACK);
        LCD_DrawString(10, 50, "STACK OVERFLOW!", COLOR_RED);
        
        while(1) {
            DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
            delay_ms(100);
        }
    }
}

void print_memory_usage(void) {
    extern uint32_t __STACK_END;
    extern uint32_t __bss_start__;
    extern uint32_t __bss_end__;
    
    uint32_t stack_size = 2048;  // Fra linker script
    uint32_t bss_size = (uint32_t)&__bss_end__ - (uint32_t)&__bss_start__;
    
    char str[32];
    snprintf(str, sizeof(str), "Stk:%u BSS:%u", stack_size, bss_size);
    LCD_DrawString(0, 0, str, COLOR_DARKGRAY);
}

//=============================================================================
// MAIN
//=============================================================================

int main(void) {
  SYSCFG_DL_init();

  // CRITICAL: Initialize g_synthState explicitly in main()
  // (static initialization may not work properly with this toolchain)
  g_synthState.waveform = WAVE_SINE;
  g_synthState.mode = MODE_SYNTH;
  g_synthState.frequency = FREQ_DEFAULT;
  g_synthState.volume = VOLUME_DEFAULT;
  g_synthState.pitchBend = 0;
  g_synthState.audio_playing = false;
  g_synthState.display_update_needed = false;
  g_synthState.joy_x = JOY_ADC_CENTER;
  g_synthState.joy_y = JOY_ADC_CENTER;
  g_synthState.joy_pressed = false;
  g_synthState.btn_s1 = false;
  g_synthState.btn_s2 = false;
  g_synthState.accel_x = ACCEL_ZERO_G;
  g_synthState.accel_y = ACCEL_ZERO_G;
  g_synthState.accel_z = ACCEL_ZERO_G;
  g_synthState.mic_level = 0;
  g_synthState.light_lux = 0.0f;

  DL_ADC12_enableConversions(ADC_MIC_JOY_INST);
  DL_ADC12_startConversion(ADC_MIC_JOY_INST);

  NVIC_EnableIRQ(TIMER_SAMPLE_INST_INT_IRQN);
  NVIC_EnableIRQ(ADC_MIC_JOY_INST_INT_IRQN);
  NVIC_EnableIRQ(ADC_ACCEL_INST_INT_IRQN); // ← Denne!
  NVIC_EnableIRQ(GPIOA_INT_IRQn);


//  LCD_Init();
//  LCD_Clear(COLOR_BLACK);

  // DEBUG: Vis initialiseringsverdier
//  char debug_str[32];
//  snprintf(debug_str, sizeof(debug_str), "F:%.1f V:%u", 
//           g_synthState.frequency, g_synthState.volume);
//  LCD_DrawString(5, 20, debug_str, COLOR_WHITE);
//  snprintf(debug_str, sizeof(debug_str), "JX:%u JY:%u", 
//           g_synthState.joy_x, g_synthState.joy_y);
//  LCD_DrawString(5, 35, debug_str, COLOR_WHITE);
//  delay_ms(3000);  // Gi tid til å lese debug-info

//  LCD_DrawString(20, 50, "MSPM0G3507", COLOR_CYAN);
//  LCD_DrawString(15, 70, "Synthesizer", COLOR_WHITE);
//  LCD_DrawString(35, 90, "v1.3.2", COLOR_YELLOW);
//  delay_ms(2000);

  
  __enable_irq();
  
  Update_Phase_Increment();
  DL_ADC12_startConversion(ADC_MIC_JOY_INST);
  Update_Display();

  while (1) {
    check_stack_canary();  // Sjekk stack health
    Process_Input();

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

void delay_ms(uint32_t milliseconds) {
  uint64_t ticks = ((uint64_t)SYSCLK_FREQUENCY / 1000ULL) * milliseconds;
  if (ticks > TIMER_MAX_VALUE) {
    ticks = TIMER_MAX_VALUE;
  }
  uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
  uint32_t target_ticks = (uint32_t)ticks;
  while (TIMER_ELAPSED(DL_Timer_getTimerCount(TIMER_SAMPLE_INST), start) <
         target_ticks) {
  }
}

void delay_us(uint32_t microseconds) {
  uint64_t ticks = ((uint64_t)SYSCLK_FREQUENCY / 1000000ULL) * microseconds;
  if (ticks > TIMER_MAX_VALUE) {
    ticks = TIMER_MAX_VALUE;
  }
  uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
  uint32_t target_ticks = (uint32_t)ticks;
  while (TIMER_ELAPSED(DL_Timer_getTimerCount(TIMER_SAMPLE_INST), start) <
         target_ticks) {
  }
}
