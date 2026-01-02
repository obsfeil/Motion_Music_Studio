/**
 * @file midi_handler.h
 * @brief MIDI Protocol Implementation for MSPM0G
 * @version 1.0
 * 
 * Implements standard MIDI 1.0 protocol over UART
 * Baudrate: 31250 (MIDI standard) or higher for USB-MIDI
 */

#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// MIDI Protocol Constants
//=============================================================================
#define MIDI_BAUD_RATE_STANDARD 31250   // Standard MIDI
#define MIDI_BAUD_RATE_USB      921600  // USB-MIDI (faster)

// MIDI Status Bytes (Channel Voice Messages)
#define MIDI_NOTE_OFF           0x80
#define MIDI_NOTE_ON            0x90
#define MIDI_POLY_PRESSURE      0xA0
#define MIDI_CONTROL_CHANGE     0xB0
#define MIDI_PROGRAM_CHANGE     0xC0
#define MIDI_CHANNEL_PRESSURE   0xD0
#define MIDI_PITCH_BEND         0xE0

// MIDI System Messages
#define MIDI_SYSEX_START        0xF0
#define MIDI_SYSEX_END          0xF7
#define MIDI_CLOCK              0xF8
#define MIDI_START              0xFA
#define MIDI_CONTINUE           0xFB
#define MIDI_STOP               0xFC
#define MIDI_ACTIVE_SENSING     0xFE
#define MIDI_RESET              0xFF

// MIDI Control Change Numbers
#define MIDI_CC_MODULATION      0x01
#define MIDI_CC_VOLUME          0x07
#define MIDI_CC_PAN             0x0A
#define MIDI_CC_EXPRESSION      0x0B
#define MIDI_CC_SUSTAIN         0x40
#define MIDI_CC_FILTER_CUTOFF   0x4A
#define MIDI_CC_RESONANCE       0x47

//=============================================================================
// MIDI Message Structure
//=============================================================================
typedef struct {
    uint8_t status;      // Status byte (includes channel)
    uint8_t data1;       // First data byte
    uint8_t data2;       // Second data byte (if applicable)
    uint8_t channel;     // MIDI channel (0-15)
    uint8_t length;      // Message length (1-3 bytes)
} MIDI_Message_t;

//=============================================================================
// MIDI Note/Frequency Conversion
//=============================================================================
// MIDI Note 69 (A4) = 440 Hz (concert pitch)
// Formula: f = 440 * 2^((note - 69) / 12)
// Range: MIDI 0-127 = ~8.18 Hz to ~12543.85 Hz

/**
 * @brief Convert MIDI note number to frequency in Hz
 * @param note MIDI note number (0-127)
 * @return Frequency in Hz
 * 
 * Uses pre-calculated table for efficiency on embedded systems.
 * Formula: f = 440 * 2^((N - 69) / 12)
 */
static inline uint16_t MIDI_NoteToFreq(uint8_t note) {
    // Complete MIDI frequency table (0-127)
    // Each entry calculated using: f = 440 * 2^((N - 69) / 12)
    static const uint16_t midi_freq_table[128] = {
        // MIDI 0-11: C-1 to B-1
        8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15,
        // MIDI 12-23: C0 to B0
        16, 17, 18, 19, 21, 22, 23, 25, 26, 28, 29, 31,
        // MIDI 24-35: C1 to B1
        33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62,
        // MIDI 36-47: C2 to B2
        65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123,
        // MIDI 48-59: C3 to B3
        131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
        // MIDI 60-71: C4 to B4 (Middle C octave)
        262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
        // MIDI 72-83: C5 to B5
        523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
        // MIDI 84-95: C6 to B6
        1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
        // MIDI 96-107: C7 to B7
        2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
        // MIDI 108-119: C8 to B8
        4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
        // MIDI 120-127: C9 to G9
        8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544
    };
    
    // Clamp to valid MIDI range
    if (note > 127) note = 127;
    
    return midi_freq_table[note];
}

/**
 * @brief Convert frequency to closest MIDI note
 * @param freq Frequency in Hz
 * @return MIDI note number (0-127)
 * 
 * Uses binary search for efficiency.
 * Inverse formula: N = 69 + 12 * log2(f / 440)
 */
static inline uint8_t MIDI_FreqToNote(uint16_t freq) {
    // Handle edge cases
    if (freq < 8) return 0;      // Below MIDI 0
    if (freq > 12544) return 127; // Above MIDI 127
    
    // Binary search in frequency table
    uint8_t low = 0;
    uint8_t high = 127;
    
    while (low < high) {
        uint8_t mid = (low + high) / 2;
        uint16_t mid_freq = MIDI_NoteToFreq(mid);
        
        if (freq < mid_freq) {
            high = mid;
        } else if (freq > mid_freq) {
            low = mid + 1;
        } else {
            return mid; // Exact match
        }
    }
    
    // Check which note is closer
    if (low > 0) {
        uint16_t freq_low = MIDI_NoteToFreq(low);
        uint16_t freq_prev = MIDI_NoteToFreq(low - 1);
        
        if ((freq - freq_prev) < (freq_low - freq)) {
            return low - 1;
        }
    }
    
    return low;
}

/**
 * @brief Get MIDI note name with octave
 * @param note MIDI note number (0-127)
 * @param buffer Output buffer (min 5 bytes: "C#10\0")
 * 
 * Note names follow MIDI standard:
 * - MIDI 0 = C-1
 * - MIDI 12 = C0
 * - MIDI 60 = C4 (Middle C)
 * - MIDI 69 = A4 (440 Hz)
 * - MIDI 127 = G9
 */
static inline void MIDI_GetNoteName(uint8_t note, char* buffer) {
    const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", 
                                "F#", "G", "G#", "A", "A#", "B"};
    
    // MIDI note 0 is C-1, note 12 is C0, note 60 is C4
    int8_t octave = (note / 12) - 1;
    uint8_t note_idx = note % 12;
    
    // Handle negative octave (MIDI 0-11 is octave -1)
    if (octave < 0) {
        if (note_names[note_idx][1] == '#') {
            buffer[0] = note_names[note_idx][0];
            buffer[1] = '#';
            buffer[2] = '-';
            buffer[3] = '1';
            buffer[4] = '\0';
        } else {
            buffer[0] = note_names[note_idx][0];
            buffer[1] = '-';
            buffer[2] = '1';
            buffer[3] = '\0';
        }
    } else if (octave >= 10) {
        // Handle two-digit octave (shouldn't happen with MIDI 0-127)
        if (note_names[note_idx][1] == '#') {
            buffer[0] = note_names[note_idx][0];
            buffer[1] = '#';
            buffer[2] = '1';
            buffer[3] = '0';
            buffer[4] = '\0';
        } else {
            buffer[0] = note_names[note_idx][0];
            buffer[1] = '1';
            buffer[2] = '0';
            buffer[3] = '\0';
        }
    } else {
        // Normal case (octave 0-9)
        if (note_names[note_idx][1] == '#') {
            buffer[0] = note_names[note_idx][0];
            buffer[1] = '#';
            buffer[2] = '0' + octave;
            buffer[3] = '\0';
        } else {
            buffer[0] = note_names[note_idx][0];
            buffer[1] = '0' + octave;
            buffer[2] = '\0';
        }
    }
}

/**
 * @brief Get detailed MIDI note information
 * @param note MIDI note number (0-127)
 * @param name_buf Buffer for note name (min 5 bytes)
 * @param freq Pointer to store frequency (can be NULL)
 * 
 * Example usage:
 *   char name[5];
 *   uint16_t freq;
 *   MIDI_GetNoteInfo(69, name, &freq);
 *   // name = "A4", freq = 440
 */
static inline void MIDI_GetNoteInfo(uint8_t note, char* name_buf, uint16_t* freq) {
    if (name_buf) {
        MIDI_GetNoteName(note, name_buf);
    }
    if (freq) {
        *freq = MIDI_NoteToFreq(note);
    }
}

/**
 * @brief Create a MIDI Note On message
 * @param channel MIDI channel (0-15)
 * @param note Note number (0-127)
 * @param velocity Velocity (0-127)
 * @param msg Output message structure
 */
static inline void MIDI_CreateNoteOn(uint8_t channel, uint8_t note, 
                                     uint8_t velocity, MIDI_Message_t* msg) {
    msg->status = MIDI_NOTE_ON | (channel & 0x0F);
    msg->data1 = note & 0x7F;
    msg->data2 = velocity & 0x7F;
    msg->channel = channel & 0x0F;
    msg->length = 3;
}

/**
 * @brief Create a MIDI Note Off message
 * @param channel MIDI channel (0-15)
 * @param note Note number (0-127)
 * @param velocity Release velocity (0-127)
 * @param msg Output message structure
 */
static inline void MIDI_CreateNoteOff(uint8_t channel, uint8_t note,
                                      uint8_t velocity, MIDI_Message_t* msg) {
    msg->status = MIDI_NOTE_OFF | (channel & 0x0F);
    msg->data1 = note & 0x7F;
    msg->data2 = velocity & 0x7F;
    msg->channel = channel & 0x0F;
    msg->length = 3;
}

/**
 * @brief Create a MIDI Control Change message
 * @param channel MIDI channel (0-15)
 * @param controller Controller number (0-127)
 * @param value Controller value (0-127)
 * @param msg Output message structure
 */
static inline void MIDI_CreateControlChange(uint8_t channel, uint8_t controller,
                                            uint8_t value, MIDI_Message_t* msg) {
    msg->status = MIDI_CONTROL_CHANGE | (channel & 0x0F);
    msg->data1 = controller & 0x7F;
    msg->data2 = value & 0x7F;
    msg->channel = channel & 0x0F;
    msg->length = 3;
}

/**
 * @brief Create a MIDI Program Change message
 * @param channel MIDI channel (0-15)
 * @param program Program number (0-127)
 * @param msg Output message structure
 */
static inline void MIDI_CreateProgramChange(uint8_t channel, uint8_t program,
                                            MIDI_Message_t* msg) {
    msg->status = MIDI_PROGRAM_CHANGE | (channel & 0x0F);
    msg->data1 = program & 0x7F;
    msg->data2 = 0;
    msg->channel = channel & 0x0F;
    msg->length = 2;
}

/**
 * @brief Create a MIDI Pitch Bend message
 * @param channel MIDI channel (0-15)
 * @param bend Bend value (0-16383, 8192 = center)
 * @param msg Output message structure
 */
static inline void MIDI_CreatePitchBend(uint8_t channel, uint16_t bend,
                                        MIDI_Message_t* msg) {
    msg->status = MIDI_PITCH_BEND | (channel & 0x0F);
    msg->data1 = bend & 0x7F;        // LSB
    msg->data2 = (bend >> 7) & 0x7F; // MSB
    msg->channel = channel & 0x0F;
    msg->length = 3;
}

#endif // MIDI_HANDLER_H