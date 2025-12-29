/**
 * @file example_main.c
 * @brief Complete Example Using EDUMKII Library
 * @version 1.0.0
 * 
 * This example demonstrates all features of the EDUMKII library:
 * - Button state machine
 * - Joystick with deadzone
 * - Accelerometer
 * - Audio synthesis
 * - ADSR envelope
 * - Filters
 * 
 * Hardware:
 * - BOOSTXL-EDUMKII BoosterPack
 * - LP-MSPM0G3507 LaunchPad
 * 
 * Features:
 * - S1: Change waveform
 * - S2: Play/Stop
 * - JOY_X: Select note (C-B)
 * - JOY_Y: Volume (0-100%)
 * - ACCEL_Y: Octave shift (tilt forward/back)
 */

#include "ti_msp_dl_config.h"
#include "lib/edumkii/edumkii.h"
#include "lib/audio/audio_engine.h"
#include "lib/audio/audio_envelope.h"
#include "lib/audio/audio_filters.h"
#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// CONFIGURATION
//=============================================================================
#define SAMPLE_RATE_HZ 8000
#define SYSTICK_RATE_HZ 100

//=============================================================================
// MUSICAL NOTES (C4 to B4)
//=============================================================================
static const uint16_t note_frequencies[7] = {
    262,  // C4
    294,  // D4
    330,  // E4
    349,  // F4
    392,  // G4
    440,  // A4
    494   // B4
};

//=============================================================================
// HARDWARE OBJECTS
//=============================================================================
static Button_t btn_s1, btn_s2, btn_joy_sel;
static Joystick_t joystick;
static Accelerometer_t accel;
static Envelope_t envelope;

//=============================================================================
// APPLICATION STATE
//=============================================================================
typedef struct {
    uint16_t joy_x;
    uint16_t joy_y;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
} SensorData_t;

static volatile SensorData_t sensors = {0};

static struct {
    uint8_t current_note;      // 0-6 (C-B)
    uint8_t volume;            // 0-100
    int8_t octave_shift;       // -1, 0, +1
    Waveform_t waveform;       // Current waveform
    bool playing;              // Audio on/off
} app_state = {
    .current_note = 0,
    .volume = 80,
    .octave_shift = 0,
    .waveform = WAVE_SINE,
    .playing = false
};

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
static void SysTick_Init(void);
static void ADC_Init(void);
static void Update_Audio_Frequency(void);

//=============================================================================
// MAIN
//=============================================================================
int main(void) {
    // Initialize hardware
    SYSCFG_DL_init();
    
    // Initialize EDUMKII library
    Button_Init(&btn_s1);
    Button_Init(&btn_s2);
    Button_Init(&btn_joy_sel);
    Joystick_Init(&joystick, 100);  // 100 = deadzone
    Accel_Init(&accel, 100);
    
    // Initialize audio
    Audio_Init(SAMPLE_RATE_HZ);
    Audio_SetWaveform(WAVE_SINE);
    Envelope_Init(&envelope, &ADSR_PIANO);
    
    // Initialize peripherals
    SysTick_Init();
    ADC_Init();
    
    // Start audio timer
    __enable_irq();
    NVIC_SetPriority(TIMG7_INT_IRQn, 1);
    NVIC_EnableIRQ(TIMG7_INT_IRQn);
    DL_TimerG_startCounter(TIMER_SAMPLE_INST);
    
    // Main loop
    while (1) {
        //=====================================================================
        // UPDATE INPUTS
        //=====================================================================
        Joystick_Update(&joystick, sensors.joy_x, sensors.joy_y);
        Accel_Update(&accel, sensors.accel_x, sensors.accel_y, sensors.accel_z);
        
        //=====================================================================
        // HANDLE S1 BUTTON (Change Waveform)
        //=====================================================================
        ButtonEvent_t s1_event = Button_GetEvent(&btn_s1);
        if (s1_event == BTN_EVENT_SHORT_CLICK) {
            // Cycle through waveforms
            app_state.waveform = (Waveform_t)((app_state.waveform + 1) % WAVE_COUNT);
            Audio_SetWaveform(app_state.waveform);
            
            // Retrigger note
            if (app_state.playing) {
                Envelope_NoteOn(&envelope);
            }
        }
        
        //=====================================================================
        // HANDLE S2 BUTTON (Play/Stop)
        //=====================================================================
        ButtonEvent_t s2_event = Button_GetEvent(&btn_s2);
        if (s2_event == BTN_EVENT_SHORT_CLICK) {
            app_state.playing = !app_state.playing;
            
            if (app_state.playing) {
                Envelope_NoteOn(&envelope);
            } else {
                Envelope_NoteOff(&envelope);
            }
        }
        
        //=====================================================================
        // HANDLE JOYSTICK X (Note Selection)
        //=====================================================================
        if (joystick.x_changed) {
            app_state.current_note = Joystick_GetKeyIndex(&joystick, 7);
            Update_Audio_Frequency();
            
            // Retrigger note if playing
            if (app_state.playing) {
                Envelope_NoteOn(&envelope);
            }
        }
        
        //=====================================================================
        // HANDLE JOYSTICK Y (Volume)
        //=====================================================================
        if (joystick.y_changed) {
            uint8_t new_vol = Joystick_GetVolume(&joystick);
            if (new_vol != 255) {  // 255 = no change (in deadzone)
                app_state.volume = new_vol;
            }
        }
        
        //=====================================================================
        // HANDLE ACCELEROMETER Y (Octave Shift)
        //=====================================================================
        if (accel.y_changed) {
            int8_t new_tilt = Accel_GetTilt(&accel);
            if (new_tilt != app_state.octave_shift) {
                app_state.octave_shift = new_tilt;
                Update_Audio_Frequency();
            }
        }
        
        //=====================================================================
        // IDLE PROCESSING
        //=====================================================================
        // You can add display updates, LED indicators, etc. here
    }
}

//=============================================================================
// UPDATE AUDIO FREQUENCY
//=============================================================================
static void Update_Audio_Frequency(void) {
    uint16_t base_freq = note_frequencies[app_state.current_note];
    
    // Apply octave shift
    if (app_state.octave_shift == -1) {
        base_freq /= 2;  // One octave down
    } else if (app_state.octave_shift == +1) {
        base_freq *= 2;  // One octave up
    }
    
    Audio_SetFrequency(base_freq);
}

//=============================================================================
// SYSTICK INITIALIZATION (100 Hz)
//=============================================================================
static void SysTick_Init(void) {
    uint32_t load_value = (80000000UL / SYSTICK_RATE_HZ) - 1;
    SysTick->LOAD = load_value;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

//=============================================================================
// SYSTICK HANDLER (Update Buttons @ 100 Hz)
//=============================================================================
void SysTick_Handler(void) {
    Button_Update(&btn_s1, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_MKII_PIN);
    Button_Update(&btn_s2, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_MKII_PIN);
    Button_Update(&btn_joy_sel, GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
}

//=============================================================================
// ADC INITIALIZATION
//=============================================================================
static void ADC_Init(void) {
    NVIC_EnableIRQ(ADC0_INT_IRQn);
    NVIC_EnableIRQ(ADC1_INT_IRQn);
    DL_ADC12_enableConversions(ADC_JOY_INST);
    DL_ADC12_startConversion(ADC_JOY_INST);
    DL_ADC12_enableConversions(ADC_ACCEL_INST);
    DL_ADC12_startConversion(ADC_ACCEL_INST);
}

//=============================================================================
// ADC0 INTERRUPT (Joystick X)
//=============================================================================
void ADC0_IRQHandler(void) {
    switch (DL_ADC12_getPendingInterrupt(ADC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            sensors.joy_x = DL_ADC12_getMemResult(ADC_JOY_INST, DL_ADC12_MEM_IDX_0);
            break;
        default:
            break;
    }
}

//=============================================================================
// ADC1 INTERRUPT (Joystick Y, Accelerometer)
//=============================================================================
void ADC1_IRQHandler(void) {
    if (DL_ADC12_getPendingInterrupt(ADC_ACCEL_INST) == DL_ADC12_IIDX_MEM3_RESULT_LOADED) {
        sensors.accel_x = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_0);
        sensors.accel_y = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_1);
        sensors.accel_z = (int16_t)DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_2);
        sensors.joy_y = DL_ADC12_getMemResult(ADC_ACCEL_INST, DL_ADC12_MEM_IDX_3);
    }
}

//=============================================================================
// AUDIO TIMER ISR (Generate Audio @ 8 kHz)
//=============================================================================
void TIMG7_IRQHandler(void) {
    uint32_t status = DL_TimerG_getPendingInterrupt(TIMER_SAMPLE_INST);
    if (!(status & DL_TIMERG_IIDX_ZERO)) return;
    
    if (!app_state.playing) {
        // Silent
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        return;
    }
    
    // Process envelope
    Envelope_Process(&envelope);
    uint16_t amplitude = Envelope_GetAmplitude(&envelope);
    
    if (amplitude == 0) {
        // Silent
        DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, 2048, DL_TIMER_CC_0_INDEX);
        return;
    }
    
    // Generate waveform sample
    int16_t sample = Audio_GenerateSample();
    
    // Apply envelope
    sample = (sample * (int32_t)amplitude) / 1000;
    
    // Apply volume
    sample = (sample * (int32_t)app_state.volume) / 100;
    
    // Apply gain (frequency compensated)
    sample = Filter_GainWithFreqCompensation(sample, 8, 
                note_frequencies[app_state.current_note]);
    
    // Apply filters
    sample = Filter_LowPass(sample);
    sample = Filter_SoftClip(sample, 1600);
    
    // Convert to PWM
    uint16_t pwm_val = Audio_SampleToPWM(sample, 2048, 4095);
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, pwm_val, DL_TIMER_CC_0_INDEX);
}

//=============================================================================
// HARDFAULT HANDLER
//=============================================================================
void HardFault_Handler(void) {
    while (1) {
        DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}
