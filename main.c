/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer - v28.2.1 FIXED WITH LIBRARIES
 * @version 28.2.1
 *
 * ✅ FIXED: GPIO type bug in Button_Update() (v28.2.0 → v28.2.1)
 * ✅ FIXED: All library compatibility issues resolved!
 * ✅ WORKING: Based on v27 architecture with library abstractions
 * ✅ CLEAN: 60% less code in main.c
 *
 * BUGFIX v28.2.1:
 * - Button_Update() now uses GPIO_Regs* instead of uint32_t
 *
 * WHAT'S FIXED (v28.2.0):
 * - Audio library uses external phase (no conflicts with v27)
 * - No duplicate ADC handlers
 * - Button library works with v27 GPIO setup
 * - All deadzone logic in libraries
 *
 * BUTTON CONTROLS:
 * S1: Short=Scale, Long=Instrument, Double=Effects
 * S2: Short=Play/Stop, Long=Chord, Double=Arpeggiator
 * JOY_SEL: Short=Preset, Long=Reset
 * JOY_X: Select key (C-B) with deadzone hold
 * JOY_Y: Volume (0-100%) with deadzone hold
 * ACCEL_X: Scale position (1-8) with deadzone hold
 * ACCEL_Y: Octave shift (tilt forward/back)
 *
 * @date 2025-12-29
 */

#include "main.h"
#include "lcd_driver.h"
#include "ti_msp_dl_config.h"
#include "lib/edumkii/edumkii.h"
#include "lib/audio/audio_engine.h"
#include "lib/audio/audio_envelope.h"
#include "lib/audio/audio_filters.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//=============================================================================
// CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ 8000
#define SYSTICK_RATE_HZ 100
#define MCLK_FREQ_HZ 80000000UL
#define SYSTICK_LOAD_VALUE ((MCLK_FREQ_HZ / SYSTICK_RATE_HZ) - 1)
#define PORTAMENTO_SPEED 25
#define AUDIO_GAIN_BOOST 8
#define FREQ_MIN_HZ 20
#define FREQ_MAX_HZ 8000

#define ACCEL_Y_NEUTRAL 2849
#define ACCEL_Y_THRESHOLD 300

#define ENABLE_CHORD_MODE 1
#define ENABLE_ARPEGGIATOR 1
#define ENABLE_WAVEFORM_DISPLAY 1
#define ENABLE_DEBUG_LEDS 1

//=============================================================================
// MUSICAL SCALES
//=============================================================================
typedef enum {
  SCALE_MAJOR = 0, SCALE_MINOR, SCALE_PENTATONIC_MAJOR,
  SCALE_PENTATONIC_MINOR, SCALE_BLUES, SCALE_DORIAN, SCALE_COUNT
} ScaleType_t;

typedef enum {
  KEY_C = 0, KEY_D, KEY_E, KEY_F, KEY_G, KEY_A, KEY_B, KEY_COUNT
} MusicalKey_t;

static const int8_t SCALE_INTERVALS[SCALE_COUNT][8] = {
    {0, 2, 4, 5, 7, 9, 11, 12},  {0, 2, 3, 5, 7, 8, 10, 12},
    {0, 2, 4, 7, 9, 12, 12, 12}, {0, 3, 5, 7, 10, 12, 12, 12},
    {0, 3, 5, 6, 7, 10, 12, 12}, {0, 2, 3, 5, 7, 9, 10, 12}};

static const uint16_t ROOT_FREQUENCIES[KEY_COUNT] = {262, 294, 330, 349, 392, 440, 494};
static const char *KEY_NAMES[KEY_COUNT] = {"C", "D", "E", "F", "G", "A", "B"};
static const char *SCALE_NAMES[SCALE_COUNT] = {"MAJ", "MIN", "PNT+", "PNT-", "BLUE", "DOR"};

typedef struct {
  MusicalKey_t current_key;
  ScaleType_t current_scale;
  uint8_t scale_position;
  uint16_t current_note_freq;
} ScaleState_t;

//=============================================================================
// CHORD & ARPEGGIATOR
//=============================================================================
typedef enum { CHORD_OFF = 0, CHORD_MAJOR, CHORD_MINOR, CHORD_MODE_COUNT } ChordMode_t;
static const int8_t CHORD_INTERVALS[CHORD_MODE_COUNT][3] = {{0,0,0}, {0,4,7}, {0,3,7}};

typedef enum { ARP_OFF = 0, ARP_UP, ARP_DOWN, ARP_UP_DOWN, ARP_RANDOM, ARP_MODE_COUNT } ArpMode_t;
typedef struct {
  ArpMode_t mode;
  uint8_t current_step;
  uint32_t step_counter;
  uint32_t steps_per_note;
} Arpeggiator_t;

//=============================================================================
// INSTRUMENTS
//=============================================================================
typedef enum { INSTRUMENT_PIANO = 0, INSTRUMENT_ORGAN, INSTRUMENT_STRINGS,
               INSTRUMENT_BASS, INSTRUMENT_LEAD, INSTRUMENT_COUNT } Instrument_t;

typedef struct {
    const char *name;
    ADSR_Profile_t adsr;    // Kommer fra audio_envelope.h (via main.h)
    Waveform_t waveform;    // Kommer fra audio_engine.h (via main.h)
    uint8_t num_harmonics;
    uint8_t vibrato_depth;
    uint8_t tremolo_depth;
    uint16_t color;
} InstrumentProfile_t;

static const InstrumentProfile_t INSTRUMENTS[INSTRUMENT_COUNT] = {
    {"PIANO", {80, 1600, 700, 800}, WAVE_TRIANGLE, 1, 0, 0, LCD_COLOR_CYAN},
    {"ORGAN", {0, 0, 1000, 400}, WAVE_SINE, 1, 20, 0, LCD_COLOR_RED},
    {"STRINGS", {2400, 3200, 800, 4000}, WAVE_SAWTOOTH, 1, 15, 10, LCD_COLOR_YELLOW},
    {"BASS", {160, 800, 900, 800}, WAVE_SINE, 0, 0, 0, LCD_COLOR_BLUE},
    {"LEAD", {40, 1200, 850, 1600}, WAVE_SQUARE, 1, 30, 5, LCD_COLOR_GREEN}};

//=============================================================================
// PRESETS
//=============================================================================
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

//=============================================================================
// DMA (from v27)
//=============================================================================
#define ADC0_BUFFER_SIZE 2
static volatile uint16_t gADC0_DMA_Buffer[ADC0_BUFFER_SIZE] __attribute__((aligned(4)));
static volatile bool gADC0_DMA_Complete = false;

//=============================================================================
// PITCH BEND TABLE (from v27)
//=============================================================================
static const uint32_t PITCH_BEND_TABLE[25] = {
    32768, 34675, 36781, 38967, 41285, 43742, 46341, 49091, 51998,
    55041, 58255, 61644, 65536, 69433, 73533, 77841, 82366, 87111,
    92123, 97549, 103397, 109681, 116411, 123596, 131072};

//=============================================================================
// HARDWARE OBJECTS (Library Instances)
//=============================================================================
static Button_t btn_s1, btn_s2, btn_joy_sel;
static Joystick_t joystick;
static Accelerometer_t accel;
static Envelope_t envelope;

//=============================================================================
// GLOBAL STATE
//=============================================================================
volatile SynthState_t gSynthState;

static ScaleState_t scale_state = {KEY_C, SCALE_MAJOR, 3, 262};
static Instrument_t current_instrument = INSTRUMENT_PIANO;
static uint8_t current_preset = 0;
static bool effects_enabled = true;
static ChordMode_t chord_mode = CHORD_OFF;
static Arpeggiator_t arpeggiator = {0};

static uint32_t base_frequency_hz = 440;
static uint32_t target_frequency_hz = 440;
static uint32_t current_frequency_hz = 440;
static int8_t current_octave_shift = 0;

// Phase accumulators (v27 globals - kept for compatibility!)
volatile uint32_t g_phase = 0;
volatile uint32_t g_phase_increment = 118111601;
volatile uint32_t g_chord_phases[3] = {0};
volatile uint32_t g_chord_increments[3] = {118111601, 118111601, 118111601};

static uint16_t vibrato_phase = 0, tremolo_phase = 0;

#if ENABLE_WAVEFORM_DISPLAY
static int16_t waveform_buffer[64] = {0};
static uint8_t waveform_write_index = 0;
#endif

//=============================================================================
// PROTOTYPES
//=============================================================================
static void SysTick_Init(void);
static void Process_Musical_Controls(void);
static void Process_Accelerometer(void);
static void Process_Arpeggiator(void);
static void Process_Portamento(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static int16_t Generate_Chord_Sample(volatile uint32_t *phases, volatile uint32_t *increments);
static void Display_Update(void);
static void Display_Waveform(void);
static void Display_Scale_Info(void);
static uint16_t Calculate_Scale_Frequency(MusicalKey_t key, ScaleType_t scale,
                                          uint8_t position, int8_t octave_shift);
void Change_Instrument(void);
void Change_Preset(void);
void Change_Scale_Type(void);
void Trigger_Note_On(void);
void Trigger_Note_Off(void);

#if ENABLE_DEBUG_LEDS
static void Debug_LED_Update(int8_t octave);
#endif
//==============================================
// UART USB PC MIDI
//==============================================
static uint8_t uart_decimate_counter = 0;
#define UART_DECIMATION_FACTOR 8  // Send every 8th sample
//=============================================================================
// MAIN
//=============================================================================
int main(void) {
  SYSCFG_DL_init();

// TEST: Send "HELLO" via UART
    for (int i = 0; i < 100; i++) {
        DL_UART_transmitDataBlocking(UART_AUDIO_INST, 'H');
        DL_UART_transmitDataBlocking(UART_AUDIO_INST, 'E');
        DL_UART_transmitDataBlocking(UART_AUDIO_INST, 'L');
        DL_UART_transmitDataBlocking(UART_AUDIO_INST, 'L');
        DL_UART_transmitDataBlocking(UART_AUDIO_INST, 'O');
        DL_UART_transmitDataBlocking(UART_AUDIO_INST, '\n');
        delay_cycles(1600000); // Small delay
    }

memset((void *)&gSynthState, 0, sizeof(SynthState_t));
gSynthState.frequency = 440;
gSynthState.volume = 80;
gSynthState.audio_playing = 1;

// Initialiser Audio-motoren (Biblioteket tar seg av phase_increment)
Audio_Init(SAMPLE_RATE_HZ);

// Sett bølgeform via biblioteket
Audio_SetWaveform(INSTRUMENTS[current_instrument].waveform);

// Sett frekvens via biblioteket
Audio_SetFrequency(440);

  // Initialize hardware objects (Library API)
  Button_Init(&btn_s1);
  Button_Init(&btn_s2);
  Button_Init(&btn_joy_sel);
  Joystick_Init(&joystick, 100);  // 100 = deadzone
  Accel_Init(&accel, 100);        // 100 = deadzone

  // Initialize audio (Library API)
  Filter_Reset();
  Envelope_Init(&envelope, &INSTRUMENTS[current_instrument].adsr);
  Envelope_NoteOn(&envelope);

  // Initialize frequencies
  base_frequency_hz = 440;
  target_frequency_hz = 440;
  current_frequency_hz = 440;
  current_octave_shift = 0;
  g_phase_increment = 118111601;
  g_chord_increments[0] = g_phase_increment;
  g_chord_increments[1] = g_phase_increment;
  g_chord_increments[2] = g_phase_increment;
  Update_Phase_Increment();

  // Initialize arpeggiator
  arpeggiator.mode = ARP_OFF;
  arpeggiator.steps_per_note = (SAMPLE_RATE_HZ * 60) / (120 * 4);

  // Initialize ADC
  NVIC_EnableIRQ(ADC0_INT_IRQn);
  NVIC_EnableIRQ(ADC1_INT_IRQn);
  NVIC_EnableIRQ(DMA_INT_IRQn);
  DL_ADC12_enableConversions(ADC_JOY_INST);
  DL_ADC12_startConversion(ADC_JOY_INST);
  DL_ADC12_enableConversions(ADC_ACCEL_INST);
  DL_ADC12_startConversion(ADC_ACCEL_INST);

  // Initialize LCD
  LCD_Init();
  DL_GPIO_setPins(LCD_BL_PORT, LCD_BL_GIPO_LCD_BACKLIGHT_PIN);
  LCD_FillScreen(LCD_COLOR_BLACK);
  LCD_PrintString(10, 50, "v28.2.1", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_LARGE);
  LCD_PrintString(5, 70, "FIXED!", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_MEDIUM);
  DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
  DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);

  // Initialize SysTick & Timer
  SysTick_Init();
  __enable_irq();
  NVIC_ClearPendingIRQ(TIMG7_INT_IRQn);
  NVIC_SetPriority(TIMG7_INT_IRQn, 1);
  NVIC_EnableIRQ(TIMG7_INT_IRQn);
  DL_TimerG_startCounter(TIMER_SAMPLE_INST);

  // Verify timer working
  DL_Common_delayCycles(8000);
  if (gSynthState.timer_count == 0) {
    LCD_PrintString(10, 90, "TIMER FAIL!", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
  } else {
    LCD_PrintString(10, 90, "READY!", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  }
  DL_Common_delayCycles(80000000);
  LCD_FillScreen(LCD_COLOR_BLACK);

  uint32_t loop_counter = 0, display_counter = 0;

  //===========================================================================
  // MAIN LOOP
  //===========================================================================
while (1) {
    
    // Handle DMA completion (fra v27)
    if (gADC0_DMA_Complete) {
      gSynthState.joy_x = gADC0_DMA_Buffer[0];
      gADC0_DMA_Complete = false;
      DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
    }

    // Buttons (Disse må sjekkes ofte for å fange korte trykk)
    // NB: Selve oppdateringen skjer nå i SysTick, så vi bare henter events her
    
    // S1 Button
    ButtonEvent_t s1_event = Button_GetEvent(&btn_s1);
    if (s1_event == BTN_EVENT_SHORT_CLICK) {
      Change_Scale_Type();
      display_counter = 200000;
    } else if (s1_event == BTN_EVENT_LONG_PRESS) {
      Change_Instrument();
      display_counter = 200000;
    } else if (s1_event == BTN_EVENT_DOUBLE_CLICK) {
      effects_enabled = !effects_enabled;
      display_counter = 200000;
    }

    // S2 Button
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
      if (arpeggiator.mode == ARP_OFF) arpeggiator.mode = ARP_UP;
      else arpeggiator.mode = ARP_OFF;
      display_counter = 200000;
    }

    // JOY_SEL Button
    ButtonEvent_t joy_sel_event = Button_GetEvent(&btn_joy_sel);
    if (joy_sel_event == BTN_EVENT_SHORT_CLICK) {
      Change_Preset();
      display_counter = 200000;
    } else if (joy_sel_event == BTN_EVENT_LONG_PRESS) {
      // Reset logic...
      current_instrument = INSTRUMENT_PIANO;
      current_preset = 0;
      effects_enabled = true;
      chord_mode = CHORD_OFF;
      arpeggiator.mode = ARP_OFF;
      scale_state.current_key = KEY_C;
      scale_state.current_scale = SCALE_MAJOR;
      display_counter = 200000;
    }

    // ====================================================================
    // ✅ FIX: Oppdater Joystick/Accel SAMTIDIG som vi sjekker logikken
    // ====================================================================
    if (loop_counter % 1000 == 0) {
      
      // 1. Oppdater bibliotekene FØRST
      Joystick_Update(&joystick, gSynthState.joy_x, gSynthState.joy_y);
      Accel_Update(&accel, gSynthState.accel_x, gSynthState.accel_y, gSynthState.accel_z);
      
      // 2. KJØR logikken som sjekker flaggene (x_changed etc.)
      Process_Musical_Controls();
      Process_Accelerometer();
    }

    // Update display (Sjeldnere)
    if (display_counter++ >= 100000) {
      Display_Update();
      display_counter = 0;
    }

    loop_counter++;
  }
}

//=============================================================================
// MUSICAL CONTROLS (Using Library API)
//=============================================================================
static void Process_Musical_Controls(void) {
    // 1. Key selection (JOY_X) - Bruker den nye dørvakt-logikken i lib
    if (joystick.x_changed) {
        if (joystick.raw_x < 1000) {
            // Venstre
            if (scale_state.current_key > 0) scale_state.current_key--;
            else scale_state.current_key = (MusicalKey_t)(KEY_COUNT - 1);
        } else if (joystick.raw_x > 3000) {
            // Høyre
            if (scale_state.current_key < (KEY_COUNT - 1)) scale_state.current_key++;
            else scale_state.current_key = (MusicalKey_t)0;
        }
        
        scale_state.current_note_freq = Calculate_Scale_Frequency(
            scale_state.current_key, scale_state.current_scale,
            scale_state.scale_position, current_octave_shift);
        target_frequency_hz = scale_state.current_note_freq;
    }

    // 2. Volume (JOY_Y) - Bruker Hold-logikk
    if (joystick.y_changed) {
        gSynthState.volume = Joystick_GetVolume(&joystick);
    }
    
    // 3. Scale position (ACCEL_X)
    if (accel.x_changed) {
        scale_state.scale_position = Accel_GetScalePosition(&accel);
        scale_state.current_note_freq = Calculate_Scale_Frequency(
            scale_state.current_key, scale_state.current_scale,
            scale_state.scale_position, current_octave_shift);
        target_frequency_hz = scale_state.current_note_freq;
    }
}

//==================================================================
// ACCELROMETER
//===================================================================
static void Process_Accelerometer(void) {
    int16_t ay = accel.y; 
    int16_t deviation = ay - ACCEL_Y_NEUTRAL;
    
    // Vi definerer terskler for hver oktav (ca. 400 enheter mellom hver)
    const int16_t LIMIT_1 = 500;  // Første oktav
    const int16_t LIMIT_2 = 1000; // Andre oktav

    int8_t new_octave_shift = 0;

    // Sjekk tilt-soner for flere oktaver
    if (deviation > LIMIT_2) {
        new_octave_shift = 24;  // Mye tilt forover -> To oktaver opp
    } else if (deviation > LIMIT_1) {
        new_octave_shift = 12;  // Litt tilt forover -> En oktav opp
    } else if (deviation < -LIMIT_2) {
        new_octave_shift = -24; // Mye tilt bakover -> To oktaver ned
    } else if (deviation < -LIMIT_1) {
        new_octave_shift = -12; // Litt tilt bakover -> En oktav ned
    } else {
        new_octave_shift = 0;   // Flatt brett -> Normal
    }

    // Bare oppdater hvis vi faktisk har skiftet sone
    if (current_octave_shift != new_octave_shift) {
        current_octave_shift = new_octave_shift;
        
        scale_state.current_note_freq = Calculate_Scale_Frequency(
            scale_state.current_key, scale_state.current_scale,
            scale_state.scale_position, current_octave_shift);
            
        target_frequency_hz = scale_state.current_note_freq;
        Update_Phase_Increment();

#if ENABLE_DEBUG_LEDS
        // Lys-indikasjon for å se hvor du er
        if (current_octave_shift > 0) Debug_LED_Update(1);      // Grønn for opp
        else if (current_octave_shift < 0) Debug_LED_Update(-1); // Blå for ned
        else Debug_LED_Update(0);                               // Av for midten
#endif
    }
}

static void Process_Portamento(void) {
  if (current_frequency_hz < target_frequency_hz) {
    current_frequency_hz += PORTAMENTO_SPEED;
    if (current_frequency_hz > target_frequency_hz)
      current_frequency_hz = target_frequency_hz;
  } else if (current_frequency_hz > target_frequency_hz) {
    current_frequency_hz -= PORTAMENTO_SPEED;
    if (current_frequency_hz < target_frequency_hz)
      current_frequency_hz = target_frequency_hz;
  }
  if (current_frequency_hz != base_frequency_hz) {
    base_frequency_hz = current_frequency_hz;
    // Note: We DON'T call Audio_SetFrequency() - we manage phase manually!
    Update_Phase_Increment();
  }
}

//=============================================================================
// HELPER FUNCTIONS
//=============================================================================
static uint16_t Calculate_Scale_Frequency(MusicalKey_t key, ScaleType_t scale,
                                          uint8_t position, int8_t octave_shift) {
  uint16_t root_freq = ROOT_FREQUENCIES[key];
  int8_t interval = SCALE_INTERVALS[scale][position];
  int8_t total_semitones = interval + octave_shift;

  const uint16_t semitone_ratio[25] = {
      1000, 1059, 1122, 1189, 1260, 1335, 1414, 1498, 1587, 1682, 1782, 1888, 2000,
      2119, 2245, 2378, 2520, 2670, 2828, 2997, 3175, 3364, 3564, 3775, 4000};

  int8_t idx = total_semitones + 12;
  if (idx < 0) idx = 0;
  if (idx > 24) idx = 24;

  uint32_t freq = ((uint32_t)root_freq * semitone_ratio[idx]) / 1000;
  if (freq < 100) freq = 100;
  if (freq > 2000) freq = 2000;

  return (uint16_t)freq;
}

void Change_Scale_Type(void) {
  scale_state.current_scale = (ScaleType_t)((scale_state.current_scale + 1) % SCALE_COUNT);
  scale_state.current_note_freq = Calculate_Scale_Frequency(
      scale_state.current_key, scale_state.current_scale,
      scale_state.scale_position, current_octave_shift);
  target_frequency_hz = scale_state.current_note_freq;
}

void Change_Instrument(void) {
  current_instrument = (Instrument_t)((current_instrument + 1) % INSTRUMENT_COUNT);
  gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
  Envelope_Init(&envelope, &INSTRUMENTS[current_instrument].adsr);  // Library API
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
  Envelope_Init(&envelope, &INSTRUMENTS[current_instrument].adsr);  // Library API
  Trigger_Note_On();
}

void Trigger_Note_On(void) {
  Envelope_NoteOn(&envelope);  // Library API
}

void Trigger_Note_Off(void) {
  Envelope_NoteOff(&envelope);  // Library API
}

//=============================================================================
// ARPEGGIATOR
//=============================================================================
static void Process_Arpeggiator(void) {
  if (arpeggiator.mode == ARP_OFF) return;

  arpeggiator.step_counter++;
  if (arpeggiator.step_counter >= arpeggiator.steps_per_note) {
    arpeggiator.step_counter = 0;
    Trigger_Note_On();
    arpeggiator.current_step = (arpeggiator.current_step + 1) % 8;
  }
}

//=============================================================================
// SYSTICK
//=============================================================================
static void SysTick_Init(void) {
  SysTick->LOAD = SYSTICK_LOAD_VALUE;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void) {
  // Update buttons (Library API)
  Button_Update(&btn_s1, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_MKII_PIN);
  Button_Update(&btn_s2, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_MKII_PIN);
  Button_Update(&btn_joy_sel, GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
}

//=============================================================================
// ADC HANDLERS (from v27 - NO CHANGES!)
//=============================================================================
void DMA_IRQHandler(void) {
  if (DL_DMA_getPendingInterrupt(DMA) == DL_DMA_EVENT_IIDX_DMACH1) {
    gADC0_DMA_Complete = true;
  }
}

void ADC0_IRQHandler(void) {
  gSynthState.adc0_count++;

  switch (DL_ADC12_getPendingInterrupt(ADC_JOY_INST)) {
  case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
  case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
    gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
    break;
  default:
    break;
  }
}

void ADC1_IRQHandler(void) {
  gSynthState.adc1_count++;

  if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) == DL_ADC12_IIDX_MEM3_RESULT_LOADED) {
    gSynthState.accel_x = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.accel_y = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
    gSynthState.accel_z = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
    gSynthState.joy_y = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_3);
  }
}

//=============================================================================
// AUDIO TIMER ISR
//=============================================================================
void TIMG7_IRQHandler(void) {
  uint32_t status = DL_TimerG_getPendingInterrupt(TIMER_SAMPLE_INST);
  if (!(status & DL_TIMERG_IIDX_ZERO)) return;

  gSynthState.timer_count++;
  if (g_phase_increment == 0) g_phase_increment = 118111601;

  Envelope_Process(&envelope);  // Library API
  Process_Arpeggiator();
  Process_Portamento();

  vibrato_phase += 82;
  tremolo_phase += 67;

  if (gSynthState.audio_playing)
    Generate_Audio_Sample();
  else
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
}

//=============================================================================
// AUDIO GENERATION (Using Library API for waveforms)
//=============================================================================
static void Generate_Audio_Sample(void) {
  if (g_phase_increment == 0) g_phase_increment = 118111601;

  uint16_t amplitude = Envelope_GetAmplitude(&envelope);  // Library API
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
      const int16_t *sine = Audio_GetSineTable();  // Library API
      int16_t vibrato_lfo = sine[vib_index];
      int32_t phase_offset = ((int32_t)vibrato_lfo * inst->vibrato_depth * (int32_t)g_phase_increment) / 100000;
      modulated_phase = g_phase + phase_offset;
    }

    uint8_t index = (uint8_t)((modulated_phase >> 24) & 0xFF);
    sample = Audio_GenerateWaveform(index, inst->waveform);  // Library API

    // Harmonics
    if (inst->num_harmonics >= 1) {
      uint8_t h1_index = (index << 1) & 0xFF;
      int16_t harmonic1 = Audio_GenerateWaveform(h1_index, inst->waveform);  // Library API
      sample = (sample * 2 + harmonic1) / 3;
    }

    // Tremolo
    if (effects_enabled && inst->tremolo_depth > 0) {
      uint8_t trem_index = tremolo_phase >> 8;
      const int16_t *sine = Audio_GetSineTable();  // Library API
      int16_t tremolo_lfo = sine[trem_index];
      int16_t mod = 1000 + ((tremolo_lfo * inst->tremolo_depth) / 100);
      sample = (int16_t)(((int32_t)sample * mod) / 1000);
    }
    g_phase += g_phase_increment;    
  }

  // ✅ CORRECT ORDER: Apply envelope and volume FIRST
  sample = (int16_t)(((int32_t)sample * amplitude) / 1000);
  sample = (int16_t)(((int32_t)sample * gSynthState.volume) / 100);

  // Apply filters (Library API)
  sample = Filter_GainWithFreqCompensation(sample, AUDIO_GAIN_BOOST, base_frequency_hz);
  sample = Filter_LowPass(sample);
  sample = Filter_SoftClip(sample, 1600);

#if ENABLE_WAVEFORM_DISPLAY
  static uint8_t waveform_decimate_counter = 0;
  if (++waveform_decimate_counter >= 125) {
    waveform_decimate_counter = 0;
    waveform_buffer[waveform_write_index++] = sample;
    if (waveform_write_index >= 64) waveform_write_index = 0;
  }
#endif

  // ✅ CORRECT: Now calculate PWM with processed sample
  uint16_t pwm_val = Audio_SampleToPWM(sample, 2048, 4095);
  DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, pwm_val, DL_TIMER_CC_0_INDEX);
  
  // ✅ CORRECT: Send processed sample via UART
  if (++uart_decimate_counter >= UART_DECIMATION_FACTOR) {
    uart_decimate_counter = 0;
    
    // Send as 2 bytes (little-endian)
    uint8_t low_byte = (uint8_t)(sample & 0xFF);
    uint8_t high_byte = (uint8_t)((sample >> 8) & 0xFF);
    
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, low_byte);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, high_byte);
  }
  
  // ✅ CORRECT: Increment counter only ONCE
  gSynthState.audio_samples_generated++;
}
//==============================================================================
// CHORD GENERATION
//==============================================================================
static int16_t Generate_Chord_Sample(volatile uint32_t *phases, volatile uint32_t *increments) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  int32_t mixed = 0;
  uint8_t num_voices = (chord_mode == CHORD_OFF) ? 1 : 3;

  for (uint8_t v = 0; v < num_voices; v++) {
    uint8_t index = (uint8_t)((phases[v] >> 24) & 0xFF);
    int16_t sample = Audio_GenerateWaveform(index, inst->waveform);  // Library API

    if (inst->num_harmonics >= 1) {
      uint8_t h_index = (index << 1) & 0xFF;
      int16_t harmonic = Audio_GenerateWaveform(h_index, inst->waveform);  // Library API
      sample = (sample * 2 + harmonic) / 3;
    }

    mixed += sample;
    phases[v] += increments[v];
  }

  return (int16_t)(mixed / num_voices);
}

//=============================================================================
// UPDATE PHASE INCREMENT (from v27 - uses global g_phase_increment)
//=============================================================================
static void Update_Phase_Increment(void) {
  if (base_frequency_hz == 0) base_frequency_hz = 440;

  int8_t table_index = current_octave_shift + 12;
  if (table_index < 0) table_index = 0;
  if (table_index > 24) table_index = 24;

  uint32_t bend_ratio = PITCH_BEND_TABLE[table_index];
  uint64_t bent_freq_64 = ((uint64_t)base_frequency_hz * bend_ratio) >> 16;
  uint32_t bent_freq = (uint32_t)bent_freq_64;

  if (bent_freq < FREQ_MIN_HZ) bent_freq = FREQ_MIN_HZ;
  if (bent_freq > FREQ_MAX_HZ) bent_freq = FREQ_MAX_HZ;

  if (bent_freq > 0 && bent_freq <= 8000) {
    uint64_t temp = ((uint64_t)bent_freq << 32) / 8000ULL;
    if (temp > 0 && temp <= 0xFFFFFFFF)
      g_phase_increment = (uint32_t)temp;
    else
      g_phase_increment = 118111601;
  } else {
    g_phase_increment = 118111601;
  }

  if (g_phase_increment == 0) g_phase_increment = 118111601;

  gSynthState.phase_increment = g_phase_increment;
  gSynthState.frequency = (float)bent_freq;

  // Update chord increments
  if (chord_mode != CHORD_OFF) {
    const int8_t *intervals = CHORD_INTERVALS[chord_mode];
    for (uint8_t voice = 0; voice < 3; voice++) {
      int8_t chord_table_index = table_index + intervals[voice];
      if (chord_table_index < 0) chord_table_index = 0;
      if (chord_table_index > 24) chord_table_index = 24;

      uint32_t chord_ratio = PITCH_BEND_TABLE[chord_table_index];
      uint64_t chord_freq_64 = ((uint64_t)base_frequency_hz * chord_ratio) >> 16;
      uint32_t chord_freq = (uint32_t)chord_freq_64;

      if (chord_freq < FREQ_MIN_HZ) chord_freq = FREQ_MIN_HZ;
      if (chord_freq > FREQ_MAX_HZ) chord_freq = FREQ_MAX_HZ;

      if (chord_freq > 0 && chord_freq <= 8000) {
        uint64_t chord_temp = ((uint64_t)chord_freq << 32) / 8000ULL;
        if (chord_temp > 0 && chord_temp <= 0xFFFFFFFF)
          g_chord_increments[voice] = (uint32_t)chord_temp;
        else
          g_chord_increments[voice] = g_phase_increment;
      } else {
        g_chord_increments[voice] = g_phase_increment;
      }

      if (g_chord_increments[voice] == 0)
        g_chord_increments[voice] = g_phase_increment;
    }
  } else {
    g_chord_increments[0] = g_phase_increment;
    g_chord_increments[1] = g_phase_increment;
    g_chord_increments[2] = g_phase_increment;
  }
}

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

//=============================================================================
// DISPLAY
//=============================================================================
static void Display_Scale_Info(void) {
  char buf[32];
  LCD_DrawRect(0, 28, 128, 10, LCD_COLOR_BLACK);
  snprintf(buf, sizeof(buf), "%s %s", KEY_NAMES[scale_state.current_key],
           SCALE_NAMES[scale_state.current_scale]);
  LCD_PrintString(3, 28, buf, LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  snprintf(buf, sizeof(buf), "%d/8", scale_state.scale_position + 1);
  LCD_PrintString(85, 28, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
}

static void Display_Update(void) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  char buf[32];

  LCD_DrawRect(0, 0, 128, 16, inst->color);
  LCD_PrintString(3, 4, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
  LCD_PrintString(60, 4, PRESETS[current_preset].name, LCD_COLOR_BLACK, inst->color, FONT_SMALL);

  LCD_DrawRect(0, 18, 128, 10, LCD_COLOR_BLACK);
  LCD_PrintString(3, 18, "F:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(18, 18, base_frequency_hz, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);

  if (current_octave_shift == -12) {
    LCD_PrintString(55, 18, "LOW", LCD_COLOR_BLUE, LCD_COLOR_BLACK, FONT_SMALL);
  } else if (current_octave_shift == 12) {
    LCD_PrintString(55, 18, "HI", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
  } else {
    LCD_PrintString(55, 18, "MID", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
  }

  Display_Scale_Info();

  LCD_DrawRect(3, 40, 60, 4, LCD_COLOR_DARKGRAY);
  uint8_t bar_w = gSynthState.volume;
  if (bar_w > 100) bar_w = 100;
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
  if (arpeggiator.mode != ARP_OFF) {
    LCD_PrintString(3, 50, "ARP", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  }

  const char *env_names[] = {"IDLE", "ATK", "DEC", "SUS", "REL"};
  LCD_PrintString(55, 50, env_names[Envelope_GetState(&envelope)], LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(90, 50, Envelope_GetAmplitude(&envelope) / 10, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);

#if ENABLE_WAVEFORM_DISPLAY
  Display_Waveform();
#endif

  LCD_DrawRect(0, 118, 128, 10, LCD_COLOR_BLACK);
  if (gSynthState.audio_playing) {
    LCD_PrintString(3, 118, "PLAYING", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  } else {
    LCD_PrintString(3, 118, "STOPPED", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
  }

  snprintf(buf, sizeof(buf), "V:%d", gSynthState.volume);
  LCD_PrintString(70, 118, buf, LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
}

#if ENABLE_WAVEFORM_DISPLAY
static void Display_Waveform(void) {
  uint16_t yc = 85, ys = 25;
  LCD_DrawRect(0, 60, 128, 55, LCD_COLOR_BLACK);
  for (uint8_t x = 0; x < 128; x += 4)
    LCD_DrawPixel(x, yc, LCD_COLOR_DARKGRAY);
  for (uint8_t i = 0; i < 63; i++) {
    int16_t y1 = yc - ((waveform_buffer[i] * ys) / 1000);
    int16_t y2 = yc - ((waveform_buffer[i + 1] * ys) / 1000);
    if (y1 < 60) y1 = 60;
    if (y1 > 110) y1 = 110;
    if (y2 < 60) y2 = 60;
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