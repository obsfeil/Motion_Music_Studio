/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer - v27.0 FINAL WITH STATE MACHINE
 * @version 27.0.0
 *
 * ✅ PROPER BUTTON STATE MACHINE (short/long/double click)
 * ✅ WORKING: 16 kHz sample rate
 * ✅ MUSICAL SCALES: Full key/scale system
 * ✅ 9 FUNCTIONS from 3 buttons!
 *
 * BUTTON CONTROLS:
 * S1 (J4.33):
 *   - Short click:  Change scale type
 *   - Long press:   Change instrument
 *   - Double click: Toggle effects
 * 
 * S2 (J4.32):
 *   - Short click:  Play/Stop
 *   - Long press:   Chord mode
 *   - Double click: Arpeggiator
 * 
 * JOY_SEL:
 *   - Short click:  Change preset
 *   - Long press:   Reset all
 *   - Double click: (Reserved)
 *
 * @date 2025-12-28
 */

#include "main.h"
#include "lcd_driver.h"
#include "ti_msp_dl_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//=============================================================================
// CONFIG
//=============================================================================
#define AUDIO_GAIN_BOOST 16
#define PORTAMENTO_SPEED 25

#define SYSTICK_RATE_HZ 100
#define MCLK_FREQ_HZ 80000000UL
#define SYSTICK_LOAD_VALUE ((MCLK_FREQ_HZ / SYSTICK_RATE_HZ) - 1)

#define ACCEL_Y_NEUTRAL 2849
#define ACCEL_Y_THRESHOLD 300
#define ACCEL_X_NEUTRAL 2048

#define ENABLE_CHORD_MODE 1
#define ENABLE_ARPEGGIATOR 1
#define ENABLE_WAVEFORM_DISPLAY 1
#define ENABLE_DEBUG_LEDS 1

//=============================================================================
// BUTTON STATE MACHINE
//=============================================================================
typedef enum {
    KEY_IDLE_STATE = 0,
    KEY_FIRST_CLICK_STATE,
    KEY_WAIT_DOUBLE_STATE,
    KEY_DOUBLE_CLICK_STATE
} KeyState_t;

typedef enum {
    KEY_NO_CLICK = 0,
    KEY_SHORT_CLICK_VALUE,
    KEY_LONG_CLICK_VALUE,
    KEY_DOUBLE_CLICK_VALUE
} KeyValue_t;

typedef struct {
    KeyState_t state;
    KeyValue_t value;
    uint16_t cnt;
    uint8_t raw_state;
    uint8_t last_raw_state;
} ButtonStateMachine_t;

#define KEY_SHORT_TIME   20
#define KEY_LONG_TIME    50
#define KEY_DOUBLE_TIME  50

static volatile ButtonStateMachine_t btn_s1_sm = {0};
static volatile ButtonStateMachine_t btn_s2_sm = {0};
static volatile ButtonStateMachine_t btn_joy_sel_sm = {0};

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
    {0,2,4,5,7,9,11,12}, {0,2,3,5,7,8,10,12}, {0,2,4,7,9,12,12,12},
    {0,3,5,7,10,12,12,12}, {0,3,5,6,7,10,12,12}, {0,2,3,5,7,9,10,12}
};

static const uint16_t ROOT_FREQUENCIES[KEY_COUNT] = {262,294,330,349,392,440,494};
static const char* KEY_NAMES[KEY_COUNT] = {"C","D","E","F","G","A","B"};
static const char* SCALE_NAMES[SCALE_COUNT] = {"MAJ","MIN","PNT+","PNT-","BLUE","DOR"};

typedef struct {
    MusicalKey_t current_key;
    ScaleType_t current_scale;
    uint8_t scale_position;
    uint16_t current_note_freq;
} ScaleState_t;

static ScaleState_t scale_state = {KEY_C, SCALE_MAJOR, 3, 262};

//=============================================================================
// DMA
//=============================================================================
#define ADC0_BUFFER_SIZE 2
static volatile uint16_t gADC0_DMA_Buffer[ADC0_BUFFER_SIZE] __attribute__((aligned(4)));
static volatile bool gADC0_DMA_Complete = false;

//=============================================================================
// SYNTH TYPES
//=============================================================================
typedef enum { ARP_OFF = 0, ARP_UP, ARP_DOWN, ARP_UP_DOWN, ARP_RANDOM, ARP_MODE_COUNT } ArpMode_t;
typedef struct { ArpMode_t mode; uint8_t current_step; uint32_t step_counter; uint32_t steps_per_note; int8_t pattern[8]; } Arpeggiator_t;
typedef enum { CHORD_OFF = 0, CHORD_MAJOR, CHORD_MINOR, CHORD_MODE_COUNT } ChordMode_t;
static const int8_t CHORD_INTERVALS[CHORD_MODE_COUNT][3] = {{0,0,0},{0,4,7},{0,3,7}};
typedef enum { INSTRUMENT_PIANO = 0, INSTRUMENT_ORGAN, INSTRUMENT_STRINGS, INSTRUMENT_BASS, INSTRUMENT_LEAD, INSTRUMENT_COUNT } Instrument_t;
typedef struct { uint16_t attack_samples, decay_samples, sustain_level, release_samples; } ADSR_Profile_t;
typedef struct { const char *name; ADSR_Profile_t adsr; Waveform_t waveform; uint8_t num_harmonics, vibrato_depth, tremolo_depth; uint16_t color; } InstrumentProfile_t;

static const InstrumentProfile_t INSTRUMENTS[INSTRUMENT_COUNT] = {
    {"PIANO",{80,1600,700,800},WAVE_TRIANGLE,1,0,0,LCD_COLOR_CYAN},
    {"ORGAN",{0,0,1000,400},WAVE_SINE,1,20,0,LCD_COLOR_RED},
    {"STRINGS",{2400,3200,800,4000},WAVE_SAWTOOTH,1,15,10,LCD_COLOR_YELLOW},
    {"BASS",{160,800,900,800},WAVE_SINE,0,0,0,LCD_COLOR_BLUE},
    {"LEAD",{40,1200,850,1600},WAVE_SQUARE,1,30,5,LCD_COLOR_GREEN}
};

typedef enum { ENV_IDLE = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE } EnvelopeState_t;
typedef struct { EnvelopeState_t state; uint32_t phase; uint16_t amplitude; bool note_on; } Envelope_t;
typedef struct { const char *name; Instrument_t instrument; bool effects_enabled; ChordMode_t chord_mode; ArpMode_t arp_mode; } Preset_t;

static const Preset_t PRESETS[3] = {
    {"CLASSIC",INSTRUMENT_PIANO,false,CHORD_OFF,ARP_OFF},
    {"AMBIENT",INSTRUMENT_STRINGS,true,CHORD_MAJOR,ARP_OFF},
    {"SEQUENCE",INSTRUMENT_LEAD,true,CHORD_MINOR,ARP_UP}
};

static const uint32_t PITCH_BEND_TABLE[25] = {
    32768,34675,36781,38967,41285,43742,46341,49091,51998,55041,58255,61644,
    65536,69433,73533,77841,82366,87111,92123,97549,103397,109681,116411,123596,131072
};

//=============================================================================
// GLOBAL STATE
//=============================================================================
volatile SynthState_t gSynthState;
volatile uint32_t g_phase = 0;
volatile uint32_t g_phase_increment = 118111601;
volatile uint32_t g_chord_phases[3] = {0};
volatile uint32_t g_chord_increments[3] = {118111601,118111601,118111601};

static Instrument_t current_instrument = INSTRUMENT_PIANO;
static uint8_t current_preset = 0;
static Envelope_t envelope = {0};
static bool effects_enabled = true;
static ChordMode_t chord_mode = CHORD_OFF;
static uint32_t base_frequency_hz = 440;
static int8_t current_octave_shift = 0;
static uint16_t vibrato_phase = 0, tremolo_phase = 0;
static Arpeggiator_t arpeggiator = {0};
static uint32_t target_frequency_hz = 440;
static uint32_t current_frequency_hz = 440;

volatile uint32_t g_systick_count = 0;
volatile uint32_t DEBUG_main_loop_count = 0;
volatile uint32_t DEBUG_timer_irq_count = 0;

#if ENABLE_WAVEFORM_DISPLAY
static int16_t waveform_buffer[64] = {0};
static uint8_t waveform_write_index = 0;
#endif

static const int16_t sine_table[256] = {
    0,25,49,74,98,122,147,171,195,219,243,267,290,314,337,360,383,405,428,450,471,493,514,535,
    555,575,595,614,633,652,670,687,704,721,737,753,768,783,797,811,824,837,849,860,871,882,892,901,
    910,918,926,933,939,945,951,955,960,963,966,969,971,972,973,974,974,973,972,971,969,966,963,960,
    955,951,945,939,933,926,918,910,901,892,882,871,860,849,837,824,811,797,783,768,753,737,721,704,
    687,670,652,633,614,595,575,555,535,514,493,471,450,428,405,383,360,337,314,290,267,243,219,195,
    171,147,122,98,74,49,25,0,-25,-49,-74,-98,-122,-147,-171,-195,-219,-243,-267,-290,-314,-337,-360,-383,
    -405,-428,-450,-471,-493,-514,-535,-555,-575,-595,-614,-633,-652,-670,-687,-704,-721,-737,-753,-768,-783,-797,-811,-824,
    -837,-849,-860,-871,-882,-892,-901,-910,-918,-926,-933,-939,-945,-951,-955,-960,-963,-966,-969,-971,-972,-973,-974,-974,
    -973,-972,-971,-969,-966,-963,-960,-955,-951,-945,-939,-933,-926,-918,-910,-901,-892,-882,-871,-860,-849,-837,-824,-811,
    -797,-783,-768,-753,-737,-721,-704,-687,-670,-652,-633,-614,-595,-575,-555,-535,-514,-493,-471,-450,-428,-405,-383,-360,
    -337,-314,-290,-267,-243,-219,-195,-171,-147,-122,-98,-74,-49,-25
};

//=============================================================================
// PROTOTYPES
//=============================================================================
static void SysTick_Init(void);
static void Button_StateMachine_Update(volatile ButtonStateMachine_t *btn, uint32_t gpio_pin);
static void Process_Accelerometer(void);
static void Process_Envelope(void);
static void Process_Arpeggiator(void);
static void Process_Portamento(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform);
static int16_t Generate_Chord_Sample(volatile uint32_t *phases, volatile uint32_t *increments);
static void Display_Update(void);
static void Display_Waveform(void);
static int16_t Low_Pass_Filter(int16_t new_sample);
static void Debug_LED_Update(int8_t octave);
static uint16_t Calculate_Scale_Frequency(MusicalKey_t key, ScaleType_t scale, uint8_t position, int8_t octave_shift);
static void Process_Key_Selection(void);
static void Process_Scale_Position(void);
static void Process_Musical_Controls(void);
static void Change_Scale_Type(void);
static void Display_Scale_Info(void);
void Change_Instrument(void);
void Change_Preset(void);
void Trigger_Note_On(void);
void Trigger_Note_Off(void);

//=============================================================================
// BUTTON STATE MACHINE UPDATE
//=============================================================================
static void Button_StateMachine_Update(volatile ButtonStateMachine_t *btn, uint32_t gpio_pin) {
    btn->raw_state = (DL_GPIO_readPins(GPIO_BUTTONS_PORT, gpio_pin) == 0) ? 0 : 1;
    uint8_t pressed = (btn->last_raw_state == 1) && (btn->raw_state == 0);
    uint8_t released = (btn->last_raw_state == 0) && (btn->raw_state == 1);
    btn->last_raw_state = btn->raw_state;
    
    switch (btn->state) {
    case KEY_IDLE_STATE:
        if (pressed) { btn->state = KEY_FIRST_CLICK_STATE; btn->cnt = 0; btn->value = KEY_NO_CLICK; }
        break;
    case KEY_FIRST_CLICK_STATE:
        btn->cnt++;
        if (released) {
            if (btn->cnt < KEY_SHORT_TIME) { btn->state = KEY_WAIT_DOUBLE_STATE; btn->cnt = 0; }
            else if (btn->cnt < KEY_LONG_TIME) { btn->state = KEY_IDLE_STATE; btn->value = KEY_SHORT_CLICK_VALUE; btn->cnt = 0; }
            else { btn->state = KEY_IDLE_STATE; btn->cnt = 0; }
        } else if (btn->cnt >= KEY_LONG_TIME) {
            btn->state = KEY_IDLE_STATE; btn->value = KEY_LONG_CLICK_VALUE; btn->cnt = 0;
        }
        break;
    case KEY_WAIT_DOUBLE_STATE:
        btn->cnt++;
        if (pressed) { btn->state = KEY_DOUBLE_CLICK_STATE; btn->cnt = 0; }
        else if (btn->cnt >= KEY_DOUBLE_TIME) { btn->state = KEY_IDLE_STATE; btn->value = KEY_SHORT_CLICK_VALUE; btn->cnt = 0; }
        break;
    case KEY_DOUBLE_CLICK_STATE:
        if (released) { btn->state = KEY_IDLE_STATE; btn->value = KEY_DOUBLE_CLICK_VALUE; btn->cnt = 0; }
        break;
    default:
        btn->state = KEY_IDLE_STATE; btn->cnt = 0; break;
    }
}

//=============================================================================
// MUSICAL SCALE FUNCTIONS
//=============================================================================
static uint16_t Calculate_Scale_Frequency(MusicalKey_t key, ScaleType_t scale, uint8_t position, int8_t octave_shift) {
    uint16_t root_freq = ROOT_FREQUENCIES[key];
    int8_t interval = SCALE_INTERVALS[scale][position];
    int8_t total_semitones = interval + octave_shift;
    const uint16_t semitone_ratio[25] = {
        1000,1059,1122,1189,1260,1335,1414,1498,1587,1682,1782,1888,
        2000,2119,2245,2378,2520,2670,2828,2997,3175,3364,3564,3775,4000
    };
    int8_t idx = total_semitones + 12;
    if (idx < 0) idx = 0; if (idx > 24) idx = 24;
    uint32_t freq = ((uint32_t)root_freq * semitone_ratio[idx]) / 1000;
    if (freq < 100) freq = 100; if (freq > 2000) freq = 2000;
    return (uint16_t)freq;
}

static void Process_Key_Selection(void) {
    uint16_t joy_x = gSynthState.joy_x;
    if (joy_x > 4095) joy_x = 4095;
    uint8_t new_key = (joy_x * KEY_COUNT) / 4096;
    if (new_key >= KEY_COUNT) new_key = KEY_COUNT - 1;
    if (scale_state.current_key != new_key) {
        scale_state.current_key = (MusicalKey_t)new_key;
        scale_state.current_note_freq = Calculate_Scale_Frequency(
            scale_state.current_key, scale_state.current_scale, scale_state.scale_position, current_octave_shift);
        target_frequency_hz = scale_state.current_note_freq;
    }
}

static void Process_Scale_Position(void) {
    int16_t accel_x = gSynthState.accel_x;
    int16_t deviation = accel_x - ACCEL_X_NEUTRAL;
    static uint8_t last_position = 3;
    static int16_t last_deviation = 0;
    int16_t deviation_change = deviation - last_deviation;
    if (deviation_change > -50 && deviation_change < 50 && scale_state.scale_position == last_position) return;
    last_deviation = deviation;
    uint8_t new_position;
    if (deviation < -600) new_position = 0;
    else if (deviation < -400) new_position = 1;
    else if (deviation < -200) new_position = 2;
    else if (deviation < 200) new_position = 3;
    else if (deviation < 400) new_position = 4;
    else if (deviation < 600) new_position = 5;
    else if (deviation < 800) new_position = 6;
    else new_position = 7;
    if (scale_state.scale_position != new_position) {
        scale_state.scale_position = new_position;
        last_position = new_position;
        scale_state.current_note_freq = Calculate_Scale_Frequency(
            scale_state.current_key, scale_state.current_scale, scale_state.scale_position, current_octave_shift);
        target_frequency_hz = scale_state.current_note_freq;
    }
}

static void Change_Scale_Type(void) {
    scale_state.current_scale = (ScaleType_t)((scale_state.current_scale + 1) % SCALE_COUNT);
    scale_state.current_note_freq = Calculate_Scale_Frequency(
        scale_state.current_key, scale_state.current_scale, scale_state.scale_position, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
}

static void Display_Scale_Info(void) {
    char buf[32];
    LCD_DrawRect(0, 28, 128, 10, LCD_COLOR_BLACK);
    snprintf(buf, sizeof(buf), "%s %s", KEY_NAMES[scale_state.current_key], SCALE_NAMES[scale_state.current_scale]);
    LCD_PrintString(3, 28, buf, LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
    snprintf(buf, sizeof(buf), "%d/8", scale_state.scale_position + 1);
    LCD_PrintString(85, 28, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
}

//=============================================================================
// MAIN
//=============================================================================
int main(void) {
  SYSCFG_DL_init();
  memset((void *)&gSynthState, 0, sizeof(SynthState_t));
  gSynthState.frequency = 440;
  gSynthState.volume = 80;
  gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
  gSynthState.audio_playing = 1;
  gSynthState.phase_increment = 118111601;
  
  base_frequency_hz = 440;
  target_frequency_hz = 440;
  current_frequency_hz = 440;
  current_octave_shift = 0;
  g_phase_increment = 118111601;
  g_chord_increments[0] = g_phase_increment;
  g_chord_increments[1] = g_phase_increment;
  g_chord_increments[2] = g_phase_increment;
  Update_Phase_Increment();
  
  arpeggiator.mode = ARP_OFF;
  arpeggiator.steps_per_note = (16000 * 60) / (120 * 4);
  envelope.state = ENV_ATTACK;
  envelope.phase = 0;
  envelope.amplitude = 0;
  envelope.note_on = true;
  
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
  LCD_PrintString(15, 50, "v27.0", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_LARGE);
  LCD_PrintString(5, 70, "STATE", LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_MEDIUM);
  DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
  DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
  
  SysTick_Init();
  __enable_irq();
  NVIC_ClearPendingIRQ(TIMG7_INT_IRQn);
  NVIC_SetPriority(TIMG7_INT_IRQn, 1);
  NVIC_EnableIRQ(TIMG7_INT_IRQn);
  DL_TimerG_startCounter(TIMER_SAMPLE_INST);
  
  DL_Common_delayCycles(8000);
  if (gSynthState.timer_count == 0) {
    LCD_PrintString(10, 90, "TIMER FAIL!", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
  } else {
    LCD_PrintString(10, 90, "TIMER OK!", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  }
  DL_Common_delayCycles(80000000);
  LCD_FillScreen(LCD_COLOR_BLACK);
  
  uint32_t loop_counter = 0, display_counter = 0;
  
  while (1) {
    DEBUG_main_loop_count++;
    
    if (gADC0_DMA_Complete) {
      gSynthState.joy_x = gADC0_DMA_Buffer[0];
      gSynthState.joy_y = gADC0_DMA_Buffer[1];
      gADC0_DMA_Complete = false;
      DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
    }
    
    // S1 BUTTON HANDLING
    if (btn_s1_sm.value != KEY_NO_CLICK) {
        KeyValue_t action = btn_s1_sm.value;
        btn_s1_sm.value = KEY_NO_CLICK;
        switch (action) {
        case KEY_SHORT_CLICK_VALUE:
            Change_Scale_Type();
            display_counter = 200000;
            break;
        case KEY_LONG_CLICK_VALUE:
            Change_Instrument();
            display_counter = 200000;
            break;
        case KEY_DOUBLE_CLICK_VALUE:
            effects_enabled = !effects_enabled;
            display_counter = 200000;
            break;
        default: break;
        }
    }
    
    // S2 BUTTON HANDLING
    if (btn_s2_sm.value != KEY_NO_CLICK) {
        KeyValue_t action = btn_s2_sm.value;
        btn_s2_sm.value = KEY_NO_CLICK;
        switch (action) {
        case KEY_SHORT_CLICK_VALUE:
            gSynthState.audio_playing = !gSynthState.audio_playing;
            if (gSynthState.audio_playing) Trigger_Note_On();
            else Trigger_Note_Off();
            display_counter = 200000;
            break;
        case KEY_LONG_CLICK_VALUE:
            chord_mode = (ChordMode_t)((chord_mode + 1) % CHORD_MODE_COUNT);
            display_counter = 200000;
            break;
        case KEY_DOUBLE_CLICK_VALUE:
            if (arpeggiator.mode == ARP_OFF) arpeggiator.mode = ARP_UP;
            else arpeggiator.mode = ARP_OFF;
            display_counter = 200000;
            break;
        default: break;
        }
    }
    
    // JOY_SEL BUTTON HANDLING
    if (btn_joy_sel_sm.value != KEY_NO_CLICK) {
        KeyValue_t action = btn_joy_sel_sm.value;
        btn_joy_sel_sm.value = KEY_NO_CLICK;
        switch (action) {
        case KEY_SHORT_CLICK_VALUE:
            Change_Preset();
            display_counter = 200000;
            break;
        case KEY_LONG_CLICK_VALUE:
            current_instrument = INSTRUMENT_PIANO;
            current_preset = 0;
            effects_enabled = true;
            chord_mode = CHORD_OFF;
            arpeggiator.mode = ARP_OFF;
            scale_state.current_key = KEY_C;
            scale_state.current_scale = SCALE_MAJOR;
            gSynthState.volume = 80;
            display_counter = 200000;
            break;
        default: break;
        }
    }
    
    if (loop_counter % 1000 == 0) {
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
// SYSTICK
//=============================================================================
static void SysTick_Init(void) {
  SysTick->LOAD = SYSTICK_LOAD_VALUE;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void) {
  g_systick_count++;
  Button_StateMachine_Update(&btn_s1_sm, GPIO_BUTTONS_S1_MKII_PIN);
  Button_StateMachine_Update(&btn_s2_sm, GPIO_BUTTONS_S2_MKII_PIN);
  Button_StateMachine_Update(&btn_joy_sel_sm, GPIO_BUTTONS_JOY_SEL_PIN);
}

//=============================================================================
// AUDIO TIMER INTERRUPT
//=============================================================================
void TIMG7_IRQHandler(void) {
  uint32_t status = DL_TimerG_getPendingInterrupt(TIMER_SAMPLE_INST);
  if (!(status & DL_TIMERG_IIDX_ZERO)) return;
  DEBUG_timer_irq_count++;
  gSynthState.timer_count++;
  if (g_phase_increment == 0) g_phase_increment = 118111601;
  Process_Envelope();
  Process_Arpeggiator();
  Process_Portamento();
  vibrato_phase += 82;
  tremolo_phase += 67;
  if (gSynthState.audio_playing) Generate_Audio_Sample();
  else DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
}

//=============================================================================
// DMA & ADC
//=============================================================================
void DMA_IRQHandler(void) {
  if (DL_DMA_getPendingInterrupt(DMA) == DL_DMA_EVENT_IIDX_DMACH1) gADC0_DMA_Complete = true;
}

void ADC0_IRQHandler(void) {
  gSynthState.adc0_count++;
  switch (DL_ADC12_getPendingInterrupt(ADC_JOY_INST)) {
  case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
  case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
    gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0); // CH2 = JOY_X
    gSynthState.mic_level = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_1); // CH5 = MIC
    gSynthState.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.joy_y = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_1);
    break;
  default: break;
  }
}

void ADC1_IRQHandler(void) {
  gSynthState.adc1_count++;
  if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) == DL_ADC12_IIDX_MEM3_RESULT_LOADED) {
    gSynthState.accel_x = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.accel_y = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
    gSynthState.accel_z = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
    gSynthState.mic_level = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_3);
  }
}


//=============================================================================
// FILTERS & PORTAMENTO
//=============================================================================
static int16_t Low_Pass_Filter(int16_t new_sample) {
  static int16_t prev_sample = 0;
  int16_t filtered = (prev_sample * 3 + new_sample) / 4;
  prev_sample = filtered;
  return filtered;
}

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

//=============================================================================
// MUSICAL CONTROLS
//=============================================================================
static void Process_Musical_Controls(void) {
    Process_Key_Selection();
    Process_Scale_Position();
    uint16_t joy_y = gSynthState.joy_y;
    if (joy_y > 4095) joy_y = 4095;
    uint8_t new_vol = (uint8_t)((joy_y * 100) / 4095);
    if (new_vol > 100) new_vol = 100;
    int16_t vol_diff = (int16_t)new_vol - (int16_t)gSynthState.volume;
    if (vol_diff > 3 || vol_diff < -3) gSynthState.volume = new_vol;
}

//=============================================================================
// AUDIO GENERATION
//=============================================================================
static void Generate_Audio_Sample(void) {
  if (g_phase_increment == 0) g_phase_increment = 118111601;
  if (gSynthState.volume == 0 || envelope.amplitude == 0) {
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
    if (effects_enabled && inst->vibrato_depth > 0) {
      uint8_t vib_index = vibrato_phase >> 8;
      int16_t vibrato_lfo = sine_table[vib_index];
      int32_t phase_offset = ((int32_t)vibrato_lfo * inst->vibrato_depth * (int32_t)g_phase_increment) / 100000;
      modulated_phase = g_phase + phase_offset;
    }
    uint8_t index = (uint8_t)((modulated_phase >> 24) & 0xFF);
    sample = Generate_Waveform(index, inst->waveform);
    if (inst->num_harmonics >= 1) {
      uint8_t h1_index = (index << 1) & 0xFF;
      int16_t harmonic1 = Generate_Waveform(h1_index, inst->waveform);
      sample = (sample * 2 + harmonic1) / 3;
    }
    if (effects_enabled && inst->tremolo_depth > 0) {
      uint8_t trem_index = tremolo_phase >> 8;
      int16_t tremolo_lfo = sine_table[trem_index];
      int16_t mod = 1000 + ((tremolo_lfo * inst->tremolo_depth) / 100);
      sample = (int16_t)(((int32_t)sample * mod) / 1000);
    }
    g_phase += g_phase_increment;
  }
  sample = (int16_t)(((int32_t)sample * envelope.amplitude) / 1000);
  sample = (int16_t)(((int32_t)sample * gSynthState.volume) / 100);
  sample *= AUDIO_GAIN_BOOST;
  sample = Low_Pass_Filter(sample);
  if (sample > 1800) { sample = 1800 + ((sample - 1800) / 4); if (sample > 2000) sample = 2000; }
  if (sample < -1800) { sample = -1800 + ((sample + 1800) / 4); if (sample < -2000) sample = -2000; }
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

//=============================================================================
// WAVEFORMS
//=============================================================================
static int16_t Generate_Waveform(uint8_t index, Waveform_t waveform) {
  switch (waveform) {
  case WAVE_SINE: return sine_table[index];
  case WAVE_SQUARE:
    if (index < 118) return 900;
    if (index < 138) return 900 - (int16_t)(((index - 118) * 1800) / 20);
    return -900;
  case WAVE_SAWTOOTH: return (int16_t)(((int32_t)index * 1800 / 256) - 900);
  case WAVE_TRIANGLE:
    if (index < 128) return (int16_t)(((int32_t)index * 1800 / 128) - 900);
    return (int16_t)(900 - ((int32_t)(index - 128) * 1800 / 128));
  default: return sine_table[index];
  }
}

static int16_t Generate_Chord_Sample(volatile uint32_t *phases, volatile uint32_t *increments) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  int32_t mixed = 0;
  uint8_t num_voices = (chord_mode == CHORD_OFF) ? 1 : 3;
  for (uint8_t v = 0; v < num_voices; v++) {
    uint8_t index = (uint8_t)((phases[v] >> 24) & 0xFF);
    int16_t sample = Generate_Waveform(index, inst->waveform);
    if (inst->num_harmonics >= 1) {
      uint8_t h_index = (index << 1) & 0xFF;
      sample = (sample * 2 + Generate_Waveform(h_index, inst->waveform)) / 3;
    }
    mixed += sample;
    phases[v] += increments[v];
  }
  return (int16_t)(mixed / num_voices);
}

//=============================================================================
// ACCELEROMETER
//=============================================================================
static void Process_Accelerometer(void) {
  int16_t accel_y = gSynthState.accel_y;
  int16_t deviation = accel_y - ACCEL_Y_NEUTRAL;
  int8_t new_octave = 0;
  if (deviation < -ACCEL_Y_THRESHOLD) {
    new_octave = -12;
#if ENABLE_DEBUG_LEDS
    Debug_LED_Update(-1);
#endif
  } else if (deviation > ACCEL_Y_THRESHOLD) {
    new_octave = 12;
#if ENABLE_DEBUG_LEDS
    Debug_LED_Update(1);
#endif
  } else {
    new_octave = 0;
#if ENABLE_DEBUG_LEDS
    Debug_LED_Update(0);
#endif
  }
  if (current_octave_shift != new_octave) {
    current_octave_shift = new_octave;
    scale_state.current_note_freq = Calculate_Scale_Frequency(
        scale_state.current_key, scale_state.current_scale, scale_state.scale_position, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
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
// ENVELOPE & ARPEGGIATOR
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

static void Process_Envelope(void) {
  const ADSR_Profile_t *adsr = &INSTRUMENTS[current_instrument].adsr;
  switch (envelope.state) {
  case ENV_IDLE: envelope.amplitude = 0; break;
  case ENV_ATTACK:
    if (adsr->attack_samples == 0) { envelope.amplitude = 1000; envelope.state = ENV_DECAY; envelope.phase = 0; }
    else {
      envelope.phase++;
      envelope.amplitude = (uint16_t)((envelope.phase * 1000) / adsr->attack_samples);
      if (envelope.amplitude >= 1000) { envelope.amplitude = 1000; envelope.state = ENV_DECAY; envelope.phase = 0; }
    }
    break;
  case ENV_DECAY:
    if (adsr->decay_samples == 0) { envelope.amplitude = adsr->sustain_level; envelope.state = ENV_SUSTAIN; }
    else {
      envelope.phase++;
      uint16_t range = 1000 - adsr->sustain_level;
      uint16_t decayed = (uint16_t)((envelope.phase * range) / adsr->decay_samples);
      if (decayed >= range) { envelope.amplitude = adsr->sustain_level; envelope.state = ENV_SUSTAIN; }
      else envelope.amplitude = 1000 - decayed;
    }
    break;
  case ENV_SUSTAIN:
    envelope.amplitude = adsr->sustain_level;
    if (!envelope.note_on) { envelope.state = ENV_RELEASE; envelope.phase = 0; }
    break;
  case ENV_RELEASE:
    if (adsr->release_samples == 0) { envelope.amplitude = 0; envelope.state = ENV_IDLE; }
    else {
      envelope.phase++;
      uint16_t start = adsr->sustain_level;
      uint16_t released = (uint16_t)((envelope.phase * start) / adsr->release_samples);
      if (released >= start) { envelope.amplitude = 0; envelope.state = ENV_IDLE; }
      else envelope.amplitude = start - released;
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

//=============================================================================
// UPDATE PHASE INCREMENT
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
  if (bent_freq > 0 && bent_freq <= 16000) {
    uint64_t temp = ((uint64_t)bent_freq << 32) / 16000ULL;
    if (temp > 0 && temp <= 0xFFFFFFFF) g_phase_increment = (uint32_t)temp;
    else g_phase_increment = 118111601;
  } else g_phase_increment = 118111601;
  if (g_phase_increment == 0) g_phase_increment = 118111601;
  gSynthState.phase_increment = g_phase_increment;
  gSynthState.frequency = (float)bent_freq;
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
      if (chord_freq > 0 && chord_freq <= 16000) {
        uint64_t chord_temp = ((uint64_t)chord_freq << 32) / 16000ULL;
        if (chord_temp > 0 && chord_temp <= 0xFFFFFFFF) g_chord_increments[voice] = (uint32_t)chord_temp;
        else g_chord_increments[voice] = g_phase_increment;
      } else g_chord_increments[voice] = g_phase_increment;
      if (g_chord_increments[voice] == 0) g_chord_increments[voice] = g_phase_increment;
    }
  } else {
    g_chord_increments[0] = g_phase_increment;
    g_chord_increments[1] = g_phase_increment;
    g_chord_increments[2] = g_phase_increment;
  }
}

//=============================================================================
// DISPLAY
//=============================================================================
static void Display_Update(void) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  LCD_DrawRect(0, 0, 128, 16, inst->color);
  LCD_PrintString(3, 4, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
  LCD_PrintString(60, 4, PRESETS[current_preset].name, LCD_COLOR_BLACK, inst->color, FONT_SMALL);
  LCD_DrawRect(0, 18, 128, 10, LCD_COLOR_BLACK);
  LCD_PrintString(3, 18, "F:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(18, 18, base_frequency_hz, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
  char buf[16];
  if (current_octave_shift == -12) snprintf(buf, 16, "LOW");
  else if (current_octave_shift == 12) snprintf(buf, 16, "HI");
  else snprintf(buf, 16, "MID");
  LCD_PrintString(55, 18, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
  Display_Scale_Info();
  LCD_DrawRect(3, 40, 60, 4, LCD_COLOR_DARKGRAY);
  uint8_t bar_w = gSynthState.volume;
  if (bar_w > 100) bar_w = 100;
  LCD_DrawRect(3, 40, (bar_w * 60) / 100, 4, LCD_COLOR_GREEN);
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
  LCD_PrintString(55, 50, env_names[envelope.state], LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(90, 50, envelope.amplitude / 10, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);
#if ENABLE_WAVEFORM_DISPLAY
  Display_Waveform();
#endif
  LCD_DrawRect(0, 118, 128, 10, LCD_COLOR_BLACK);
  if (gSynthState.audio_playing) LCD_PrintString(3, 118, "PLAYING", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  else LCD_PrintString(3, 118, "STOPPED", LCD_COLOR_RED, LCD_COLOR_BLACK, FONT_SMALL);
  snprintf(buf, 16, "V:%d", gSynthState.volume);
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

void HardFault_Handler(void) {
  while (1) {
    DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    for (volatile uint32_t i = 0; i < 100000; i++);
  }
}