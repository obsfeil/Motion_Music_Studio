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

  uint8_t index = (phase >> 24); // ✅ DENNE LINJEN MANGLER!
  int16_t sample = 0;            // ✅ DENNE LINJEN MANGLER!

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
      sample = (index * 16) - 1024;
    } else {
      sample = 1024 - ((index - 128) * 16);
    }
    break;

  default:
    sample = 0;
    break;
  }

  sample = (sample * g_synthState.volume) / 100;

  uint16_t duty = 2048 + sample;
  if (duty > 4095)
    duty = 4095;
  if (duty < 1)
    duty = 1;

  DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, duty, DL_TIMER_CC_0_INDEX);

  phase += phase_increment;
}

//=============================================================================
// USER INPUT HANDLING
//=============================================================================

static void Process_Input(void) {
  static uint32_t last_update = 0;
  uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);

  if ((now - last_update) < (SYSCLK_FREQUENCY / SENSOR_UPDATE_HZ)) {
    return;
  }
  last_update = now;

  // Joystick X: Frequency
  if (g_synthState.joy_x > (JOY_ADC_CENTER + JOY_DEADZONE) ||
      g_synthState.joy_x < (JOY_ADC_CENTER - JOY_DEADZONE)) {

    float ratio = (float)g_synthState.joy_x / JOY_ADC_MAX;
    g_synthState.frequency = FREQ_MIN + (ratio * (FREQ_MAX - FREQ_MIN));
    Update_Phase_Increment();
    g_synthState.display_update_needed = true;
  }

  // Joystick Y: Volume
  if (g_synthState.joy_y > (JOY_ADC_CENTER + JOY_DEADZONE) ||
      g_synthState.joy_y < (JOY_ADC_CENTER - JOY_DEADZONE)) {

    g_synthState.volume = (g_synthState.joy_y * 100) / JOY_ADC_MAX;
    g_synthState.display_update_needed = true;
  }

  // Button S1: Cycle Waveform
  static bool last_s1 = false;
  if (g_synthState.btn_s1 && !last_s1) {
    g_synthState.waveform = (g_synthState.waveform + 1) % WAVE_COUNT;
    g_synthState.display_update_needed = true;

    // RGB LED feedback
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_GREEN_PIN |
                                         GPIO_RGB_BLUE_PIN);
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
  last_s1 = g_synthState.btn_s1;

  // Button S2 or Joy Press: Start/Stop
  static bool last_s2 = false;
  static bool last_joy = false;

  if ((g_synthState.btn_s2 && !last_s2) ||
      (g_synthState.joy_pressed && !last_joy)) {
    g_synthState.audio_playing = !g_synthState.audio_playing;
    g_synthState.display_update_needed = true;

    if (!g_synthState.audio_playing) {
      DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_GREEN_PIN |
                                           GPIO_RGB_BLUE_PIN);
    }
  }
  last_s2 = g_synthState.btn_s2;
  last_joy = g_synthState.joy_pressed;
}

//=============================================================================
// DISPLAY UPDATE
//=============================================================================

static void Update_Display(void) {
  // Clear screen
  LCD_Clear(COLOR_BLACK);

  char str[32];

  // === TITLE BAR ===
  const char *wave_names[] = {"SINE", "SQUARE", "SAW", "TRI"};
  snprintf(str, 32, "SYNTH - %s", wave_names[g_synthState.waveform]);
  LCD_DrawString(5, 5, str, COLOR_YELLOW);

  // Play/Stop indicator
  if (g_synthState.audio_playing) {
    LCD_FillCircle(115, 10, 4, COLOR_RED);
  } else {
    LCD_DrawRect(111, 6, 8, 8, COLOR_GRAY);
  }

  // === FREQUENCY ===
  snprintf(str, 32, "F: %.1f Hz", g_synthState.frequency);
  LCD_DrawString(5, 25, str, COLOR_CYAN);

  // === WAVEFORM VISUALIZATION ===
  LCD_DrawWaveform(10, 45, 108, 40, g_synthState.waveform);

  // === VOLUME ===
  snprintf(str, 32, "Vol: %d%%", g_synthState.volume);
  LCD_DrawString(5, 95, str, COLOR_GREEN);

  // Volume bar
  LCD_DrawRect(4, 105, 120, 12, COLOR_GREEN);
  uint8_t vol_width = (g_synthState.volume * 118) / 100;
  LCD_FillRect(5, 106, vol_width, 10, COLOR_GREEN);

  // === STATUS ===
  if (g_synthState.audio_playing) {
    LCD_DrawString(35, 130, "PLAYING", COLOR_RED);
  } else {
    LCD_DrawString(35, 130, "STOPPED", COLOR_GRAY);
  }

  // === INSTRUCTIONS ===
  LCD_DrawString(10, 145, "Joy: Freq/Vol", COLOR_DARKGRAY);
  LCD_DrawString(10, 155, "S1: Wave S2: Play", COLOR_DARKGRAY);
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

//=============================================================================
// MAIN PROGRAM
//=============================================================================

int main(void) {
  // Initialize system
  SYSCFG_DL_init();

  // Enable interrupts
  NVIC_EnableIRQ(TIMER_SAMPLE_INST_INT_IRQN);
  NVIC_EnableIRQ(ADC_MIC_JOY_INST_INT_IRQN);
  NVIC_EnableIRQ(GPIOA_INT_IRQn);

  __enable_irq();

  // Initialize LCD
  LCD_Init();
  LCD_Clear(COLOR_BLACK);

  // Splash screen
  LCD_DrawString(20, 50, "MSPM0G3507", COLOR_CYAN);
  LCD_DrawString(15, 70, "Synthesizer", COLOR_WHITE);
  LCD_DrawString(35, 90, "Phase 1+2", COLOR_YELLOW);
  delay_ms(2000);

  // Initialize audio
  Update_Phase_Increment();

  // Start ADC
  DL_ADC12_startConversion(ADC_MIC_JOY_INST);

  // Initial display
  Update_Display();

  // Main loop
  while (1) {
    // Process input
    Process_Input();

    // Update display (10 Hz)
    static uint32_t last_display = 0;
    uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);

    if ((now - last_display) > (SYSCLK_FREQUENCY / 10)) {
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
  uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
  uint32_t ticks = (SYSCLK_FREQUENCY / 1000) * milliseconds;

  while ((DL_Timer_getTimerCount(TIMER_SAMPLE_INST) - start) < ticks)
    ;
}

void delay_us(uint32_t microseconds) {
  uint32_t start = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
  uint32_t ticks = (SYSCLK_FREQUENCY / 1000000) * microseconds;

  while ((DL_Timer_getTimerCount(TIMER_SAMPLE_INST) - start) < ticks)
    ;
}
