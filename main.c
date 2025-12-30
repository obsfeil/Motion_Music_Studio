/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer - v31.0 COMBINED BEST OF BOTH
 * @version 31.0
 *
 * ✅ CORE: Based on v28.2.1 structure (Stable libraries, Clean ISRs)
 * ✅ LOGIC: Ported from v30.2 (Harmonic engine, Greensleeves, Accurate Pitch)
 *
 * FEATURES:
 * - Libraries: Uses lib/audio, lib/edumkii for hardware abstraction.
 * - Audio: Accurate Harmonic Frequency calculation (from v30.2).
 * - Mode: "Epic Mode" (Greensleeves) triggered by JOY_SEL (from v30.2).
 * - Controls: Accel Tilt does musical intervals (+7/+12) (from v30.2).
 *
 * @date 2025-12-30
 */

#include "main.h"
#include "lcd_driver.h"
#include "lib/audio/audio_engine.h"
#include "lib/audio/audio_envelope.h"
#include "lib/audio/audio_filters.h"
#include "lib/edumkii/edumkii.h"
#include "ti_msp_dl_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//=============================================================================
// CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ 16000
#define SYSTICK_RATE_HZ 100
#define MCLK_FREQ_HZ 80000000UL
#define SYSTICK_LOAD_VALUE ((MCLK_FREQ_HZ / SYSTICK_RATE_HZ) - 1)
#define PORTAMENTO_SPEED 45
#define AUDIO_GAIN_BOOST 8
#define FREQ_MIN_HZ 20
#define FREQ_MAX_HZ 8000

#define ACCEL_Y_NEUTRAL 2849
#define ACCEL_Y_THRESHOLD 300

#define PWM_MAX_VALUE 2047
#define PWM_CENTER_VALUE 1023

#define ENABLE_CHORD_MODE 1
#define ENABLE_ARPEGGIATOR 1
#define ENABLE_WAVEFORM_DISPLAY 1
#define ENABLE_DEBUG_LEDS 2

//=============================================================================
// 1. TYPE DEFINITIONS & HARMONY (From v30.2)
//=============================================================================

// Scale & Key Types
typedef enum { SCALE_MAJOR = 0, SCALE_MINOR, SCALE_COUNT } ScaleType_t;
typedef enum {
  KEY_C = 0, KEY_D, KEY_E, KEY_F, KEY_G, KEY_A, KEY_B, KEY_COUNT
} MusicalKey_t;
typedef enum { MODE_MAJOR = 0, MODE_MINOR = 1, MODE_COUNT = 2 } MusicalMode_t;

// Harmonic Functions (From v30.2 - "Accurate Tones" Logic)
typedef enum {
  HARM_I = 0, HARM_ii, HARM_iii, HARM_IV, HARM_V, HARM_vi, HARM_vii,
  HARM_bVII, HARM_V7, HARM_COUNT
} HarmonicFunction_t;

// Instrument Types
typedef enum {
  INSTRUMENT_PIANO = 0, INSTRUMENT_ORGAN, INSTRUMENT_STRINGS,
  INSTRUMENT_BASS, INSTRUMENT_LEAD, INSTRUMENT_COUNT
} Instrument_t;

// Chord Modes
typedef enum {
  CHORD_OFF = 0, CHORD_MAJOR, CHORD_MINOR, CHORD_MODE_COUNT
} ChordMode_t;

// Arpeggiator Modes
typedef enum { ARP_OFF = 0, ARP_UP, ARP_DOWN, ARP_MODE_COUNT } ArpMode_t;

//=============================================================================
// 2. STRUCTS
//=============================================================================

typedef struct {
  MusicalKey_t current_key;
  ScaleType_t current_scale;
  uint8_t scale_position;
  uint16_t current_note_freq;
} ScaleState_t;

typedef struct {
  const char *name;
  ADSR_Profile_t adsr;
  Waveform_t waveform;
  uint8_t num_harmonics;
  uint8_t vibrato_depth;
  uint8_t tremolo_depth;
  uint16_t color;
} InstrumentProfile_t;

typedef struct {
  const char *name;
  Instrument_t instrument;
  bool effects_enabled;
  ChordMode_t chord_mode;
  ArpMode_t arp_mode;
} Preset_t;

typedef struct {
  ArpMode_t mode;
  uint8_t current_step;
  uint32_t step_counter;
  uint32_t steps_per_note;
} Arpeggiator_t;

//=============================================================================
// 3. CONSTANTS & TABLES (From v30.2)
//=============================================================================

static const int8_t HARMONIC_INTERVALS_MAJOR[HARM_COUNT][4] = {
    {0, 4, 7, -1},    {2, 5, 9, -1},    {4, 7, 11, -1},
    {5, 9, 12, -1},   {7, 11, 14, -1},  {9, 12, 16, -1},
    {11, 14, 17, -1}, {10, 14, 17, -1}, {7, 11, 14, 17}};

static const int8_t HARMONIC_INTERVALS_MINOR[HARM_COUNT][4] = {
    {0, 3, 7, -1},    {2, 5, 8, -1},    {3, 7, 10, -1},
    {5, 8, 12, -1},   {7, 11, 14, -1},  {8, 12, 15, -1},
    {11, 14, 17, -1}, {10, 14, 17, -1}, {7, 11, 14, 17}};

static const uint16_t ROOT_FREQUENCIES[KEY_COUNT] = {
    262, 294, 330, 349, 392, 440, 494};
static const char *KEY_NAMES[KEY_COUNT] = {"C", "D", "E", "F", "G", "A", "B"};

static const char *HARMONIC_NAMES_MAJOR[HARM_COUNT] = {
    "I", "ii", "iii", "IV", "V", "vi", "dim", "bVII", "V7"};
static const char *HARMONIC_NAMES_MINOR[HARM_COUNT] = {
    "i", "dim", "III", "iv", "V", "VI", "dim", "bVII", "V7"};

static const int8_t CHORD_INTERVALS[CHORD_MODE_COUNT][3] = {
    {0, 0, 0}, {0, 4, 7}, {0, 3, 7}};

// PITCH BEND TABLE (From v27/v30)
static const uint32_t PITCH_BEND_TABLE[25] = {
    32768, 34675, 36781,  38967,  41285,  43742,  46341, 49091, 51998,
    55041, 58255, 61644,  65536,  69433,  73533,  77841, 82366, 87111,
    92123, 97549, 103397, 109681, 116411, 123596, 131072};

// INSTRUMENTS
static const InstrumentProfile_t INSTRUMENTS[INSTRUMENT_COUNT] = {
    {"PIANO", {40, 1200, 650, 600}, WAVE_TRIANGLE, 2, 0, 0, LCD_COLOR_CYAN},
    {"ORGAN", {0, 0, 1000, 200}, WAVE_SINE, 3, 25, 0, LCD_COLOR_RED},
    {"STRINGS", {3200, 4000, 900, 5000}, WAVE_SAWTOOTH, 1, 20, 15, LCD_COLOR_YELLOW},
    {"BASS", {80, 400, 950, 600}, WAVE_SINE, 0, 0, 0, LCD_COLOR_BLUE},
    {"LEAD", {20, 800, 900, 1200}, WAVE_SQUARE, 2, 40, 8, LCD_COLOR_GREEN}};

// PRESETS
static const Preset_t PRESETS[3] = {
    {"CLASSIC", INSTRUMENT_PIANO, false, CHORD_OFF, ARP_OFF},
    {"AMBIENT", INSTRUMENT_STRINGS, true, CHORD_MAJOR, ARP_OFF},
    {"SEQUENCE", INSTRUMENT_LEAD, true, CHORD_MINOR, ARP_UP}};

//=============================================================================
// 4. GLOBAL VARIABLES
//=============================================================================
static Button_t btn_s1, btn_s2, btn_joy_sel;
static Joystick_t joystick;
static Accelerometer_t accel;
static Envelope_t envelope;
volatile SynthState_t gSynthState;

// Musical State
static ScaleState_t scale_state = {KEY_C, SCALE_MAJOR, 3, 262};
static MusicalMode_t current_mode = MODE_MAJOR;
static HarmonicFunction_t current_harmony = HARM_I;
static Instrument_t current_instrument = INSTRUMENT_PIANO;
static uint8_t current_preset = 0;
static bool effects_enabled = true;
static ChordMode_t chord_mode = CHORD_OFF;
static Arpeggiator_t arpeggiator = {0};

// Epic Mode (Greensleeves)
static bool epic_mode_active = false;
static uint8_t epic_sequence_step = 0;
static uint32_t epic_step_counter = 0;
static const uint32_t EPIC_STEPS_PER_NOTE = 24000;

// Frequencies
static uint32_t base_frequency_hz = 440;
static uint32_t target_frequency_hz = 440;
static uint32_t current_frequency_hz = 440;
static int8_t current_octave_shift = 0;

// Phase Accumulators
volatile uint32_t g_phase = 0;
volatile uint32_t g_phase_increment = 118111601;
volatile uint32_t g_chord_phases[3] = {0};
volatile uint32_t g_chord_increments[3] = {118111601, 118111601, 118111601};
static uint16_t vibrato_phase = 0, tremolo_phase = 0;

#if ENABLE_WAVEFORM_DISPLAY
static int16_t waveform_buffer[64] = {0};
static uint8_t waveform_write_index = 0;
#endif

// DMA Buffers
#define ADC0_BUFFER_SIZE 2
static volatile uint16_t gADC0_DMA_Buffer[ADC0_BUFFER_SIZE]
    __attribute__((aligned(4)));
static volatile bool gADC0_DMA_Complete = false;
static int16_t uart_buffer[16];
static uint8_t uart_buf_idx = 0;
static uint8_t uart_decimate_counter = 0;
#define UART_DECIMATION_FACTOR 4

//=============================================================================
// 5. PROTOTYPES
//=============================================================================
static void SysTick_Init(void);
static void Process_Musical_Controls(void);
static void Process_Accelerometer(void);
static void Process_Epic_Mode(void); // From v30.2
static void Toggle_Epic_Mode(void);  // From v30.2
static void Process_Portamento(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static int16_t Generate_Chord_Sample(volatile uint32_t *phases,
                                     volatile uint32_t *increments);
static void Display_Update(void);
static void Display_Waveform(void);
static void Display_Scale_Info(void);
// The accurate frequency calculation from v30.2
static uint16_t Calculate_Harmonic_Frequency(MusicalKey_t key,
                                             MusicalMode_t mode,
                                             HarmonicFunction_t harmony,
                                             int8_t octave_shift);
void Change_Instrument(void);
void Change_Preset(void);
void Trigger_Note_On(void);
void Trigger_Note_Off(void);
#if ENABLE_DEBUG_LEDS
static void Debug_LED_Update(int8_t octave);
#endif

//=============================================================================
// 6. MAIN FUNCTION
//=============================================================================
int main(void) {
  SYSCFG_DL_init();
  memset((void *)&gSynthState, 0, sizeof(SynthState_t));
  gSynthState.frequency = 440;
  gSynthState.volume = 80;
  gSynthState.audio_playing = 1;

  Audio_Init(SAMPLE_RATE_HZ);
  Audio_SetWaveform(INSTRUMENTS[current_instrument].waveform);
  Audio_SetFrequency(440);

  // Init Library Objects
  Button_Init(&btn_s1);
  Button_Init(&btn_s2);
  Button_Init(&btn_joy_sel);
  Joystick_Init(&joystick, 100);
  Accel_Init(&accel, 100);

  Filter_Reset();
  Envelope_Init(&envelope, &INSTRUMENTS[current_instrument].adsr);
  Envelope_NoteOn(&envelope);

  Update_Phase_Increment();
  arpeggiator.mode = ARP_OFF;
  arpeggiator.steps_per_note = (SAMPLE_RATE_HZ * 60) / (120 * 4);

  NVIC_EnableIRQ(ADC0_INT_IRQn);
  NVIC_EnableIRQ(ADC1_INT_IRQn);
  NVIC_EnableIRQ(DMA_INT_IRQn);
  DL_ADC12_enableConversions(ADC_JOY_INST);
  DL_ADC12_startConversion(ADC_JOY_INST);
  DL_ADC12_enableConversions(ADC_ACCEL_INST);
  DL_ADC12_startConversion(ADC_ACCEL_INST);

  LCD_Init();
  DL_GPIO_setPins(LCD_BL_PORT, LCD_BL_GIPO_LCD_BACKLIGHT_PIN);
  LCD_FillScreen(LCD_COLOR_BLACK);
  LCD_PrintString(10, 50, "v31.0 MERGED", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_LARGE);
  LCD_PrintString(5, 70, "BEST OF BOTH", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_MEDIUM);
  DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
  DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);

  SysTick_Init();
  __enable_irq();
  NVIC_ClearPendingIRQ(TIMG7_INT_IRQn);
  NVIC_SetPriority(TIMG7_INT_IRQn, 1);
  NVIC_EnableIRQ(TIMG7_INT_IRQn);
  DL_TimerG_startCounter(TIMER_SAMPLE_INST);
  DL_Common_delayCycles(80000000);
  LCD_FillScreen(LCD_COLOR_BLACK);

  uint32_t loop_counter = 0, display_counter = 0;

  while (1) {
    if (gADC0_DMA_Complete) {
      gSynthState.joy_x = gADC0_DMA_Buffer[0];
      gADC0_DMA_Complete = false;
    }

    // --- Button Handling (Combined Logic) ---
    ButtonEvent_t s1_event = Button_GetEvent(&btn_s1);
    if (s1_event == BTN_EVENT_SHORT_CLICK) {
      Change_Instrument();
      display_counter = 200000;
    } else if (s1_event == BTN_EVENT_LONG_PRESS) {
      // Switch Major/Minor Mode (From v30.2)
      current_mode = (MusicalMode_t)((current_mode + 1) % MODE_COUNT);
      scale_state.current_note_freq = Calculate_Harmonic_Frequency(
          scale_state.current_key, current_mode, current_harmony, current_octave_shift);
      target_frequency_hz = scale_state.current_note_freq;
      Update_Phase_Increment();
      display_counter = 200000;
    } else if (s1_event == BTN_EVENT_DOUBLE_CLICK) {
      effects_enabled = !effects_enabled;
      display_counter = 200000;
    }

    ButtonEvent_t s2_event = Button_GetEvent(&btn_s2);
    if (s2_event == BTN_EVENT_SHORT_CLICK) {
      gSynthState.audio_playing = !gSynthState.audio_playing;
      if (gSynthState.audio_playing) Trigger_Note_On();
      else Trigger_Note_Off();
      display_counter = 200000;
    } else if (s2_event == BTN_EVENT_LONG_PRESS) {
      chord_mode = (ChordMode_t)((chord_mode + 1) % CHORD_MODE_COUNT);
      display_counter = 200000;
    } else if (s2_event == BTN_EVENT_DOUBLE_CLICK) {
      arpeggiator.mode = (arpeggiator.mode == ARP_OFF) ? ARP_UP : ARP_OFF;
      display_counter = 200000;
    }

    ButtonEvent_t joy_sel_event = Button_GetEvent(&btn_joy_sel);
    if (joy_sel_event == BTN_EVENT_SHORT_CLICK) {
      // Toggle GREENSLEEVES / Epic Mode (From v30.2)
      Toggle_Epic_Mode();
      display_counter = 200000;
    } else if (joy_sel_event == BTN_EVENT_LONG_PRESS) {
      // Full Reset
      epic_mode_active = false;
      current_instrument = INSTRUMENT_PIANO;
      current_preset = 0;
      effects_enabled = true;
      chord_mode = CHORD_OFF;
      arpeggiator.mode = ARP_OFF;
      scale_state.current_key = KEY_C;
      current_mode = MODE_MAJOR;
      display_counter = 200000;
    }

    if (loop_counter % 1000 == 0) {
      Joystick_Update(&joystick, gSynthState.joy_x, gSynthState.joy_y);
      Accel_Update(&accel, gSynthState.accel_x, gSynthState.accel_y, gSynthState.accel_z);
      Process_Musical_Controls();
      Process_Accelerometer();
    }
    if (display_counter++ >= 100000) {
      Display_Update();
      display_counter = 0;
    }
    loop_counter++;
  }
}

//=============================================================================
// 7. MUSICAL LOGIC (The "Accurate" Logic from v30.2)
//=============================================================================

static void Process_Musical_Controls(void) {
  if (epic_mode_active) return;

  // Joystick X changes Key (C, D, E...)
  if (joystick.x_changed) {
    if (joystick.raw_x < 1000) {
      if (scale_state.current_key > 0) scale_state.current_key--;
      else scale_state.current_key = (MusicalKey_t)(KEY_COUNT - 1);
    } else if (joystick.raw_x > 3000) {
      if (scale_state.current_key < (KEY_COUNT - 1)) scale_state.current_key++;
      else scale_state.current_key = (MusicalKey_t)0;
    }
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
  }

  // Joystick Y changes Volume
  if (joystick.y_changed) {
    gSynthState.volume = Joystick_GetVolume(&joystick);
  }

  // Accel X changes Harmony (I, ii, iii...) - From v30.2
  if (accel.x_changed) {
    uint8_t harm_pos = (uint8_t)((accel.x * HARM_COUNT) / 4096);
    if (harm_pos >= HARM_COUNT) harm_pos = HARM_COUNT - 1;
    current_harmony = (HarmonicFunction_t)harm_pos;
    
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
  }
}

static void Process_Accelerometer(void) {
  if (epic_mode_active) return;
  
  // "Accurate Tones" Tilt Logic from v30.2
  // +500 = +7 semitones (Perfect Fifth)
  // +1000 = +12 semitones (Octave)
  int16_t deviation = accel.y - ACCEL_Y_NEUTRAL;
  int8_t new_shift = 0;
  
  if (deviation > 1000) new_shift = 12;
  else if (deviation > 500) new_shift = 7;
  else if (deviation < -1000) new_shift = -12;
  else if (deviation < -500) new_shift = -5;
  else new_shift = 0;

  if (current_octave_shift != new_shift) {
    current_octave_shift = new_shift;
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
#if ENABLE_DEBUG_LEDS
    Debug_LED_Update((new_shift > 0) ? 1 : (new_shift < 0 ? -1 : 0));
#endif
  }
}

// The v30.2 Frequency Calculation (Preserved for accuracy)
static uint16_t Calculate_Harmonic_Frequency(MusicalKey_t key,
                                             MusicalMode_t mode,
                                             HarmonicFunction_t harmony,
                                             int8_t octave_shift) {
  uint16_t root_freq = ROOT_FREQUENCIES[key];
  const int8_t *intervals = (mode == MODE_MAJOR)
                                ? HARMONIC_INTERVALS_MAJOR[harmony]
                                : HARMONIC_INTERVALS_MINOR[harmony];
  int32_t semitone = intervals[0] + octave_shift;
  const uint16_t semitone_ratio[] = {1000, 1059, 1122, 1189, 1260, 1335, 1414,
                                     1498, 1587, 1682, 1782, 1888, 2000, 2119,
                                     2245, 2378, 2520, 2670, 2828, 2997, 3175,
                                     3364, 3564, 3775, 4000};
  int32_t idx = semitone + 12;
  int32_t octave_adjust = 0;
  while (idx > 24) {
    idx -= 12;
    octave_adjust++;
  }
  while (idx < 0) {
    idx += 12;
    octave_adjust--;
  }
  uint32_t freq = ((uint32_t)root_freq * semitone_ratio[idx]) / 1000;
  if (octave_adjust > 0) freq <<= octave_adjust;
  else if (octave_adjust < 0) freq >>= -octave_adjust;
  return (freq < 100) ? 100 : (freq > 8000 ? 8000 : (uint16_t)freq);
}


//=============================================================================
// GREENSLEEVES / EPIC MODE (Tab v2)
//=============================================================================

static const struct {
  MusicalKey_t key;
  HarmonicFunction_t harmony;
  MusicalMode_t mode;
  int8_t octave_shift;
} EPIC_SEQUENCE[] = {
    // --- VERSE (PART A) ---
    // Pickup: A
    {KEY_A, HARM_I, MODE_MINOR, 0},

    // Bar 1: C - D - E (High) - F (High) - E (High)
    {KEY_C, HARM_I, MODE_MAJOR, 12}, // C (High octave)
    {KEY_D, HARM_I, MODE_MAJOR, 12}, // D
    {KEY_E, HARM_I, MODE_MINOR, 12}, // E
    {KEY_F, HARM_I, MODE_MAJOR, 12}, // F
    {KEY_E, HARM_I, MODE_MINOR, 12}, // E

    // Bar 2: D - B - G - A - B
    {KEY_D, HARM_I, MODE_MAJOR, 12}, // D
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B (Natural high B)
    {KEY_G, HARM_I, MODE_MAJOR, 0},  // G (Natural low G)
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B

    // Bar 3: C - A - A - G# (Accidental!)
    {KEY_C, HARM_I, MODE_MAJOR, 12}, // C
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G# (Trikset: E + Major 3rd = G#)

    // Bar 4: A - B - G# - E
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G#
    {KEY_E, HARM_I, MODE_MINOR, 0},  // E (Low)

    // --- REPEAT VERSE START ---
    // Bar 5: A - C - D - E - F - E
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A (Pickup)
    {KEY_C, HARM_I, MODE_MAJOR, 12}, // C
    {KEY_D, HARM_I, MODE_MAJOR, 12}, // D
    {KEY_E, HARM_I, MODE_MINOR, 12}, // E
    {KEY_F, HARM_I, MODE_MAJOR, 12}, // F
    {KEY_E, HARM_I, MODE_MINOR, 12}, // E

    // Bar 6: D - B - G - A - B
    {KEY_D, HARM_I, MODE_MAJOR, 12}, // D
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B
    {KEY_G, HARM_I, MODE_MAJOR, 0},  // G
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B

    // Bar 7 (Ending): C - B - A - G# - F# - G#
    {KEY_C, HARM_I, MODE_MAJOR, 12}, // C
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G#
    {KEY_D, HARM_iii, MODE_MAJOR, 0},// F# (Trikset: D + Major 3rd = F#)
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G#

    // Bar 8: A (Resolution)
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A (Hold)


    // --- CHORUS (PART B - High Part) ---
    // Bar 9: G (High) - G - F - E - D
    {KEY_G, HARM_I, MODE_MAJOR, 12}, // G (Very High)
    {KEY_G, HARM_I, MODE_MAJOR, 12}, // G
    {KEY_F, HARM_I, MODE_MAJOR, 12}, // F
    {KEY_E, HARM_I, MODE_MINOR, 12}, // E
    {KEY_D, HARM_I, MODE_MAJOR, 12}, // D

    // Bar 10: B - G - A - B
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B
    {KEY_G, HARM_I, MODE_MAJOR, 0},  // G
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B

    // Bar 11: C - A - A - G#
    {KEY_C, HARM_I, MODE_MAJOR, 12}, // C
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G#

    // Bar 12: A - B - G# - E
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G#
    {KEY_E, HARM_I, MODE_MINOR, 0},  // E

    // --- REPEAT CHORUS ENDING ---
    // Bar 13: G (High) - G - F - E - D
    {KEY_G, HARM_I, MODE_MAJOR, 12}, // G (High)
    {KEY_G, HARM_I, MODE_MAJOR, 12}, // G
    {KEY_F, HARM_I, MODE_MAJOR, 12}, // F
    {KEY_E, HARM_I, MODE_MINOR, 12}, // E
    {KEY_D, HARM_I, MODE_MAJOR, 12}, // D

    // Bar 14: B - G - A - B
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B
    {KEY_G, HARM_I, MODE_MAJOR, 0},  // G
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B

    // Bar 15: C - B - A - G# - F# - G#
    {KEY_C, HARM_I, MODE_MAJOR, 12}, // C
    {KEY_B, HARM_I, MODE_MINOR, 0},  // B
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G#
    {KEY_D, HARM_iii, MODE_MAJOR, 0},// F#
    {KEY_E, HARM_iii, MODE_MAJOR, 0},// G#

    // Final Resolution
    {KEY_A, HARM_I, MODE_MINOR, 0},  // A
    {KEY_A, HARM_I, MODE_MINOR, -12},// Low A
    {KEY_A, HARM_I, MODE_MINOR, -12} // Low A Hold
};
#define EPIC_SEQUENCE_LENGTH (sizeof(EPIC_SEQUENCE) / sizeof(EPIC_SEQUENCE[0]))

static void Process_Epic_Mode(void) {
  if (!epic_mode_active) return;
  epic_step_counter++;
  if (epic_step_counter >= EPIC_STEPS_PER_NOTE) {
    epic_step_counter = 0;
    epic_sequence_step = (epic_sequence_step + 1) % EPIC_SEQUENCE_LENGTH;
    DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN | GPIO_RGB_GREEN_PIN);
    scale_state.current_key = EPIC_SEQUENCE[epic_sequence_step].key;
    current_harmony = EPIC_SEQUENCE[epic_sequence_step].harmony;
    current_mode = EPIC_SEQUENCE[epic_sequence_step].mode;
    current_octave_shift = EPIC_SEQUENCE[epic_sequence_step].octave_shift;
    
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
    Trigger_Note_On();
  }
}

static void Toggle_Epic_Mode(void) {
  epic_mode_active = !epic_mode_active;
  if (epic_mode_active) {
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    LCD_FillScreen(LCD_COLOR_BLACK);
    LCD_PrintString(10, 50, "GREENSLEEVES", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_LARGE);
    LCD_PrintString(35, 70, "MODE!", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_MEDIUM);
    DL_Common_delayCycles(40000000);
    
    current_instrument = INSTRUMENT_STRINGS;
    effects_enabled = true;
    chord_mode = CHORD_OFF;
    arpeggiator.mode = ARP_OFF;
    epic_sequence_step = 0;
    epic_step_counter = 0;
    
    scale_state.current_key = EPIC_SEQUENCE[0].key;
    current_harmony = EPIC_SEQUENCE[0].harmony;
    current_mode = EPIC_SEQUENCE[0].mode;
    current_octave_shift = EPIC_SEQUENCE[0].octave_shift;
    
    gSynthState.waveform = INSTRUMENTS[INSTRUMENT_STRINGS].waveform;
    Envelope_Init(&envelope, &INSTRUMENTS[INSTRUMENT_STRINGS].adsr);
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
    Trigger_Note_On();
  } else {
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    current_octave_shift = 0;
  }
}

//=============================================================================
// 8. HELPERS & AUDIO GENERATION
//=============================================================================

static void Process_Portamento(void) {
  if (current_frequency_hz < target_frequency_hz) {
    current_frequency_hz += PORTAMENTO_SPEED;
    if (current_frequency_hz > target_frequency_hz) current_frequency_hz = target_frequency_hz;
  } else if (current_frequency_hz > target_frequency_hz) {
    current_frequency_hz -= PORTAMENTO_SPEED;
    if (current_frequency_hz < target_frequency_hz) current_frequency_hz = target_frequency_hz;
  }
  if (current_frequency_hz != base_frequency_hz) {
    base_frequency_hz = current_frequency_hz;
    Update_Phase_Increment();
  }
}

void Change_Instrument(void) {
  current_instrument = (Instrument_t)((current_instrument + 1) % INSTRUMENT_COUNT);
  gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
  Envelope_Init(&envelope, &INSTRUMENTS[current_instrument].adsr);
  Trigger_Note_On();
}

void Trigger_Note_On(void) { Envelope_NoteOn(&envelope); }
void Trigger_Note_Off(void) { Envelope_NoteOff(&envelope); }

static void Update_Phase_Increment(void) {
  if (base_frequency_hz == 0) base_frequency_hz = 440;
  int8_t table_index = current_octave_shift + 12;
  if (table_index < 0) table_index = 0;
  if (table_index > 24) table_index = 24;
  
  uint32_t bent_ratio = PITCH_BEND_TABLE[table_index];
  uint64_t bent_freq_64 = ((uint64_t)base_frequency_hz * bent_ratio) >> 16;
  uint32_t bent_freq = (uint32_t)bent_freq_64;
  
  if (bent_freq < FREQ_MIN_HZ) bent_freq = FREQ_MIN_HZ;
  if (bent_freq > FREQ_MAX_HZ) bent_freq = FREQ_MAX_HZ;
  
  if (bent_freq > 0 && bent_freq <= 8000) {
    uint64_t temp = ((uint64_t)bent_freq << 32) / 16000ULL;
    if (temp > 0 && temp <= 0xFFFFFFFF) g_phase_increment = (uint32_t)temp;
    else g_phase_increment = 118111601;
  } else {
    g_phase_increment = 118111601;
  }

  // Handle Chords
  if (chord_mode != CHORD_OFF) {
    const int8_t *intervals = CHORD_INTERVALS[chord_mode];
    for (uint8_t voice = 0; voice < 3; voice++) {
      int8_t chord_table_index = table_index + intervals[voice];
      if (chord_table_index < 0) chord_table_index = 0;
      if (chord_table_index > 24) chord_table_index = 24;
      
      uint32_t chord_ratio = PITCH_BEND_TABLE[chord_table_index];
      uint64_t chord_freq_64 = ((uint64_t)base_frequency_hz * chord_ratio) >> 16;
      uint32_t chord_freq = (uint32_t)chord_freq_64;
      
      if (chord_freq > 0 && chord_freq <= 8000) {
        uint64_t chord_temp = ((uint64_t)chord_freq << 32) / 16000ULL;
        if (chord_temp > 0 && chord_temp <= 0xFFFFFFFF)
           g_chord_increments[voice] = (uint32_t)chord_temp;
        else g_chord_increments[voice] = g_phase_increment;
      }
    }
  } else {
    g_chord_increments[0] = g_chord_increments[1] = g_chord_increments[2] = g_phase_increment;
  }
}

static void Generate_Audio_Sample(void) {
  if (g_phase_increment == 0) g_phase_increment = 118111601;
  uint16_t amplitude = Envelope_GetAmplitude(&envelope);
  
  if (gSynthState.volume == 0 || amplitude == 0) {
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
    g_phase += g_phase_increment;
    gSynthState.audio_samples_generated++;
    return;
  }

  int16_t sample;
  if (chord_mode != CHORD_OFF) {
    sample = Generate_Chord_Sample(g_chord_phases, g_chord_increments);
  } else {
    const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
    uint32_t modulated_phase = g_phase;
    
    // Vibrato
    if (effects_enabled && inst->vibrato_depth > 0) {
      uint8_t vib_index = vibrato_phase >> 8;
      const int16_t *sine = Audio_GetSineTable();
      int32_t phase_offset = ((int32_t)sine[vib_index] * inst->vibrato_depth * (int32_t)g_phase_increment) / 100000;
      modulated_phase = g_phase + phase_offset;
    }
    
    uint8_t index = (uint8_t)((modulated_phase >> 24) & 0xFF);
    sample = Audio_GenerateWaveform(index, inst->waveform); // Use Lib
    
    if (inst->num_harmonics >= 1) {
      uint8_t h1_index = (index << 1) & 0xFF;
      int16_t harmonic1 = Audio_GenerateWaveform(h1_index, inst->waveform);
      sample = (sample * 2 + harmonic1) / 3;
    }
    
    // Tremolo
    if (effects_enabled && inst->tremolo_depth > 0) {
      uint8_t trem_index = tremolo_phase >> 8;
      const int16_t *sine = Audio_GetSineTable();
      int16_t mod = 1000 + ((sine[trem_index] * inst->tremolo_depth) / 100);
      sample = (int16_t)(((int32_t)sample * mod) / 1000);
    }
    g_phase += g_phase_increment;
  }
  
  sample = (int16_t)(((int32_t)sample * amplitude) / 1000);
  sample = (int16_t)(((int32_t)sample * gSynthState.volume) / 100);
  sample = Filter_GainWithFreqCompensation(sample, AUDIO_GAIN_BOOST, base_frequency_hz);
  sample = Filter_LowPass(sample);
  sample = Filter_SoftClip(sample, 28000);

#if ENABLE_WAVEFORM_DISPLAY
  static uint8_t wave_dec = 0;
  // Using v30.2 display logic (decimate by 8) for smoother drawing
  if (++wave_dec >= 8) {
    wave_dec = 0;
    waveform_buffer[waveform_write_index++] = sample;
    if (waveform_write_index >= 64) waveform_write_index = 0;
  }
#endif

  uint16_t pwm_val = Audio_SampleToPWM(sample, 1023, 2047);
  DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, pwm_val, DL_TIMER_CC_0_INDEX);

  if (++uart_decimate_counter >= UART_DECIMATION_FACTOR) {
    uart_decimate_counter = 0;
    uart_buffer[uart_buf_idx++] = sample;
    if (uart_buf_idx >= 16) {
      DL_DMA_setSrcAddr(DMA, DMA_CH_UART_CHAN_ID, (uint32_t)uart_buffer);
      DL_DMA_setTransferSize(DMA, DMA_CH_UART_CHAN_ID, 32);
      DL_DMA_enableChannel(DMA, DMA_CH_UART_CHAN_ID);
      uart_buf_idx = 0;
    }
    uint8_t low_byte = (uint8_t)(sample & 0xFF);
    uint8_t high_byte = (uint8_t)((sample >> 8) & 0xFF);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, low_byte);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, high_byte);
  }
  gSynthState.audio_samples_generated++;
}

static int16_t Generate_Chord_Sample(volatile uint32_t *phases,
                                     volatile uint32_t *increments) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  int32_t mixed = 0;
  uint8_t num_voices = (chord_mode == CHORD_OFF) ? 1 : 3;
  for (uint8_t v = 0; v < num_voices; v++) {
    uint8_t index = (uint8_t)((phases[v] >> 24) & 0xFF);
    int16_t sample = Audio_GenerateWaveform(index, inst->waveform);
    if (inst->num_harmonics >= 1) {
      uint8_t h_index = (index << 1) & 0xFF;
      int16_t harmonic = Audio_GenerateWaveform(h_index, inst->waveform);
      sample = (sample * 2 + harmonic) / 3;
    }
    mixed += sample;
    phases[v] += increments[v];
  }
  return (int16_t)(mixed / num_voices);
}

//=============================================================================
// 9. INTERRUPTS
//=============================================================================
void SysTick_Handler(void) {
  Button_Update(&btn_s1, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_MKII_PIN);
  Button_Update(&btn_s2, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_MKII_PIN);
  Button_Update(&btn_joy_sel, GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
}

void SysTick_Init(void) {
  SysTick->LOAD = SYSTICK_LOAD_VALUE;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void DMA_IRQHandler(void) {
  if (DL_DMA_getPendingInterrupt(DMA) == DL_DMA_EVENT_IIDX_DMACH1) {
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_EVENT_IIDX_DMACH1);
    gADC0_DMA_Complete = true;
  }
}

void ADC0_IRQHandler(void) {
  gSynthState.adc0_count++;
  if(DL_ADC12_getPendingInterrupt(ADC_JOY_INST) == DL_ADC12_IIDX_MEM1_RESULT_LOADED) {
    DL_ADC12_clearInterruptStatus(ADC_JOY_INST, DL_ADC12_IIDX_MEM1_RESULT_LOADED);
    gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
  }
}

void ADC1_IRQHandler(void) {
  gSynthState.adc1_count++;
  if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) == DL_ADC12_IIDX_MEM3_RESULT_LOADED) {
    DL_ADC12_clearInterruptStatus(ADC_ACCEL_INST, DL_ADC12_IIDX_MEM3_RESULT_LOADED);
    gSynthState.accel_x = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.accel_y = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
    gSynthState.accel_z = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
    gSynthState.joy_y   = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_3);
  }
}

void TIMG7_IRQHandler(void) {
  if(DL_TimerG_getPendingInterrupt(TIMER_SAMPLE_INST) & DL_TIMERG_IIDX_ZERO) {
    DL_TimerG_clearInterruptStatus(TIMER_SAMPLE_INST, DL_TIMERG_IIDX_ZERO);
    gSynthState.timer_count++;
    if (g_phase_increment == 0) g_phase_increment = 118111601;

    Envelope_Process(&envelope);
    
    // Process Arpeggiator OR Epic Mode (They share timing logic mostly)
    if(epic_mode_active) Process_Epic_Mode();
    else if(arpeggiator.mode != ARP_OFF) {
        arpeggiator.step_counter++;
        if (arpeggiator.step_counter >= arpeggiator.steps_per_note) {
          arpeggiator.step_counter = 0;
          Trigger_Note_On();
          arpeggiator.current_step = (arpeggiator.current_step + 1) % 8;
        }
    }
    
    Process_Portamento();
    vibrato_phase += 82;
    tremolo_phase += 67;

    if (gSynthState.audio_playing) Generate_Audio_Sample();
    else DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
  }
}

//=============================================================================
// 10. DISPLAY
//=============================================================================
static void Display_Scale_Info(void) {
  char buf[32];
  LCD_DrawRect(0, 28, 128, 10, LCD_COLOR_BLACK);
  const char *mode_name = (current_mode == MODE_MAJOR) ? "MAJ" : "MIN";
  snprintf(buf, sizeof(buf), "%s %s", KEY_NAMES[scale_state.current_key], mode_name);
  LCD_PrintString(3, 28, buf, LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  
  const char **harm_names = (current_mode == MODE_MAJOR) ? HARMONIC_NAMES_MAJOR : HARMONIC_NAMES_MINOR;
  snprintf(buf, sizeof(buf), "%s", harm_names[current_harmony]);
  LCD_PrintString(85, 28, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
}

static void Display_Update(void) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  char buf[32];
  LCD_DrawRect(0, 0, 128, 16, inst->color);
  LCD_PrintString(3, 4, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
  if (epic_mode_active) {
    LCD_PrintString(50, 4, "EPIC", LCD_COLOR_RED, inst->color, FONT_SMALL);
    snprintf(buf, sizeof(buf), "%d/16", epic_sequence_step + 1);
    LCD_PrintString(85, 4, buf, LCD_COLOR_YELLOW, inst->color, FONT_SMALL);
  } else {
    LCD_PrintString(60, 4, PRESETS[current_preset].name, LCD_COLOR_BLACK, inst->color, FONT_SMALL);
  }
  LCD_DrawRect(0, 18, 128, 10, LCD_COLOR_BLACK);
  LCD_PrintString(3, 18, "F:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(18, 18, base_frequency_hz, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
  
  if (current_octave_shift == -12) LCD_PrintString(55, 18, "LOW", LCD_COLOR_BLUE, LCD_COLOR_BLACK, FONT_SMALL);
  else if (current_octave_shift == 12) LCD_PrintString(55, 18, "HI", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
  else LCD_PrintString(55, 18, "MID", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
  
  Display_Scale_Info();
  
  // Volume bar
  LCD_DrawRect(3, 40, 60, 4, LCD_COLOR_DARKGRAY);
  uint8_t bar_w = (gSynthState.volume > 100) ? 100 : gSynthState.volume;
  LCD_DrawRect(3, 40, (bar_w * 60) / 100, 4, LCD_COLOR_GREEN);
  snprintf(buf, sizeof(buf), "%d%%", gSynthState.volume);
  LCD_PrintString(3, 46, buf, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
  
  LCD_DrawRect(66, 40, 62, 10, LCD_COLOR_BLACK);
  LCD_PrintString(66, 40, "FX:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintString(84, 40, effects_enabled ? "ON" : "OFF",
                  effects_enabled ? LCD_COLOR_GREEN : LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
                  
  if (chord_mode != CHORD_OFF) {
    const char *chord_names[] = {"", "MAJ", "MIN"};
    LCD_PrintString(105, 40, chord_names[chord_mode], LCD_COLOR_MAGENTA, LCD_COLOR_BLACK, FONT_SMALL);
  }
  
  LCD_DrawRect(0, 50, 128, 10, LCD_COLOR_BLACK);
  if (arpeggiator.mode != ARP_OFF) LCD_PrintString(3, 50, "ARP", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  
  const char *env_names[] = {"IDLE", "ATK", "DEC", "SUS", "REL"};
  LCD_PrintString(55, 50, env_names[Envelope_GetState(&envelope)], LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(90, 50, Envelope_GetAmplitude(&envelope) / 10, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);

#if ENABLE_WAVEFORM_DISPLAY
  Display_Waveform();
#endif

  LCD_DrawRect(0, 118, 128, 10, LCD_COLOR_BLACK);
  if (gSynthState.audio_playing) LCD_PrintString(3, 118, "PLAYING", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  else LCD_PrintString(3, 118, "STOPPED", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
  snprintf(buf, sizeof(buf), "V:%d", gSynthState.volume);
  LCD_PrintString(70, 118, buf, LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
}

#if ENABLE_WAVEFORM_DISPLAY
static void Display_Waveform(void) {
  uint16_t yc = 85, ys = 25;
  LCD_DrawRect(0, 60, 128, 55, LCD_COLOR_BLACK);
  for (uint8_t x = 0; x < 128; x += 4) LCD_DrawPixel(x, yc, LCD_COLOR_DARKGRAY);
  for (uint8_t i = 0; i < 63; i++) {
    int16_t y1 = yc - ((waveform_buffer[i] * ys) / 1000);
    int16_t y2 = yc - ((waveform_buffer[i + 1] * ys) / 1000);
    if (y1 < 60) y1 = 60; if (y1 > 110) y1 = 110;
    if (y2 < 60) y2 = 60; if (y2 > 110) y2 = 110;
    LCD_DrawLine(i * 2, y1, (i + 1) * 2, y2, LCD_COLOR_CYAN);
  }
}
#endif

#if ENABLE_DEBUG_LEDS
static void Debug_LED_Update(int8_t octave) {
  if (octave < 0) {
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
  } else if (octave > 0) {
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
  } else {
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
  }
}
#endif

void HardFault_Handler(void) {
  while (1) {
    DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    for (volatile uint32_t i = 0; i < 100000; i++);
  }
}