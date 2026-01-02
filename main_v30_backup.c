/**
 * @file main.c
 * @brief MSPM0G3507 Synthesizer - v29.3 GREENSLEEVES
 * @version 29.3
 *
 * ✅ NEW: Greensleeves Mode - Authentic 16th century English melody
 * ✅ NEW: 12-position harmonic progression system
 * ✅ ENHANCED: Extended harmony (I-vii°, V7, inversions, 7th chords)
 *
 * BUTTON CONTROLS:
 * S1: Short=Instrument, Long=Major/Minor, Double=Effects
 * S2: Short=Play/Stop, Long=Chord, Double=Arpeggiator
 * JOY_SEL: Short=GREENSLEEVES (🍀 Traditional Melody), Long=Reset
 * JOY_X: Select key (C-B) with deadzone hold
 * JOY_Y: Volume (0-100%) with deadzone hold
 * ACCEL_X: Harmonic progression (24 positions: vii↓ to I↑↑↑ - smooth MIDI-standard semitone control)
 * ACCEL_Y: Octave shift (tilt forward/back)
 *
 * GREENSLEEVES MODE:
 * - Authentic traditional melody from 16th century England
 * - Public domain - no copyright restrictions
 * - 16-step sequence in A minor
 * - Classic progression: Am-C-G-Am-E-Am
 * - ~2 seconds per chord
 * - STRINGS instrument (lute/fiddle-like)
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
// MIDI PROTOCOL IMPLEMENTATION
//=============================================================================
// MIDI Note 69 (A4) = 440 Hz (concert pitch)
// Formula: f = 440 * 2^((N - 69) / 12)

// MIDI Status Bytes
#define MIDI_NOTE_OFF           0x80
#define MIDI_NOTE_ON            0x90
#define MIDI_CONTROL_CHANGE     0xB0
#define MIDI_PROGRAM_CHANGE     0xC0
#define MIDI_PITCH_BEND         0xE0

// MIDI Control Change Numbers
#define MIDI_CC_VOLUME          0x07

// MIDI Message Structure
typedef struct {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint8_t length;
} MIDI_Message_t;

// MIDI Frequency Table (complete 0-127)
static const uint16_t MIDI_FREQ_TABLE[128] = {
    8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15,
    16, 17, 18, 19, 21, 22, 23, 25, 26, 28, 29, 31,
    33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62,
    65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123,
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
    4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
    8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544
};

static inline uint16_t MIDI_NoteToFreq(uint8_t note) {
    if (note > 127) note = 127;
    return MIDI_FREQ_TABLE[note];
}

static inline uint8_t MIDI_FreqToNote(uint16_t freq) {
    if (freq < 8) return 0;
    if (freq > 12544) return 127;
    
    // Binary search
    uint8_t low = 0, high = 127;
    while (low < high) {
        uint8_t mid = (low + high) / 2;
        if (freq < MIDI_FREQ_TABLE[mid]) {
            high = mid;
        } else if (freq > MIDI_FREQ_TABLE[mid]) {
            low = mid + 1;
        } else {
            return mid;
        }
    }
    
    if (low > 0) {
        uint16_t diff_low = (freq >= MIDI_FREQ_TABLE[low]) ? 
            (freq - MIDI_FREQ_TABLE[low]) : (MIDI_FREQ_TABLE[low] - freq);
        uint16_t diff_prev = (freq >= MIDI_FREQ_TABLE[low-1]) ? 
            (freq - MIDI_FREQ_TABLE[low-1]) : (MIDI_FREQ_TABLE[low-1] - freq);
        if (diff_prev < diff_low) return low - 1;
    }
    return low;
}

static inline void MIDI_CreateNoteOn(uint8_t channel, uint8_t note, 
                                     uint8_t velocity, MIDI_Message_t* msg) {
    msg->status = MIDI_NOTE_ON | (channel & 0x0F);
    msg->data1 = note & 0x7F;
    msg->data2 = velocity & 0x7F;
    msg->length = 3;
}

static inline void MIDI_CreateNoteOff(uint8_t channel, uint8_t note,
                                      uint8_t velocity, MIDI_Message_t* msg) {
    msg->status = MIDI_NOTE_OFF | (channel & 0x0F);
    msg->data1 = note & 0x7F;
    msg->data2 = velocity & 0x7F;
    msg->length = 3;
}

static inline void MIDI_CreateControlChange(uint8_t channel, uint8_t controller,
                                            uint8_t value, MIDI_Message_t* msg) {
    msg->status = MIDI_CONTROL_CHANGE | (channel & 0x0F);
    msg->data1 = controller & 0x7F;
    msg->data2 = value & 0x7F;
    msg->length = 3;
}

static inline void MIDI_CreateProgramChange(uint8_t channel, uint8_t program,
                                            MIDI_Message_t* msg) {
    msg->status = MIDI_PROGRAM_CHANGE | (channel & 0x0F);
    msg->data1 = program & 0x7F;
    msg->data2 = 0;
    msg->length = 2;
}


//=============================================================================
// CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ 16000
#define SYSTICK_RATE_HZ 100
#define MCLK_FREQ_HZ 80000000UL
#define SYSTICK_LOAD_VALUE ((MCLK_FREQ_HZ / SYSTICK_RATE_HZ) - 1)
#define PORTAMENTO_SPEED 25
#define AUDIO_GAIN_BOOST 8
#define FREQ_MIN_HZ 20
#define FREQ_MAX_HZ 8000

#define ACCEL_Y_NEUTRAL 2849
#define ACCEL_Y_THRESHOLD 300

#define PWM_MAX_VALUE 2047 // 11-bit oppløsning (fjerner pipelyd)
#define PWM_CENTER_VALUE 1023

#define ENABLE_CHORD_MODE 1
#define ENABLE_ARPEGGIATOR 1
#define ENABLE_WAVEFORM_DISPLAY 1
#define ENABLE_DEBUG_LEDS 2

//=============================================================================
// MUSICAL SCALES
//=============================================================================
typedef enum {
  SCALE_MAJOR = 0,
  SCALE_MINOR,
  SCALE_PENTATONIC_MAJOR,
  SCALE_PENTATONIC_MINOR,
  SCALE_BLUES,
  SCALE_DORIAN,
  SCALE_COUNT
} ScaleType_t;

typedef enum {
  KEY_C = 0,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_A,
  KEY_B,
  KEY_COUNT
} MusicalKey_t;

static const int8_t SCALE_INTERVALS[SCALE_COUNT][8] = {
    {0, 2, 4, 5, 7, 9, 11, 12},  {0, 2, 3, 5, 7, 8, 10, 12},
    {0, 2, 4, 7, 9, 12, 12, 12}, {0, 3, 5, 7, 10, 12, 12, 12},
    {0, 3, 5, 6, 7, 10, 12, 12}, {0, 2, 3, 5, 7, 9, 10, 12}};

static const uint16_t ROOT_FREQUENCIES[KEY_COUNT] = {262, 294, 330, 349,
                                                     392, 440, 494};
static const char *KEY_NAMES[KEY_COUNT] = {"C", "D", "E", "F", "G", "A", "B"};
static const char *SCALE_NAMES[SCALE_COUNT] = {"MAJ",  "MIN",  "PNT+",
                                               "PNT-", "BLUE", "DOR"};

typedef struct {
  MusicalKey_t current_key;
  ScaleType_t current_scale;
  uint8_t scale_position;
  uint16_t current_note_freq;
} ScaleState_t;

//=============================================================================
// HARMONIC PROGRESSION SYSTEM
//=============================================================================
typedef enum {
  MODE_MAJOR = 0,
  MODE_MINOR = 1,
  MODE_COUNT = 2
} MusicalMode_t;

// Harmonic function: Extended harmony with 24 positions for smooth ACCEL_X control
// Follows MIDI standard with semitone-based progressions
// Organized for natural musical flow with octave variants
typedef enum {
  // Low register (-7 to -1 semitones from root)
  HARM_vii_low = 0,   // -1 semitone (leading tone below)
  HARM_vi_low,        // -2 semitones
  HARM_V_low,         // -5 semitones (dominant below)
  HARM_IV_low,        // -7 semitones (subdominant below)
  
  // Mid-low register (base diatonic chords)
  HARM_I = 4,         // 0 semitones - Root/Tonic ⭐
  HARM_ii,            // +2 semitones - Supertonic
  HARM_iii,           // +4 semitones - Mediant
  HARM_IV,            // +5 semitones - Subdominant
  HARM_V,             // +7 semitones - Dominant
  HARM_vi,            // +9 semitones - Submediant
  HARM_vii,           // +11 semitones - Leading tone
  
  // Mid-high register (extended chords)
  HARM_I_oct,         // +12 semitones - Tonic octave up
  HARM_V7,            // +7 semitones - Dominant 7th
  HARM_ii7,           // +2 semitones - Supertonic 7th
  HARM_vi7,           // +9 semitones - Submediant 7th
  HARM_IVmaj7,        // +5 semitones - Subdominant maj7
  
  // High register (+12 to +19 semitones from root)
  HARM_I_inv,         // +16 semitones - Tonic first inversion high
  HARM_ii_high,       // +14 semitones
  HARM_IV_high,       // +17 semitones
  HARM_V_high,        // +19 semitones
  
  // Very high register (+24 semitones = 2 octaves)
  HARM_I_2oct,        // +24 semitones - Tonic 2 octaves up
  HARM_V_2oct,        // +31 semitones - Dominant 2 octaves up
  HARM_IV_2oct,       // +29 semitones - Subdominant 2 octaves up
  HARM_I_3oct,        // +36 semitones - Tonic 3 octaves up
  
  HARM_COUNT
} HarmonicFunction_t;

// Chord intervals from root (semitones) - MAJOR MODE
// Root = MIDI note 60 (C4), all intervals calculated from there
static const int8_t HARMONIC_INTERVALS_MAJOR[HARM_COUNT][4] = {
    // Low register
    {-1, 2, 6, -1},      // vii_low - B dim below (B-D-F)
    {-2, 1, 5, -1},      // vi_low  - A minor below (A-C-E)
    {-5, -1, 2, -1},     // V_low   - G major below (G-B-D)
    {-7, -3, 0, -1},     // IV_low  - F major below (F-A-C)
    
    // Mid-low register (base chords)
    {0, 4, 7, -1},       // I       - C major (C-E-G)
    {2, 5, 9, -1},       // ii      - D minor (D-F-A)
    {4, 7, 11, -1},      // iii     - E minor (E-G-B)
    {5, 9, 12, -1},      // IV      - F major (F-A-C)
    {7, 11, 14, -1},     // V       - G major (G-B-D)
    {9, 12, 16, -1},     // vi      - A minor (A-C-E)
    {11, 14, 17, -1},    // vii     - B dim (B-D-F)
    
    // Mid-high register (extended)
    {12, 16, 19, -1},    // I_oct   - C major octave up
    {7, 11, 14, 17},     // V7      - G7 (G-B-D-F)
    {2, 5, 9, 12},       // ii7     - Dm7 (D-F-A-C)
    {9, 12, 16, 19},     // vi7     - Am7 (A-C-E-G)
    {5, 9, 12, 16},      // IVmaj7  - Fmaj7 (F-A-C-E)
    
    // High register
    {16, 19, 24, -1},    // I_inv   - C/E high (E-G-C)
    {14, 17, 21, -1},    // ii_high - D minor high
    {17, 21, 24, -1},    // IV_high - F major high
    {19, 23, 26, -1},    // V_high  - G major high
    
    // Very high register
    {24, 28, 31, -1},    // I_2oct  - C major 2 octaves up
    {31, 35, 38, -1},    // V_2oct  - G major 2 octaves up
    {29, 33, 36, -1},    // IV_2oct - F major 2 octaves up
    {36, 40, 43, -1}     // I_3oct  - C major 3 octaves up
};

// Chord intervals from root (semitones) - MINOR MODE
static const int8_t HARMONIC_INTERVALS_MINOR[HARM_COUNT][4] = {
    // Low register
    {-1, 2, 6, -1},      // vii_low - B dim below
    {-4, -1, 3, -1},     // VI_low  - Ab major below
    {-5, -1, 2, -1},     // V_low   - G major below
    {-7, -4, 0, -1},     // iv_low  - F minor below
    
    // Mid-low register
    {0, 3, 7, -1},       // i       - C minor (C-Eb-G)
    {2, 5, 8, -1},       // ii°     - D dim (D-F-Ab)
    {3, 7, 10, -1},      // III     - Eb major (Eb-G-Bb)
    {5, 8, 12, -1},      // iv      - F minor (F-Ab-C)
    {7, 11, 14, -1},     // V       - G major (G-B-D) - harmonic minor!
    {8, 12, 15, -1},     // VI      - Ab major (Ab-C-Eb)
    {11, 14, 17, -1},    // vii°    - B dim (B-D-F)
    
    // Mid-high register
    {12, 15, 19, -1},    // i_oct   - C minor octave up
    {7, 11, 14, 17},     // V7      - G7 (G-B-D-F)
    {2, 5, 8, 12},       // ii°7    - Dm7b5 (D-F-Ab-C)
    {8, 12, 15, 19},     // VI7     - Abmaj7 (Ab-C-Eb-G)
    {5, 8, 12, 15},      // iv7     - Fm7 (F-Ab-C-Eb)
    
    // High register
    {15, 19, 24, -1},    // i_inv   - Cm/Eb high
    {14, 17, 20, -1},    // ii_high - D dim high
    {17, 20, 24, -1},    // iv_high - F minor high
    {19, 23, 26, -1},    // V_high  - G major high
    
    // Very high register
    {24, 27, 31, -1},    // i_2oct  - C minor 2 octaves up
    {31, 35, 38, -1},    // V_2oct  - G major 2 octaves up
    {29, 32, 36, -1},    // iv_2oct - F minor 2 octaves up
    {36, 39, 43, -1}     // i_3oct  - C minor 3 octaves up
};

static const char* HARMONIC_NAMES_MAJOR[HARM_COUNT] = {
    "vii↓", "vi↓", "V↓", "IV↓",           // Low register
    "I", "ii", "iii", "IV", "V", "vi", "vii",  // Mid-low
    "I↑", "V7", "ii7", "vi7", "IVM7",     // Mid-high
    "I/E", "ii↑", "IV↑", "V↑",            // High
    "I↑↑", "V↑↑", "IV↑↑", "I↑↑↑"         // Very high
};

static const char* HARMONIC_NAMES_MINOR[HARM_COUNT] = {
    "vii↓", "VI↓", "V↓", "iv↓",           // Low register
    "i", "ii°", "III", "iv", "V", "VI", "vii°",  // Mid-low
    "i↑", "V7", "ii7", "VI7", "iv7",      // Mid-high
    "i/E", "ii↑", "iv↑", "V↑",            // High
    "i↑↑", "V↑↑", "iv↑↑", "i↑↑↑"         // Very high
};

//=============================================================================
// CHORD & ARPEGGIATOR
//=============================================================================
typedef enum {
  CHORD_OFF = 0,
  CHORD_MAJOR,
  CHORD_MINOR,
  CHORD_MODE_COUNT
} ChordMode_t;
static const int8_t CHORD_INTERVALS[CHORD_MODE_COUNT][3] = {
    {0, 0, 0}, {0, 4, 7}, {0, 3, 7}};

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
} Arpeggiator_t;

//=============================================================================
// INSTRUMENTS
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
  const char *name;
  ADSR_Profile_t adsr; // Kommer fra audio_envelope.h (via main.h)
  Waveform_t waveform; // Kommer fra audio_engine.h (via main.h)
  uint8_t num_harmonics;
  uint8_t vibrato_depth;
  uint8_t tremolo_depth;
  uint16_t color;
} InstrumentProfile_t;

static const InstrumentProfile_t INSTRUMENTS[INSTRUMENT_COUNT] = {
    // PIANO: Quick attack, moderate decay, bright
    {"PIANO", {40, 1200, 650, 600}, WAVE_TRIANGLE, 2, 0, 0, LCD_COLOR_CYAN},
    
    // ORGAN: Instant attack, sustained, rich harmonics
    {"ORGAN", {0, 0, 1000, 200}, WAVE_SINE, 3, 25, 0, LCD_COLOR_RED},
    
    // STRINGS: Very slow attack, long sustain, warm vibrato
    {"STRINGS", {3200, 4000, 900, 5000}, WAVE_SAWTOOTH, 1, 20, 15, LCD_COLOR_YELLOW},
    
    // BASS: Fast attack, punchy, deep and resonant
    {"BASS", {80, 400, 950, 600}, WAVE_SINE, 0, 0, 0, LCD_COLOR_BLUE},
    
    // LEAD: Sharp attack, bright square wave, aggressive vibrato
    {"LEAD", {20, 800, 900, 1200}, WAVE_SQUARE, 2, 40, 8, LCD_COLOR_GREEN}
};

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
static volatile uint16_t gADC0_DMA_Buffer[ADC0_BUFFER_SIZE]
    __attribute__((aligned(4)));
static volatile bool gADC0_DMA_Complete = false;

//=============================================================================
// PITCH BEND TABLE (from v27)
//=============================================================================
static const uint32_t PITCH_BEND_TABLE[25] = {
    32768, 34675, 36781,  38967,  41285,  43742,  46341, 49091, 51998,
    55041, 58255, 61644,  65536,  69433,  73533,  77841, 82366, 87111,
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
static MusicalMode_t current_mode = MODE_MAJOR;
static HarmonicFunction_t current_harmony = HARM_I;
static Instrument_t current_instrument = INSTRUMENT_PIANO;
static uint8_t current_preset = 0;
static bool effects_enabled = true;
static ChordMode_t chord_mode = CHORD_OFF;
static Arpeggiator_t arpeggiator = {0};

// Epic Organ Mode (inspired by 70s progressive rock)
static bool epic_mode_active = false;
static uint8_t epic_sequence_step = 0;
static uint32_t epic_step_counter = 0;
static const uint32_t EPIC_STEPS_PER_NOTE = 32000; // ~2 seconds per note at 16kHz

// MIDI State
static uint8_t midi_last_note = 0;
static uint16_t midi_last_frequency = 0;
static bool midi_note_is_on = false;
static uint8_t midi_last_volume = 0;
static uint8_t midi_last_instrument = 0xFF;

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
static void Process_Epic_Mode(void);
static void Toggle_Epic_Mode(void);
static void Process_Portamento(void);
static void Generate_Audio_Sample(void);
static void Update_Phase_Increment(void);
static int16_t Generate_Chord_Sample(volatile uint32_t *phases,
                                     volatile uint32_t *increments);
static void Display_Update(void);
static void Display_Waveform(void);
static void Display_Scale_Info(void);
static uint16_t Calculate_Scale_Frequency(MusicalKey_t key, ScaleType_t scale,
                                          uint8_t position,
                                          int8_t octave_shift);
static uint16_t Calculate_Harmonic_Frequency(MusicalKey_t key, MusicalMode_t mode,
                                             HarmonicFunction_t harmony, int8_t octave_shift);
void Change_Instrument(void);
void Change_Preset(void);
void Change_Scale_Type(void);
void Trigger_Note_On(void);
void Trigger_Note_Off(void);

// DAC12 Helper Functions (defined later)
static inline uint16_t Audio_SampleToDAC12(int16_t sample);
static inline void Audio_WriteDAC12(int16_t sample);
static inline void Audio_MuteDAC12(void);

#if ENABLE_DEBUG_LEDS
static void Debug_LED_Update(int8_t octave);
#endif
//=============================================================================
// MAIN
//=============================================================================
int main(void) {
  SYSCFG_DL_init();

  DL_MathACL_enablePower(MATHACL);
  DL_MathACL_enableSaturation(MATHACL);  // Prevent overflow  

  // Initialize and calibrate DAC12
  DL_DAC12_enable(DAC0);
  delay_cycles(1000);  // Let DAC12 settle
  Audio_MuteDAC12();   // Set to midpoint

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
  Joystick_Init(&joystick, 100); // 100 = deadzone
  Accel_Init(&accel, 100);       // 100 = deadzone

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
  LCD_PrintString(10, 50, "v28.2.1", LCD_COLOR_GREEN, LCD_COLOR_BLACK,
                  FONT_LARGE);
  LCD_PrintString(5, 70, "FIXED!", LCD_COLOR_CYAN, LCD_COLOR_BLACK,
                  FONT_MEDIUM);
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
    LCD_PrintString(10, 90, "TIMER FAIL!", LCD_COLOR_RED, LCD_COLOR_BLACK,
                    FONT_SMALL);
  } else {
    LCD_PrintString(10, 90, "READY!", LCD_COLOR_GREEN, LCD_COLOR_BLACK,
                    FONT_SMALL);
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
      //      DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
    }

    // Buttons (Disse må sjekkes ofte for å fange korte trykk)
    // NB: Selve oppdateringen skjer nå i SysTick, så vi bare henter events her

    // S1 Button
    ButtonEvent_t s1_event = Button_GetEvent(&btn_s1);
    if (s1_event == BTN_EVENT_SHORT_CLICK) {
      Change_Instrument();
      display_counter = 200000;
    } else if (s1_event == BTN_EVENT_LONG_PRESS) {
      // Toggle between Major and Minor mode
      current_mode = (MusicalMode_t)((current_mode + 1) % MODE_COUNT);

      // Update frequency for new mode
      scale_state.current_note_freq = Calculate_Harmonic_Frequency(
          scale_state.current_key, current_mode, current_harmony, current_octave_shift);
      target_frequency_hz = scale_state.current_note_freq;
      Update_Phase_Increment();

      display_counter = 200000;
    } else if (s1_event == BTN_EVENT_DOUBLE_CLICK) {
      effects_enabled = !effects_enabled;
      display_counter = 200000;
    }

    // S2 Button
    ButtonEvent_t s2_event = Button_GetEvent(&btn_s2);
    if (s2_event == BTN_EVENT_SHORT_CLICK) {
      gSynthState.audio_playing = !gSynthState.audio_playing;
      if (gSynthState.audio_playing) {
        Trigger_Note_On();
      } else {
        Trigger_Note_Off();
        // Flush UART to prevent hanging MIDI notes
        while(DL_UART_isTXFIFOEmpty(UART_AUDIO_INST) == false);
      }
      display_counter = 200000;
    } else if (s2_event == BTN_EVENT_LONG_PRESS) {
      chord_mode = (ChordMode_t)((chord_mode + 1) % CHORD_MODE_COUNT);
      display_counter = 200000;
    } else if (s2_event == BTN_EVENT_DOUBLE_CLICK) {
      if (arpeggiator.mode == ARP_OFF)
        arpeggiator.mode = ARP_UP;
      else
        arpeggiator.mode = ARP_OFF;
      display_counter = 200000;
    }

    // JOY_SEL Button
    ButtonEvent_t joy_sel_event = Button_GetEvent(&btn_joy_sel);
    if (joy_sel_event == BTN_EVENT_SHORT_CLICK) {
      // Force blue/green LED immediately to confirm button press
      DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
      DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
      
      Toggle_Epic_Mode();
      display_counter = 200000;
    } else if (joy_sel_event == BTN_EVENT_LONG_PRESS) {
      // Reset logic...
      epic_mode_active = false;
      current_instrument = INSTRUMENT_PIANO;
      current_preset = 0;
      effects_enabled = true;
      chord_mode = CHORD_OFF;
      arpeggiator.mode = ARP_OFF;
      scale_state.current_key = KEY_C;
      scale_state.current_scale = SCALE_MAJOR;
      
      // Mute DAC12 on reset
      Audio_MuteDAC12();
      
      display_counter = 200000;
    }

    // ====================================================================
    // ✅ FIX: Oppdater Joystick/Accel SAMTIDIG som vi sjekker logikken
    // ====================================================================
    if (loop_counter % 1000 == 0) {

      // 1. Oppdater bibliotekene FØRST
      Joystick_Update(&joystick, gSynthState.joy_x, gSynthState.joy_y);
      Accel_Update(&accel, gSynthState.accel_x, gSynthState.accel_y,
                   gSynthState.accel_z);

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
  // Skip manual controls if epic mode is running
  if (epic_mode_active) return;
  
  // 1. Key selection (JOY_X) - Now selects musical key
  if (joystick.x_changed) {
    if (joystick.raw_x < 1000) {
      // Left - previous key
      if (scale_state.current_key > 0)
        scale_state.current_key--;
      else
        scale_state.current_key = (MusicalKey_t)(KEY_COUNT - 1);
    } else if (joystick.raw_x > 3000) {
      // Right - next key
      if (scale_state.current_key < (KEY_COUNT - 1))
        scale_state.current_key++;
      else
        scale_state.current_key = (MusicalKey_t)0;
    }

    // Update frequency based on current harmony
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
  }

  // 2. Volume (JOY_Y) - Unchanged
  if (joystick.y_changed) {
    gSynthState.volume = Joystick_GetVolume(&joystick);
  }

  // 3. Harmonic progression (ACCEL_X) - 24 positions for smooth control!
  if (accel.x_changed) {
    // Map accelerometer X (0-4095) to harmonic functions (0-23)
    // This gives smooth semitone-based progressions across full tilt range
    uint8_t harm_pos = (uint8_t)((accel.x * HARM_COUNT) / 4096);
    if (harm_pos >= HARM_COUNT) harm_pos = HARM_COUNT - 1;

    current_harmony = (HarmonicFunction_t)harm_pos;

    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
  }
}

//==================================================================
// ACCELROMETER
//===================================================================
static void Process_Accelerometer(void) {
  // Skip manual controls if epic mode is running
  if (epic_mode_active) return;
  
  int16_t ay = accel.y;
  int16_t deviation = ay - ACCEL_Y_NEUTRAL;

  // Vi definerer terskler for hver oktav (ca. 400 enheter mellom hver)
  const int16_t LIMIT_1 = 500;  // Første oktav
  const int16_t LIMIT_2 = 1000; // Andre oktav

  int8_t new_octave_shift = 0;

  // Sjekk tilt-soner for flere oktaver
  if (deviation > LIMIT_2) {
    new_octave_shift = 24; // Mye tilt forover -> To oktaver opp
  } else if (deviation > LIMIT_1) {
    new_octave_shift = 12; // Litt tilt forover -> En oktav opp
  } else if (deviation < -LIMIT_2) {
    new_octave_shift = -24; // Mye tilt bakover -> To oktaver ned
  } else if (deviation < -LIMIT_1) {
    new_octave_shift = -12; // Litt tilt bakover -> En oktav ned
  } else {
    new_octave_shift = 0; // Flatt brett -> Normal
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
    if (current_octave_shift > 0)
      Debug_LED_Update(1); // Grønn for opp
    else if (current_octave_shift < 0)
      Debug_LED_Update(-1); // Blå for ned
    else
      Debug_LED_Update(0); // Av for midten
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
// MATHACL SINE WAVE GENERATION
//=============================================================================
/**
 * @brief Generate sine sample using MATHACL hardware
 * @param phase Phase accumulator (0 to UINT32_MAX = 0 to 2π)
 * @return Sample value (-1024 to +1024 for 11-bit PWM)
 */
static inline int16_t MATHACL_Sine(uint32_t phase) {
    // Convert phase to Q31 angle for MATHACL
    // Phase 0x00000000 = 0°, 0x80000000 = 180°, 0xFFFFFFFF = 360°
    int32_t angle = (int32_t)(phase >> 1);  // Scale to Q31
    
    // Configure MATHACL for SINCOS operation
    DL_MathACL_operationConfig config = {
        .opType = DL_MATHACL_OP_TYPE_SINCOS,
        .qType = DL_MATHACL_Q_TYPE_Q31,
        .opSign = DL_MATHACL_OPSIGN_SIGNED
    };
    
    // Execute hardware sine
    DL_MathACL_configOperation(MATHACL, &config, angle, 0);
    DL_MathACL_waitForOperation(MATHACL);
    
    // Get result (Q31: -2^31 to +2^31 represents -1.0 to +1.0)
    int32_t result = DL_MathACL_getResultOne(MATHACL);
    
    // Scale to 11-bit PWM range (-1024 to +1024)
    return (int16_t)(result >> 21);
}
//=============================================================================
// HELPER FUNCTIONS
//=============================================================================
static uint16_t Calculate_Scale_Frequency(MusicalKey_t key, ScaleType_t scale,
                                          uint8_t position,
                                          int8_t octave_shift) {
  uint16_t root_freq = ROOT_FREQUENCIES[key];
  int8_t interval = SCALE_INTERVALS[scale][position];
  int8_t total_semitones = interval + octave_shift;

  const uint16_t semitone_ratio[25] = {1000, 1059, 1122, 1189, 1260, 1335, 1414,
                                       1498, 1587, 1682, 1782, 1888, 2000, 2119,
                                       2245, 2378, 2520, 2670, 2828, 2997, 3175,
                                       3364, 3564, 3775, 4000};

  int8_t idx = total_semitones + 12;
  if (idx < 0)
    idx = 0;
  if (idx > 24)
    idx = 24;

  uint32_t freq = ((uint32_t)root_freq * semitone_ratio[idx]) / 1000;
  if (freq < 100)
    freq = 100;
  if (freq > 2000)
    freq = 2000;

  return (uint16_t)freq;
}

static uint16_t Calculate_Harmonic_Frequency(MusicalKey_t key, MusicalMode_t mode,
                                             HarmonicFunction_t harmony, int8_t octave_shift) {
  uint16_t root_freq = ROOT_FREQUENCIES[key];

  // Get chord intervals based on mode
  const int8_t* intervals = (mode == MODE_MAJOR) ?
      HARMONIC_INTERVALS_MAJOR[harmony] :
      HARMONIC_INTERVALS_MINOR[harmony];

  // Use root note of the chord
  int8_t semitone = intervals[0] + octave_shift;

  // Use same calculation as original
  const uint16_t semitone_ratio[25] = {
      1000, 1059, 1122, 1189, 1260, 1335, 1414, 1498, 1587, 1682, 1782, 1888, 2000,
      2119, 2245, 2378, 2520, 2670, 2828, 2997, 3175, 3364, 3564, 3775, 4000};

  int8_t idx = semitone + 12;
  if (idx < 0) idx = 0;
  if (idx > 24) idx = 24;

  uint32_t freq = ((uint32_t)root_freq * semitone_ratio[idx]) / 1000;
  if (freq < 100) freq = 100;
  if (freq > 2000) freq = 2000;

  return (uint16_t)freq;
}

void Change_Scale_Type(void) {
  scale_state.current_scale =
      (ScaleType_t)((scale_state.current_scale + 1) % SCALE_COUNT);
  scale_state.current_note_freq = Calculate_Scale_Frequency(
      scale_state.current_key, scale_state.current_scale,
      scale_state.scale_position, current_octave_shift);
  target_frequency_hz = scale_state.current_note_freq;
}

void Change_Instrument(void) {
  current_instrument =
      (Instrument_t)((current_instrument + 1) % INSTRUMENT_COUNT);
  gSynthState.waveform = INSTRUMENTS[current_instrument].waveform;
  Envelope_Init(&envelope,
                &INSTRUMENTS[current_instrument].adsr); // Library API
  
  // Send MIDI Program Change
  if (current_instrument != midi_last_instrument) {
    midi_last_instrument = current_instrument;
    MIDI_Message_t msg;
    MIDI_CreateProgramChange(0, current_instrument, &msg);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.status);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data1);
  }
  
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
  Envelope_Init(&envelope,
                &INSTRUMENTS[current_instrument].adsr); // Library API
  Trigger_Note_On();
}

void Trigger_Note_On(void) {
  Envelope_NoteOn(&envelope); // Library API
}

void Trigger_Note_Off(void) {
  Envelope_NoteOff(&envelope); // Library API
  // Immediately mute DAC12
  Audio_MuteDAC12();
}

//=============================================================================
// ARPEGGIATOR
//=============================================================================
static void Process_Arpeggiator(void) {
  if (arpeggiator.mode == ARP_OFF)
    return;

  arpeggiator.step_counter++;
  if (arpeggiator.step_counter >= arpeggiator.steps_per_note) {
    arpeggiator.step_counter = 0;
    Trigger_Note_On();
    arpeggiator.current_step = (arpeggiator.current_step + 1) % 8;
  }
}

//=============================================================================
// CELTIC MODE - Greensleeves (Traditional, Public Domain)
//=============================================================================
// Based on the iconic English/Irish traditional melody from 16th century
// Chord progression: Am - C - G - Am - E - Am - C - G - Am - E - Am
// This is authentic traditional music - no copyright restrictions!
static const struct {
  MusicalKey_t key;
  HarmonicFunction_t harmony;
  MusicalMode_t mode;
  int8_t octave_shift;
} EPIC_SEQUENCE[] = {
    // Verse 1: "Alas my love, you do me wrong..."
    {KEY_A, HARM_I, MODE_MINOR, 0},      // Am (home)
    {KEY_A, HARM_iii, MODE_MINOR, 0},    // C major (relative major)
    {KEY_A, HARM_V, MODE_MINOR, 0},      // G major (bVII)
    {KEY_A, HARM_I, MODE_MINOR, 0},      // Am (return)
    {KEY_A, HARM_V, MODE_MINOR, 0},      // E major (dominant - harmonic minor!)
    {KEY_A, HARM_I, MODE_MINOR, 0},      // Am (resolve)
    
    // Verse 2: "To cast me off discourteously..."
    {KEY_A, HARM_iii, MODE_MINOR, 0},    // C major
    {KEY_A, HARM_V, MODE_MINOR, 0},      // G major (bVII)
    {KEY_A, HARM_I, MODE_MINOR, 0},      // Am
    {KEY_A, HARM_V, MODE_MINOR, 0},      // E major (dominant)
    {KEY_A, HARM_I, MODE_MINOR, 0},      // Am (resolve)
    
    // Chorus: "Greensleeves was all my joy..."
    {KEY_A, HARM_iii, MODE_MINOR, 5},    // C major (up a 4th - lift!)
    {KEY_A, HARM_V, MODE_MINOR, 5},      // G major (stay up)
    {KEY_A, HARM_I, MODE_MINOR, 5},      // Am (high point)
    {KEY_A, HARM_V, MODE_MINOR, 0},      // E major (back down)
    {KEY_A, HARM_I, MODE_MINOR, 0}       // Am (final home)
};

#define EPIC_SEQUENCE_LENGTH (sizeof(EPIC_SEQUENCE) / sizeof(EPIC_SEQUENCE[0]))

static void Process_Epic_Mode(void) {
  if (!epic_mode_active) return;
  
  epic_step_counter++;
  
  if (epic_step_counter >= EPIC_STEPS_PER_NOTE) {
    epic_step_counter = 0;
    epic_sequence_step = (epic_sequence_step + 1) % EPIC_SEQUENCE_LENGTH;
    
    // Visual feedback: Toggle between blue and green at each step
    DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN | GPIO_RGB_GREEN_PIN);
    
    // Update harmony from sequence
    scale_state.current_key = EPIC_SEQUENCE[epic_sequence_step].key;
    current_harmony = EPIC_SEQUENCE[epic_sequence_step].harmony;
    current_mode = EPIC_SEQUENCE[epic_sequence_step].mode;
    current_octave_shift = EPIC_SEQUENCE[epic_sequence_step].octave_shift;
    
    // Calculate new frequency
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
    
    // Trigger note on for each change
    Trigger_Note_On();
  }
}

static void Toggle_Epic_Mode(void) {
  epic_mode_active = !epic_mode_active;
  
  if (epic_mode_active) {
    // Visual feedback: BLUE LED on (Celtic mode)
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    
    // Show activation message
    LCD_FillScreen(LCD_COLOR_BLACK);
    LCD_PrintString(10, 50, "GREENSLEEVES", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_LARGE);
    LCD_PrintString(35, 70, "MODE!", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_MEDIUM);
    DL_Common_delayCycles(40000000); // Brief pause to see message
    
    // Enable epic mode with strings sound (Celtic fiddle-like)
    current_instrument = INSTRUMENT_STRINGS;
    effects_enabled = true;
    chord_mode = CHORD_OFF; // Single notes for clarity
    arpeggiator.mode = ARP_OFF;
    
    // Start from beginning
    epic_sequence_step = 0;
    epic_step_counter = 0;
    
    // Set initial state
    scale_state.current_key = EPIC_SEQUENCE[0].key;
    current_harmony = EPIC_SEQUENCE[0].harmony;
    current_mode = EPIC_SEQUENCE[0].mode;
    current_octave_shift = EPIC_SEQUENCE[0].octave_shift;
    
    // Update envelope for strings
    gSynthState.waveform = INSTRUMENTS[INSTRUMENT_STRINGS].waveform;
    Envelope_Init(&envelope, &INSTRUMENTS[INSTRUMENT_STRINGS].adsr);
    
    // Calculate first note
    scale_state.current_note_freq = Calculate_Harmonic_Frequency(
        scale_state.current_key, current_mode, current_harmony, current_octave_shift);
    target_frequency_hz = scale_state.current_note_freq;
    Update_Phase_Increment();
    
    Trigger_Note_On();
  } else {
    // Return to normal operation
    // Visual feedback: GREEN LED on
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    current_octave_shift = 0;
  }
}

//=============================================================================
// SYSTICK
//=============================================================================
static void SysTick_Init(void) {
  SysTick->LOAD = SYSTICK_LOAD_VALUE;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk;
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

  if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) ==
      DL_ADC12_IIDX_MEM3_RESULT_LOADED) {
    gSynthState.accel_x =
        (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
    gSynthState.accel_y =
        (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
    gSynthState.accel_z =
        (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
    gSynthState.joy_y =
        DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_3);
  }
}

//=============================================================================
// AUDIO TIMER ISR
//=============================================================================
void TIMG7_IRQHandler(void) {
  uint32_t status = DL_TimerG_getPendingInterrupt(TIMER_SAMPLE_INST);
  if (!(status & DL_TIMERG_IIDX_ZERO))
    return;

  gSynthState.timer_count++;
  if (g_phase_increment == 0)
    g_phase_increment = 118111601;

  Envelope_Process(&envelope); // Library API
  Process_Arpeggiator();
  Process_Epic_Mode();
  Process_Portamento();

  vibrato_phase += 82;
  tremolo_phase += 67;

  if (gSynthState.audio_playing) {
    Generate_Audio_Sample();
  } else {
    // MUTE: Set DAC12 to midpoint
    Audio_MuteDAC12();
  }
}

//=============================================================================
// DAC12 AUDIO OUTPUT HELPERS
//=============================================================================

/**
 * @brief Convert audio sample to 12-bit DAC value
 * @param sample Audio sample (-2048 to +2047 for 12-bit range)
 * @return DAC value (0 to 4095)
 */
static inline uint16_t Audio_SampleToDAC12(int16_t sample) {
    // Convert signed sample to unsigned DAC value
    // Sample range: -2048 to +2047
    // DAC range:    0 to 4095
    int32_t dac_val = (int32_t)sample + 2048;
    
    // Clamp to 12-bit range
    if (dac_val < 0) dac_val = 0;
    if (dac_val > 4095) dac_val = 4095;
    
    return (uint16_t)dac_val;
}

/**
 * @brief Write audio sample to DAC12
 * @param sample Audio sample (-2048 to +2047)
 */
static inline void Audio_WriteDAC12(int16_t sample) {
    uint16_t dac_val = Audio_SampleToDAC12(sample);
    DL_DAC12_output12(DAC0, dac_val);
}

/**
 * @brief Mute DAC12 output (set to midpoint)
 */
static inline void Audio_MuteDAC12(void) {
    DL_DAC12_output12(DAC0, 2048);  // Midpoint = silence
}

//=============================================================================
// AUDIO GENERATION (Using Library API for waveforms)
//=============================================================================
static void Generate_Audio_Sample(void) {
  if (g_phase_increment == 0)
    g_phase_increment = 118111601;

  uint16_t amplitude = Envelope_GetAmplitude(&envelope); // Library API
  if (gSynthState.volume == 0 || amplitude == 0) {
    // MUTE: Set DAC12 to midpoint
    Audio_MuteDAC12();
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
      const int16_t *sine = Audio_GetSineTable(); // Library API
      int16_t vibrato_lfo = sine[vib_index];
      int32_t phase_offset = ((int32_t)vibrato_lfo * inst->vibrato_depth *
                              (int32_t)g_phase_increment) /
                             100000;
      modulated_phase = g_phase + phase_offset;
    }

    uint8_t index = (uint8_t)((modulated_phase >> 24) & 0xFF);
    
    // Use MATHACL hardware for sine, library for others
    if (inst->waveform == WAVE_SINE) {
        // Perfect hardware-accelerated sine!
        sample = MATHACL_Sine(modulated_phase);
    } else {
        // Use library for square/saw/triangle
        sample = Audio_GenerateWaveform(index, inst->waveform);
    }

    // Harmonics
    if (inst->num_harmonics >= 1) {
      uint8_t h1_index = (index << 1) & 0xFF;
      int16_t harmonic1 =
          Audio_GenerateWaveform(h1_index, inst->waveform); // Library API
      sample = (sample * 2 + harmonic1) / 3;
    }

    // Tremolo
    if (effects_enabled && inst->tremolo_depth > 0) {
      uint8_t trem_index = tremolo_phase >> 8;
      const int16_t *sine = Audio_GetSineTable(); // Library API
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
  sample = Filter_GainWithFreqCompensation(sample, AUDIO_GAIN_BOOST,
                                           base_frequency_hz);
  sample = Filter_LowPass(sample);
  sample = Filter_SoftClip(sample, 1600);

#if ENABLE_WAVEFORM_DISPLAY
  static uint8_t waveform_decimate_counter = 0;
  if (++waveform_decimate_counter >= 125) {
    waveform_decimate_counter = 0;
    waveform_buffer[waveform_write_index++] = sample;
    if (waveform_write_index >= 64)
      waveform_write_index = 0;
  }
#endif

  // ✅ CORRECT: Write to DAC12 using helper
  Audio_WriteDAC12(sample);

  // ============================================================================
  // MIDI OUTPUT - Send MIDI messages instead of raw audio
  // ============================================================================
  
  // Send MIDI Note On/Off on frequency changes
  if (base_frequency_hz != midi_last_frequency) {
    midi_last_frequency = base_frequency_hz;
    
    uint8_t midi_note = MIDI_FreqToNote((uint16_t)base_frequency_hz);
    
    // Note Off for previous note
    if (midi_note_is_on && midi_last_note != midi_note) {
      MIDI_Message_t msg;
      MIDI_CreateNoteOff(0, midi_last_note, 64, &msg);
      DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.status);
      DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data1);
      DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data2);
      midi_note_is_on = false;
    }
    
    // Note On for new note
    if (gSynthState.audio_playing && (!midi_note_is_on || midi_last_note != midi_note)) {
      MIDI_Message_t msg;
      uint8_t velocity = (gSynthState.volume * 127) / 100;
      if (velocity == 0) velocity = 1; // MIDI velocity 0 = Note Off
      MIDI_CreateNoteOn(0, midi_note, velocity, &msg);
      DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.status);
      DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data1);
      DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data2);
      midi_last_note = midi_note;
      midi_note_is_on = true;
    }
  }
  
  // Send MIDI Note Off when audio stops
  if (!gSynthState.audio_playing && midi_note_is_on) {
    MIDI_Message_t msg;
    MIDI_CreateNoteOff(0, midi_last_note, 64, &msg);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.status);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data1);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data2);
    
    // Send MIDI CC 123 (All Notes Off) as safety
    MIDI_CreateControlChange(0, 123, 0, &msg);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.status);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data1);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data2);
    
    midi_note_is_on = false;
  }
  
  // Send MIDI CC for volume changes
  if (gSynthState.volume != midi_last_volume) {
    midi_last_volume = gSynthState.volume;
    MIDI_Message_t msg;
    uint8_t midi_volume = (gSynthState.volume * 127) / 100;
    MIDI_CreateControlChange(0, MIDI_CC_VOLUME, midi_volume, &msg);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.status);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data1);
    DL_UART_transmitDataBlocking(UART_AUDIO_INST, msg.data2);
  }

  // ✅ CORRECT: Increment counter only ONCE
  gSynthState.audio_samples_generated++;
}
//==============================================================================
// CHORD GENERATION
//==============================================================================
static int16_t Generate_Chord_Sample(volatile uint32_t *phases,
                                     volatile uint32_t *increments) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  int32_t mixed = 0;
  uint8_t num_voices = (chord_mode == CHORD_OFF) ? 1 : 3;

  for (uint8_t v = 0; v < num_voices; v++) {
    uint8_t index = (uint8_t)((phases[v] >> 24) & 0xFF);
    int16_t sample = Audio_GenerateWaveform(index, inst->waveform); // Library API

    if (inst->num_harmonics >= 1) {
      uint8_t h_index = (index << 1) & 0xFF;
      int16_t harmonic =
          Audio_GenerateWaveform(h_index, inst->waveform); // Library API
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
  if (base_frequency_hz == 0)
    base_frequency_hz = 440;

  int8_t table_index = current_octave_shift + 12;
  if (table_index < 0)
    table_index = 0;
  if (table_index > 24)
    table_index = 24;

  uint32_t bend_ratio = PITCH_BEND_TABLE[table_index];
  uint64_t bent_freq_64 = ((uint64_t)base_frequency_hz * bend_ratio) >> 16;
  uint32_t bent_freq = (uint32_t)bent_freq_64;

  if (bent_freq < FREQ_MIN_HZ)
    bent_freq = FREQ_MIN_HZ;
  if (bent_freq > FREQ_MAX_HZ)
    bent_freq = FREQ_MAX_HZ;

  if (bent_freq > 0 && bent_freq <= 8000) {
    uint64_t temp = ((uint64_t)bent_freq << 32) / 16000ULL;
    if (temp > 0 && temp <= 0xFFFFFFFF)
      g_phase_increment = (uint32_t)temp;
    else
      g_phase_increment = 118111601;
  } else {
    g_phase_increment = 118111601;
  }

  if (g_phase_increment == 0)
    g_phase_increment = 118111601;

  gSynthState.phase_increment = g_phase_increment;
  gSynthState.frequency = (float)bent_freq;

  // Update chord increments
  if (chord_mode != CHORD_OFF) {
    const int8_t *intervals = CHORD_INTERVALS[chord_mode];
    for (uint8_t voice = 0; voice < 3; voice++) {
      int8_t chord_table_index = table_index + intervals[voice];
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
        uint64_t chord_temp = ((uint64_t)chord_freq << 32) / 16000ULL;
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

  // Show key and mode
  const char* mode_name = (current_mode == MODE_MAJOR) ? "MAJ" : "MIN";
  snprintf(buf, sizeof(buf), "%s %s", KEY_NAMES[scale_state.current_key], mode_name);
  LCD_PrintString(3, 28, buf, LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);

  // Show current harmonic function
  const char** harm_names = (current_mode == MODE_MAJOR) ?
      HARMONIC_NAMES_MAJOR : HARMONIC_NAMES_MINOR;
  snprintf(buf, sizeof(buf), "%s", harm_names[current_harmony]);
  LCD_PrintString(85, 28, buf, LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
}

static void Display_Update(void) {
  const InstrumentProfile_t *inst = &INSTRUMENTS[current_instrument];
  char buf[32];

  LCD_DrawRect(0, 0, 128, 16, inst->color);
  LCD_PrintString(3, 4, inst->name, LCD_COLOR_WHITE, inst->color, FONT_SMALL);
  
  // Show EPIC MODE if active
  if (epic_mode_active) {
    LCD_PrintString(50, 4, "EPIC", LCD_COLOR_RED, inst->color, FONT_SMALL);
    snprintf(buf, sizeof(buf), "%d/16", epic_sequence_step + 1);
    LCD_PrintString(85, 4, buf, LCD_COLOR_YELLOW, inst->color, FONT_SMALL);
  } else {
    LCD_PrintString(60, 4, PRESETS[current_preset].name, LCD_COLOR_BLACK,
                    inst->color, FONT_SMALL);
  }

  LCD_DrawRect(0, 18, 128, 10, LCD_COLOR_BLACK);
  LCD_PrintString(3, 18, "F:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(18, 18, base_frequency_hz, LCD_COLOR_WHITE, LCD_COLOR_BLACK,
                  FONT_SMALL);

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
  if (bar_w > 100)
    bar_w = 100;
  LCD_DrawRect(3, 40, (bar_w * 60) / 100, 4, LCD_COLOR_GREEN);

  snprintf(buf, sizeof(buf), "%d%%", gSynthState.volume);
  LCD_PrintString(3, 46, buf, LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);

  LCD_DrawRect(66, 40, 62, 10, LCD_COLOR_BLACK);
  LCD_PrintString(66, 40, "FX:", LCD_COLOR_YELLOW, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintString(84, 40, effects_enabled ? "ON" : "OFF",
                  effects_enabled ? LCD_COLOR_GREEN : LCD_COLOR_RED,
                  LCD_COLOR_BLACK, FONT_SMALL);

  if (chord_mode != CHORD_OFF) {
    const char *chord_names[] = {"", "MAJ", "MIN"};
    LCD_PrintString(105, 40, chord_names[chord_mode], LCD_COLOR_MAGENTA,
                    LCD_COLOR_BLACK, FONT_SMALL);
  }

  LCD_DrawRect(0, 50, 128, 10, LCD_COLOR_BLACK);
  if (arpeggiator.mode != ARP_OFF) {
    LCD_PrintString(3, 50, "ARP", LCD_COLOR_GREEN, LCD_COLOR_BLACK, FONT_SMALL);
  }

  const char *env_names[] = {"IDLE", "ATK", "DEC", "SUS", "REL"};
  LCD_PrintString(55, 50, env_names[Envelope_GetState(&envelope)],
                  LCD_COLOR_CYAN, LCD_COLOR_BLACK, FONT_SMALL);
  LCD_PrintNumber(90, 50, Envelope_GetAmplitude(&envelope) / 10,
                  LCD_COLOR_WHITE, LCD_COLOR_BLACK, FONT_SMALL);

#if ENABLE_WAVEFORM_DISPLAY
  Display_Waveform();
#endif

  LCD_DrawRect(0, 118, 128, 10, LCD_COLOR_BLACK);
  if (gSynthState.audio_playing) {
    LCD_PrintString(3, 118, "PLAYING", LCD_COLOR_GREEN, LCD_COLOR_BLACK,
                    FONT_SMALL);
  } else {
    LCD_PrintString(3, 118, "STOPPED", LCD_COLOR_RED, LCD_COLOR_BLACK,
                    FONT_SMALL);
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
    if (y1 < 60)
      y1 = 60;
    if (y1 > 110)
      y1 = 110;
    if (y2 < 60)
      y2 = 60;
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
