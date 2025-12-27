/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer - SysTick Edition
 * @version 14.0.0
 *
 * ✅ SYSTICK FOR BUTTON DEBOUNCING (Best Practice!)
 * ✅ TIMG7 FOR AUDIO ONLY (8kHz sample rate)
 * ✅ Clean separation of concerns
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
// SYSTICK CONFIGURATION (10ms tick rate)
//=============================================================================
#define SYSTICK_RATE_HZ 100 // 100 Hz = 10ms
#define MCLK_FREQ_HZ 80000000UL
#define SYSTICK_LOAD_VALUE ((MCLK_FREQ_HZ / SYSTICK_RATE_HZ) - 1)

// Button debounce (in 10ms ticks)
#define BTN_DEBOUNCE_TICKS 5    // 50ms debounce
#define BTN_LONG_PRESS_TICKS 80 // 800ms for long press

//=============================================================================
// DMA KONFIGURASJON
//=============================================================================
#define ADC0_BUFFER_SIZE 2
static volatile uint16_t gADC0_DMA_Buffer[ADC0_BUFFER_SIZE]
    __attribute__((aligned(4)));
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
  ARP_OFF = 0,
  ARP_UP,
  ARP_DOWN,
  ARP_UP_DOWN,
  ARP_RANDOM,
  ARP_MODE_COUNT
} ArpMode_t;

typedef struct {
  ArpMode_t mode;
  uint8_t current_step;
  uint32_t step_counter;
  uint32_t steps_per_note;
  int8_t pattern[8];
} Arpeggiator_t;

typedef enum {
  CHORD_OFF = 0,
  CHORD_MAJOR,
  CHORD_MINOR,
  CHORD_MODE_COUNT
} ChordMode_t;

static const int8_t CHORD_INTERVALS[CHORD_MODE_COUNT][3] = {
    {0, 0, 0}, {0, 4, 7}, {0, 3, 7}};

typedef enum {
  INSTRUMENT_PIANO = 0,
  INSTRUMENT_ORGAN,
  INSTRUMENT_STRINGS,
  INSTRUMENT_BASS,
  INSTRUMENT_LEAD,
  INSTRUMENT_COUNT
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
    {"PIANO", {80, 1600, 700, 800}, WAVE_TRIANGLE, 1, 0, 0, LCD_COLOR_CYAN},
    {"ORGAN", {0, 0, 1000, 400}, WAVE_SINE, 1, 20, 0, LCD_COLOR_RED},
    {"STRINGS",
     {2400, 3200, 800, 4000},
     WAVE_SAWTOOTH,
     1,
     15,
     10,
     LCD_COLOR_YELLOW},
    {"BASS", {160, 800, 900, 800}, WAVE_SINE, 0, 0, 0, LCD_COLOR_BLUE},
    {"LEAD", {40, 1200, 850, 1600}, WAVE_SQUARE, 1, 30, 5, LCD_COLOR_GREEN}};

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
    {"SEQUENCE", INSTRUMENT_LEAD, true, CHORD_MINOR, ARP_UP}};

static const uint32_t PITCH_BEND_TABLE[25] = {
    32768, 34675, 36781,  38967,  41285,  43742,  46341, 49091, 51998,
    55041, 58255, 61644,  65536,  69433,  73533,  77841, 82366, 87111,
    92123, 97549, 103397, 109681, 116411, 123596, 131072};

//=============================================================================
// BUTTON STATE (Handled by SysTick)
//=============================================================================
typedef struct {
  uint8_t raw_state;         // Current GPIO reading
  uint8_t debounced_state;   // Stable state after debounce
  uint8_t last_stable_state; // Previous stable state
  uint16_t press_count;      // Number of presses detected
  uint16_t tick_counter;     // Timer for debouncing
} ButtonState_t;

static volatile ButtonState_t btn_s1 = {0};
static volatile ButtonState_t btn_s2 = {0};
static volatile ButtonState_t btn_joy_sel = {0};
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

// System tick counter (10ms ticks)
volatile uint32_t g_systick_count = 0;

// Debug counters
volatile uint32_t DEBUG_main_loop_count = 0;
volatile uint32_t DEBUG_timer_irq_count = 0;

#if ENABLE_WAVEFORM_DISPLAY
static int16_t waveform_buffer[64] = {0};
static uint8_t waveform_write_index = 0;
#endif

static const int16_t sine_table[256] = {
    0,    25,   49,   74,   98,   122,  147,  171,  195,  219,  243,  267,
    290,  314,  337,  360,  383,  405,  428,  450,  471,  493,  514,  535,
    555,  575,  595,  614,  633,  652,  670,  687,  704,  721,  737,  753,
    768,  783,  797,  811,  824,  837,  849,  860,  871,  882,  892,  901,
    910,  918,  926,  933,  939,  945,  951,  955,  960,  963,  966,  969,
    971,  972,  973,  974,  974,  973,  972,  971,  969,  966,  963,  960,
    955,  951,  945,  939,  933,  926,  918,  910,  901,  892,  882,  871,
    860,  849,  837,  824,  811,  797,  783,  768,  753,  737,  721,  704,
    687,  670,  652,  633,  614,  595,  575,  555,  535,  514,  493,  471,
    450,  428,  405,  383,  360,  337,  314,  290,  267,  243,  219,  195,
    171,  147,  122,  98,   74,   49,   25,   0,    -25,  -49,  -74,  -98,
    -122, -147, -171, -195, -219, -243, -267, -290, -314, -337, -360, -383,
    -405, -428, -450, -471, -493, -514, -535, -555, -575, -595, -614, -633,
    -652, -670, -687, -704, -721, -737, -753, -768, -783, -797, -811, -824,
    -837, -849, -860, -871, -882, -892, -901, -910, -918, -926, -933, -939,
    -945, -951, -955, -960, -963, -966, -969, -971, -972, -973, -974, -974,
    -973, -972, -971, -969, -966, -963, -960, -955, -951, -945, -939, -933,
    -926, -918, -910, -901, -892, -882, -871, -860, -849, -837, -824, -811,
    -797, -783, -768, -753, -737, -721, -704, -687, -670, -652, -633, -614,
    -595, -575, -555, -535, -514, -493, -471, -450, -428, -405, -383, -360,
    -337, -314, -290, -267, -243, -219, -195, -171, -147, -122, -98,  -74,
    -49,  -25};
//============================================================================
// SCALE SIZE
//============================================================================
static const int8_t SCALE_INTERVALS[] = {0, 2, 4, 7, 9};
#define SCALE_SIZE (sizeof(SCALE_INTERVALS) / sizeof(SCALE_INTERVALS[0]))
//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
static void SysTick_Init(void);
static void Button_Update(volatile ButtonState_t *btn, uint32_t gpio_pin);
static void Process_Joystick(void);
static void Process_Pitch_Bend(void);
static void Process_Envelope(void);
static void Process_Arpeggiator(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform);
static int16_t Generate_Chord_Sample(uint32_t *phases, uint32_t *increments);
static void Display_Update(void);
static void Display_Waveform(void);
static int8_t Quantize_Semitones(int8_t semitones);
static int16_t Low_Pass_Filter(int16_t new_sample); 

//=============================================================================
// MAIN
//=============================================================================
int main(void) {
  SYSCFG_DL_init();

  // ========== STATE INIT ==========
  memset((void *)&gSynthState, 0, sizeof(SynthState_t));
  gSynthState.frequency = 440.0f;
  gSynthState.volume = 80;
  gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
  gSynthState.audio_playing = 1;

  base_frequency_hz = 440;
  pitch_bend_semitones = 0;
  phase_increment = 236223201;
  chord_increments[0] = chord_increments[1] = chord_increments[2] = 236223201;

  arpeggiator.mode = ARP_OFF;
  arpeggiator.steps_per_note = (8000 * 60) / (120 * 4);

  envelope.state = ENV_ATTACK;
  envelope.phase = 0;
  envelope.amplitude = 0;
  envelope.note_on = true;

  // ========== ADC INIT ==========
  NVIC_EnableIRQ(ADC0_INT_IRQn);
  NVIC_EnableIRQ(ADC1_INT_IRQn);
  NVIC_EnableIRQ(DMA_INT_IRQn);

  DL_ADC12_enableConversions(ADC_JOY_INST);
  DL_ADC12_startConversion(ADC_JOY_INST);
  DL_ADC12_enableConversions(ADC_ACCEL_INST);
  DL_ADC12_startConversion(ADC_ACCEL_INST);

  // ========== LCD INIT ==========
  LCD_Init();
  DL_GPIO_setPins(LCD_BL_PORT, LCD_BL_GIPO_LCD_BACKLIGHT_PIN);
  LCD_FillScreen(LCD_COLOR_BLACK);
  LCD_PrintString(30, 50, "v14.0", LCD_COLOR_GREEN, LCD_COLOR_BLACK,
                  FONT_LARGE);
  LCD_PrintString(20, 70, "SysTick", LCD_COLOR_CYAN, LCD_COLOR_BLACK,
                  FONT_MEDIUM);

  // ========== LED INIT ==========
  DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
  DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);

  // ========== BUTTON INIT (Simple GPIO input, no interrupts!) ==========
  // Buttons are polled by SysTick, no GPIO interrupts needed

  // ========== SYSTICK INIT (10ms tick) ==========
  SysTick_Init();

  // ========== ENABLE INTERRUPTS ==========
  __enable_irq();

  // ========== AUDIO TIMER INIT (SIST!) ==========
  NVIC_EnableIRQ(TIMG7_INT_IRQn);
  DL_TimerG_startCounter(TIMER_SAMPLE_INST);

  // ========== DELAY FOR SPLASH SCREEN ==========
  DL_Common_delayCycles(80000000); // 1 second @ 80MHz
  LCD_FillScreen(LCD_COLOR_BLACK);

  // ========== MAIN LOOP ==========
  uint32_t loop_counter = 0, display_counter = 0;

  while (1) {
    DEBUG_main_loop_count++;

    // ✅ DMA HANDLING
    if (gADC0_DMA_Complete) {
      gSynthState.joy_x = gADC0_DMA_Buffer[0];
      gSynthState.joy_y = gADC0_DMA_Buffer[1];
      gADC0_DMA_Complete = false;
      DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
    }

    // ✅ BUTTON HANDLING (Check all 3 buttons)
    static uint16_t last_s1_count = 0;
    static uint16_t last_s2_count = 0;
    static uint16_t last_joy_count = 0;

    // S1 pressed? (Change Instrument)
    if (btn_s1.press_count != last_s1_count) {
      last_s1_count = btn_s1.press_count;
      Change_Instrument();
      DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
      display_counter = 200000;
    }

    // S2 - Toggle Audio
    if (btn_s2.press_count != last_s2_count) {
      last_s2_count = btn_s2.press_count;

      // Toggle audio
      gSynthState.audio_playing = !gSynthState.audio_playing;

      if (gSynthState.audio_playing) {
        DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        Trigger_Note_On();
      } else {
        DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        Trigger_Note_Off();
      }

      display_counter = 200000;
    }

    // ✅ JOY_SEL pressed? (Change Preset)
    if (btn_joy_sel.press_count != last_joy_count) {
      last_joy_count = btn_joy_sel.press_count;

      Change_Preset();
      DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
      display_counter = 200000;
    }

    // Processing
    if (loop_counter % 50000 == 0)
      Process_Joystick();
    if (loop_counter % 5000 == 0)
      Process_Pitch_Bend();

    // Display
    if (display_counter++ >= 200000) {
      Display_Update();
      display_counter = 0;
    }

    // Heartbeat
    if (loop_counter % 100000 == 0) {
      DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    }

    loop_counter++;
  }
}

//=============================================================================
// SYSTICK INITIALIZATION
//=============================================================================
static void SysTick_Init(void) {
  // Configure SysTick for 10ms tick (100 Hz)
  SysTick->LOAD = SYSTICK_LOAD_VALUE;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | // Use processor clock
                  SysTick_CTRL_TICKINT_Msk |   // Enable interrupt
                  SysTick_CTRL_ENABLE_Msk;     // Enable SysTick
}

//=============================================================================
// SYSTICK HANDLER (Called every 10ms)
//=============================================================================
void SysTick_Handler(void) {
  g_systick_count++;

  // Update button states (debouncing + edge detection)
  Button_Update(&btn_s1, GPIO_BUTTONS_S1_MKII_PIN);
  Button_Update(&btn_s2, GPIO_BUTTONS_S2_MKII_PIN);
  Button_Update(&btn_joy_sel, GPIO_BUTTONS_JOY_SEL_PIN);
}

//=============================================================================
// BUTTON UPDATE (Software Debouncing)
//=============================================================================
static void Button_Update(volatile ButtonState_t *btn, uint32_t gpio_pin) {
  // Read current GPIO state (0 = pressed because active-low)
  uint8_t current_state =
      (DL_GPIO_readPins(GPIO_BUTTONS_PORT, gpio_pin) == 0) ? 1 : 0;

  // If state changed, reset debounce timer
  if (current_state != btn->raw_state) {
    btn->raw_state = current_state;
    btn->tick_counter = 0;
  } else {
    // State stable, increment counter
    if (btn->tick_counter < BTN_DEBOUNCE_TICKS) {
      btn->tick_counter++;
    }

    // After debounce period, accept new state
    if (btn->tick_counter == BTN_DEBOUNCE_TICKS) {
      btn->debounced_state = current_state;

      // Detect PRESS edge (0 -> 1)
      if (btn->debounced_state && !btn->last_stable_state) {
        btn->press_count++; // Increment press counter
      }

      btn->last_stable_state = btn->debounced_state;
    }
  }
}

//=============================================================================
// AUDIO TIMER INTERRUPT (8kHz sample rate)
//=============================================================================
void TIMG7_IRQHandler(void) {
  DEBUG_timer_irq_count++;
  gSynthState.timer_count++;

  if (phase_increment == 0)
    phase_increment = 236223201;

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

//=============================================================================
// DMA INTERRUPT
//=============================================================================
void DMA_IRQHandler(void) {
  uint32_t status = DL_DMA_getPendingInterrupt(DMA);
  if (status == DL_DMA_EVENT_IIDX_DMACH1) {
    gADC0_DMA_Complete = true;
  }
}

//=============================================================================
// ADC INTERRUPTS
//=============================================================================
void ADC0_IRQHandler(void) {
  gSynthState.adc0_count++;

  switch (DL_ADC12_getPendingInterrupt(ADC_JOY_INST)) {
  case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
    gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.joy_y = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_1);
    break;
  case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
    gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.joy_y = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_1);
    break;
  default:
    break;
  }
}

void ADC1_IRQHandler(void) {
  gSynthState.adc1_count++;
  if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) ==
      DL_ADC12_IIDX_MEM3_RESULT_LOADED) {
    gSynthState.mic_level =
        DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.accel_x =
        (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
    gSynthState.accel_y =
        (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
    gSynthState.accel_z =
        (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_3);
  }
}

//=============================================================================
// LOW-PASS FILTER
//=============================================================================
static int16_t Low_Pass_Filter(int16_t new_sample) {
    static int16_t prev_sample = 0;
    
    // Simple 1-pole low-pass filter (50/50 mix of old and new)
    int16_t filtered = (prev_sample + new_sample) / 2;
    prev_sample = filtered;
    
    return filtered;
}

//=============================================================================
// AUDIO GENERATION
//=============================================================================
static void Generate_Audio_Sample(void) {
  if (phase_increment == 0)
    phase_increment = 236223201;
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
      int32_t phase_offset = ((int32_t)vibrato_lfo * inst->vibrato_depth *
                              (int32_t)phase_increment) /
                             100000;
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
  sample = Low_Pass_Filter(sample);
  if (sample > 1800) {
    int16_t excess = sample - 1800;
    sample = 1800 + (excess / 4);
    if (sample > 2000)
      sample = 2000;
  }
  if (sample < -1800) {
    int16_t excess = sample + 1800;
    sample = -1800 + (excess / 4);
    if (sample < -2000)
      sample = -2000;
  }

#if ENABLE_WAVEFORM_DISPLAY
  static uint8_t waveform_decimate_counter = 0;
  if (++waveform_decimate_counter >= 125) {
    waveform_decimate_counter = 0;
    waveform_buffer[waveform_write_index++] = sample;
    if (waveform_write_index >= 64)
      waveform_write_index = 0;
  }
#endif

  int32_t val = 2048 + (sample * 2);
  if (val < 0)
    val = 0;
  if (val > 4095)
    val = 4095;
  DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, (uint16_t)val,
                                   DL_TIMER_CC_0_INDEX);
  gSynthState.audio_samples_generated++;
}

static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform) {
  int16_t sample = 0;

  switch (waveform) {
  case WAVE_SINE:
    sample = sine_table[index];
    break;

  case WAVE_SQUARE:
    // ✅ SOFT SQUARE - Ikke brå overgang
    if (index < 120) {
      sample = 1000;
    } else if (index < 136) {
      // Smooth transition (16 samples)
      sample = 1000 - (int16_t)(((index - 120) * 2000) / 16);
    } else {
      sample = -1000;
    }
    break;

  case WAVE_SAWTOOTH:
    // ✅ SMOOTHER SAWTOOTH
    sample = (int16_t)(((int32_t)index * 2000 / 256) - 1000);
    // Add slight curve
    sample = (sample * 95) / 100;
    break;

  case WAVE_TRIANGLE:
    // ✅ Triangle er allerede myk, men kan forbedres
    if (index < 128) {
      sample = (int16_t)(((int32_t)index * 2000 / 128) - 1000);
    } else {
      sample = (int16_t)(1000 - ((int32_t)(index - 128) * 2000 / 128));
    }
    // Reduce amplitude slightly for smoother sound
    sample = (sample * 90) / 100;
    break;

  default:
    sample = sine_table[index];
    break;
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
// FREQUENCY SMOOTHING
//=============================================================================
static uint32_t Smooth_Frequency(uint32_t new_freq) {
    static uint32_t freq_history[4] = {440, 440, 440, 440};
    static uint8_t freq_index = 0;
    
    // Store new frequency in circular buffer
    freq_history[freq_index] = new_freq;
    freq_index = (freq_index + 1) % 4;
    
    // Return average of last 4 readings
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 4; i++) {
        sum += freq_history[i];
    }
    return sum / 4;
}

//=============================================================================
// PROCESSING
//=============================================================================
static int8_t Quantize_Semitones(int8_t semitones) { return semitones; }

static void Process_Arpeggiator(void) {
  if (arpeggiator.mode == ARP_OFF)
    return;
  arpeggiator.step_counter++;
  if (arpeggiator.step_counter >= arpeggiator.steps_per_note) {
    arpeggiator.step_counter = 0;
    const int8_t *intervals = CHORD_INTERVALS[chord_mode];
    switch (arpeggiator.mode) {
    case ARP_UP:
      arpeggiator.pattern[arpeggiator.current_step % 3] =
          intervals[arpeggiator.current_step % 3];
      break;
    case ARP_DOWN:
      arpeggiator.pattern[arpeggiator.current_step % 3] =
          intervals[2 - (arpeggiator.current_step % 3)];
      break;
    case ARP_UP_DOWN: {
      uint8_t idx = arpeggiator.current_step % 4;
      if (idx == 3)
        idx = 1;
      arpeggiator.pattern[arpeggiator.current_step] = intervals[idx];
    } break;
    default:
      break;
    }
    Trigger_Note_On();
    arpeggiator.current_step++;
    if (arpeggiator.current_step >= 8)
      arpeggiator.current_step = 0;
  }
}

static void Process_Envelope(void) {
  const ADSR_Profile_t *adsr = &INSTRUMENTS[current_instrument].adsr;
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
      envelope.amplitude =
          (uint16_t)((envelope.phase * 1000) / adsr->attack_samples);
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
      uint16_t decayed =
          (uint16_t)((envelope.phase * decay_range) / adsr->decay_samples);
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
      uint16_t released =
          (uint16_t)((envelope.phase * start_amp) / adsr->release_samples);
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

void Change_Instrument(void) {
  current_instrument = (current_instrument + 1) % INSTRUMENT_COUNT;
  gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
  Trigger_Note_On();
}

void Change_Preset(void) {
  current_preset = (current_preset + 1) % 3;
  const Preset_t *preset = &PRESETS[current_preset];
  current_instrument = preset->instrument;
  effects_enabled = preset->effects_enabled;
  chord_mode = preset->chord_mode;
  arpeggiator.mode = preset->arp_mode;
  gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
  Trigger_Note_On();
}

void Trigger_Note_On(void) {
  envelope.state = ENV_ATTACK;
  envelope.phase = 0;
  envelope.amplitude = 0;
  envelope.note_on = true;
}

void Trigger_Note_Off(void) {
  envelope.state = ENV_RELEASE;
  envelope.phase = 0;
  envelope.note_on = false;
}

static void Process_Joystick(void) {
    // Dead zone: Only respond to significant joystick movements
    #define JOY_DEAD_ZONE 50
    #define JOY_HYSTERESIS 20  // Prevents oscillation
    
    // Joystick X controls frequency
    if (gSynthState.joy_x > (2048 + JOY_DEAD_ZONE) || 
        gSynthState.joy_x < (2048 - JOY_DEAD_ZONE)) {
        
        uint32_t freq_int = FREQ_MIN_HZ + ((gSynthState.joy_x * (FREQ_MAX_HZ - FREQ_MIN_HZ)) / 4095);

        freq_int = Smooth_Frequency(freq_int);
        
        // Hysteresis: Only update if difference is significant
        uint32_t diff = (freq_int > base_frequency_hz) ? 
                        (freq_int - base_frequency_hz) : 
                        (base_frequency_hz - freq_int);
        
        if (diff > JOY_HYSTERESIS) {  // Changed from 10 to 20
            base_frequency_hz = freq_int;
            Update_Phase_Increment();
        }
    }
    
    // Joystick Y controls volume
    if (gSynthState.joy_y > (2048 + JOY_DEAD_ZONE) || 
        gSynthState.joy_y < (2048 - JOY_DEAD_ZONE)) {
        
        uint8_t new_vol = (uint8_t)((gSynthState.joy_y * 100) / 4095);
        if (new_vol > 100) new_vol = 100;
        
        // Only update if change is significant
        uint8_t vol_diff = (new_vol > gSynthState.volume) ?
                          (new_vol - gSynthState.volume) :
                          (gSynthState.volume - new_vol);
        
        if (vol_diff > 2) {  // Ignore tiny volume changes
            gSynthState.volume = new_vol;
        }
    }
}

static void Process_Pitch_Bend(void) {
    // ==========================================
    // Y-AKSE: OKTAV-HOPP (Med treghet)
    // ==========================================
    static int8_t target_octave = 0;
    static int8_t current_octave_val = 0; // Brukes for myk overgang? Nei, oktaver bør hoppe, men stabilt.
    
    int16_t accel_y = gSynthState.accel_y;

    // Økt hysterese for å unngå flimring mellom oktaver
    if (accel_y > 2800) target_octave = 12;
    else if (accel_y < 1300) target_octave = -12;
    else if (accel_y > 1700 && accel_y < 2400) target_octave = 0;

    // ==========================================
    // X-AKSE: SKALA (Med GLIDE-effekt)
    // ==========================================
    int16_t accel_x = gSynthState.accel_x;
    
    // Vi bruker et filter på selve rådataen FØR vi regner ut noten.
    // Dette fjerner "skjelving" på hånden.
    static int16_t filtered_x = 2048;
    
    // Filterformel: 90% gammel verdi, 10% ny verdi (Tungt filter)
    filtered_x = (filtered_x * 9 + accel_x) / 10;

    int16_t deviation_x = filtered_x - 2048;
    const int16_t SENSITIVITY_X = 120; // Økt litt for roligere spilling

    // Deadzone
    if (deviation_x > -50 && deviation_x < 50) deviation_x = 0;

    int8_t step = deviation_x / SENSITIVITY_X;
    int8_t x_semitones = 0;

    if (step != 0) {
        int8_t direction = (step > 0) ? 1 : -1;
        int8_t abs_step = (step > 0) ? step : -step;
        
        // Pentatonisk mapping
        int8_t note_idx = abs_step % SCALE_SIZE;
        int8_t oct_shift = abs_step / SCALE_SIZE;
        
        int8_t note_val = SCALE_INTERVALS[note_idx] + (12 * oct_shift);
        x_semitones = note_val * direction;
    }

    // ==========================================
    // TOTALT OG PORTAMENTO (GLIDE)
    // ==========================================
    int8_t target_total = x_semitones + target_octave;
    
    // Begrensning
    if (target_total > 24) target_total = 24;
    if (target_total < -24) target_total = -24;

    // Her skjer magien for harmoniske overganger:
    // I stedet for å sette pitch_bend_semitones direkte,
    // bruker vi en flyttalls-tilnærming eller et filter.
    // Men siden pitch_bend_semitones er int8, må vi gjøre det i Update_Phase_Increment 
    // eller oppdatere sjeldnere.
    
    // ENKEL LØSNING: Oppdater bare hvis endringen er stabil (Debounce)
    static uint8_t stable_count = 0;
    static int8_t last_target = 0;

    if (target_total != last_target) {
        stable_count = 0;
        last_target = target_total;
    } else {
        stable_count++;
    }

    // Vent til vi har hatt samme note i 5 loops (ca 50ms) før vi bytter.
    // Dette fjerner de små, stygge hoppene ("glitch").
    if (stable_count > 3) { 
        if (pitch_bend_semitones != target_total) {
            pitch_bend_semitones = target_total;
            Update_Phase_Increment();
        }
    }
}

static void Update_Phase_Increment(void) {
  if (base_frequency_hz == 0)
    base_frequency_hz = 440;
  int8_t table_index = pitch_bend_semitones + 12;
  if (table_index < 0)
    table_index = 0;
  if (table_index > 24)
    table_index = 24;
  uint32_t bend_ratio_fixed = PITCH_BEND_TABLE[table_index];
  uint64_t bent_freq_64 =
      ((uint64_t)base_frequency_hz * bend_ratio_fixed) >> 16;
  uint32_t bent_freq = (uint32_t)bent_freq_64;
  if (bent_freq < FREQ_MIN_HZ)
    bent_freq = FREQ_MIN_HZ;
  if (bent_freq > FREQ_MAX_HZ)
    bent_freq = FREQ_MAX_HZ;
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
  if (phase_increment == 0)
    phase_increment = 236223201;
  if (chord_mode != CHORD_OFF) {
    const int8_t *intervals = CHORD_INTERVALS[chord_mode];
    for (uint8_t v = 0; v < 3; v++) {
      int8_t chord_table_index = table_index + intervals[v];
      if (chord_table_index < 0)
        chord_table_index = 0;
      if (chord_table_index > 24)
        chord_table_index = 24;
      uint32_t chord_ratio = PITCH_BEND_TABLE[chord_table_index];
      uint64_t chord_freq_64 =
          ((uint64_t)base_frequency_hz * chord_ratio) >> 16;
      uint32_t chord_freq = (uint32_t)chord_freq_64;
      if (chord_freq < FREQ_MIN_HZ)
        chord_freq = FREQ_MIN_HZ;
      if (chord_freq > FREQ_MAX_HZ)
        chord_freq = FREQ_MAX_HZ;
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
    chord_increments[0] = chord_increments[1] = chord_increments[2] =
        phase_increment;
  }
  if (phase_increment == 0) {
    phase_increment = 236223201;
  }
  gSynthState.frequency = (float)bent_freq;
}

static void Display_Update(void) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];

  // Header with instrument name
  LCD_DrawRect(0, 0, 128, 16, inst->color);
  LCD_PrintString(3, 4, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
  LCD_PrintString(60, 4, PRESETS[current_preset].name, LCD_COLOR_BLACK,
                  inst->color, FONT_SMALL);

  // Frequency line
  LCD_DrawRect(0, 18, 128, 10, LCD_COLOR_BLACK); // ✅ CLEAR HELE LINJEN!
  LCD_PrintString(3, 18, "F:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(18, 18, base_frequency_hz, LCD_COLOR_WHITE, LCD_COLOR_BLACK,
                  FONT_SMALL);
  char buf[16];
  snprintf(buf, sizeof(buf), "%+d", pitch_bend_semitones);
  LCD_PrintString(55, 18, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);

  // Volume bar
  LCD_DrawRect(3, 30, 60, 4, LCD_COLOR_DARKGRAY);
  uint8_t bar_w = gSynthState.volume;
  if (bar_w > 100)
    bar_w = 100;
  LCD_DrawRect(3, 30, (bar_w * 60) / 100, 4, LCD_COLOR_GREEN);

  // Effects line
  LCD_DrawRect(66, 30, 62, 10, LCD_COLOR_BLACK); // ✅ CLEAR FX AREA!
  LCD_PrintString(66, 30, "FX:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintString(84, 30, effects_enabled ? "ON" : "OFF",
                  effects_enabled ? LCD_COLOR_GREEN : LCD_COLOR_RED,
                  LCD_COLOR_BLACK, FONT_SMALL);

  if (chord_mode != CHORD_OFF) {
    const char *chord_names[] = {"", "MAJ", "MIN"};
    LCD_PrintString(105, 30, chord_names[chord_mode], LCD_COLOR_MAGENTA,
                    LCD_COLOR_BLACK, FONT_SMALL);
  }

  // Arpeggiator and envelope
  LCD_DrawRect(0, 40, 128, 10, LCD_COLOR_BLACK); // ✅ CLEAR HELE LINJEN!
  if (arpeggiator.mode != ARP_OFF) {
    LCD_PrintString(3, 40, "ARP", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
    const char *arp_names[] = {"", "UP", "DN", "UD", "RND"};
    LCD_PrintString(24, 40, arp_names[arpeggiator.mode], LCD_COLOR_WHITE,
                    LCD_COLOR_BLACK, FONT_SMALL);
  }
  const char *env_names[] = {"IDLE", "ATK", "DEC", "SUS", "REL"};
  LCD_PrintString(55, 40, env_names[envelope.state], LCD_COLOR_CYAN,
                  LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(90, 40, envelope.amplitude / 10, LCD_COLOR_WHITE,
                  LCD_COLOR_BLACK, FONT_SMALL);

#if ENABLE_WAVEFORM_DISPLAY
  Display_Waveform();
#endif

  // Playing status
  LCD_DrawRect(0, 118, 60, 10, LCD_COLOR_BLACK); // ✅ CLEAR STATUS AREA!
  if (gSynthState.audio_playing) {
    LCD_PrintString(3, 118, "PLAYING", LCD_COLOR_GREEN, LCD_COLOR_BLACK,
                    FONT_SMALL);
  } else {
    LCD_PrintString(3, 118, "STOPPED", LCD_COLOR_RED, LCD_COLOR_BLACK,
                    FONT_SMALL);
  }
}

#if ENABLE_WAVEFORM_DISPLAY
static void Display_Waveform(void) {
  uint16_t y_center = 80, y_scale = 25;
  LCD_DrawRect(0, 50, 128, 60, LCD_COLOR_BLACK);
  for (uint8_t x = 0; x < 128; x += 4)
    LCD_DrawPixel(x, y_center, LCD_COLOR_DARKGRAY);
  for (uint8_t i = 0; i < 63; i++) {
    int16_t y1 = y_center - ((waveform_buffer[i] * y_scale) / 1000);
    int16_t y2 = y_center - ((waveform_buffer[i + 1] * y_scale) / 1000);
    if (y1 < 50)
      y1 = 50;
    if (y1 > 110)
      y1 = 110;
    if (y2 < 50)
      y2 = 50;
    if (y2 > 110)
      y2 = 110;
    LCD_DrawLine(i * 2, y1, (i + 1) * 2, y2, LCD_COLOR_CYAN);
  }
}
#endif

void HardFault_Handler(void) {
  while (1) {
    DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    for (volatile uint32_t i = 0; i < 100000; i++)
      ;
  }
}