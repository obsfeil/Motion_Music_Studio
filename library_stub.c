/**
 * @file library_stub.c
 * @brief Force linker to include all library symbols
 * @version 1.0
 * 
 * This file tricks the linker into keeping all library symbols!
 * 
 * HOW IT WORKS:
 * - Linker sees references in this file FIRST
 * - Keeps all library symbols
 * - main.o can then find them!
 * 
 * This function is NEVER called - it just forces symbol inclusion.
 */

#include <stdint.h>

// Forward declarations of ALL library functions
// From edumkii_buttons.h
extern void Button_Init(void* btn);
extern void Button_Update(void* btn, void* gpio_port, uint32_t gpio_pin);
extern int Button_GetEvent(void* btn);
extern int Button_IsPressed(void* btn);

// From edumkii_joystick.h
extern void Joystick_Init(void* joy, uint16_t deadzone);
extern void Joystick_Update(void* joy, uint16_t raw_x, uint16_t raw_y);
extern int16_t Joystick_GetX(void* joy);
extern int16_t Joystick_GetY(void* joy);
extern uint8_t Joystick_GetKeyIndex(void* joy, uint8_t num_keys);
extern uint8_t Joystick_GetVolume(void* joy);
extern int Joystick_IsCentered(void* joy);

// From edumkii_accel.h
extern void Accel_Init(void* accel, uint16_t deadzone);
extern void Accel_Update(void* accel, int16_t raw_x, int16_t raw_y, int16_t raw_z);
extern int8_t Accel_GetTilt(void* accel);
extern uint8_t Accel_GetScalePosition(void* accel);
extern int16_t Accel_GetXDeviation(void* accel);
extern int16_t Accel_GetYDeviation(void* accel);
extern int Accel_IsFlat(void* accel);

// From audio_envelope.h
extern void Envelope_Init(void* env, const void* profile);
extern void Envelope_NoteOn(void* env);
extern void Envelope_NoteOff(void* env);
extern void Envelope_Process(void* env);
extern uint16_t Envelope_GetAmplitude(const void* env);
extern int Envelope_GetState(const void* env);

// From audio_engine.h
extern const int16_t* Audio_GetSineTable(void);
extern int16_t Audio_GenerateWaveform(uint8_t index, int waveform);
extern uint32_t Audio_CalculatePhaseIncrement(uint32_t freq_hz, uint32_t sample_rate_hz);
extern uint16_t Audio_SampleToPWM(int16_t sample, uint16_t pwm_center, uint16_t pwm_max);

// From audio_filters.h
extern void Filter_Reset(void);
extern int16_t Filter_LowPass(int16_t new_sample);
extern int16_t Filter_SoftClip(int16_t sample, int16_t threshold);
extern int16_t Filter_GainWithFreqCompensation(int16_t sample, uint8_t gain, uint32_t frequency_hz);

/**
 * @brief Force all library symbols to be included by linker
 * 
 * This function is NEVER executed! It's marked with __attribute__((used))
 * so the compiler doesn't optimize it away, and it references all library
 * functions so the linker keeps them.
 * 
 * The linker sees: "Oh, library_stub.c needs Button_Init, so I'll keep it!"
 * Then later: "Oh, main.c also needs Button_Init - good thing I kept it!"
 */
void __attribute__((used)) __force_library_symbols_inclusion(void) {
    // Just reference function addresses - doesn't actually call them!
    // The (void) cast tells compiler "I know I'm not using this"
    
    // Buttons
    (void)&Button_Init;
    (void)&Button_Update;
    (void)&Button_GetEvent;
    (void)&Button_IsPressed;
    
    // Joystick
    (void)&Joystick_Init;
    (void)&Joystick_Update;
    (void)&Joystick_GetX;
    (void)&Joystick_GetY;
    (void)&Joystick_GetKeyIndex;
    (void)&Joystick_GetVolume;
    (void)&Joystick_IsCentered;
    
    // Accelerometer
    (void)&Accel_Init;
    (void)&Accel_Update;
    (void)&Accel_GetTilt;
    (void)&Accel_GetScalePosition;
    (void)&Accel_GetXDeviation;
    (void)&Accel_GetYDeviation;
    (void)&Accel_IsFlat;
    
    // Envelope
    (void)&Envelope_Init;
    (void)&Envelope_NoteOn;
    (void)&Envelope_NoteOff;
    (void)&Envelope_Process;
    (void)&Envelope_GetAmplitude;
    (void)&Envelope_GetState;
    
    // Audio Engine
    (void)&Audio_GetSineTable;
    (void)&Audio_GenerateWaveform;
    (void)&Audio_CalculatePhaseIncrement;
    (void)&Audio_SampleToPWM;
    
    // Filters
    (void)&Filter_Reset;
    (void)&Filter_LowPass;
    (void)&Filter_SoftClip;
    (void)&Filter_GainWithFreqCompensation;
}