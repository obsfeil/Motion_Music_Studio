# EDUMKII Hardware Abstraction Library

**Version:** 1.0.0  
**Date:** 2025-12-29  
**Author:** MSPM0 Synthesizer Project  
**License:** MIT

A modular, reusable library for the BOOSTXL-EDUMKII BoosterPack with MSPM0G3507.

---

## 📦 What's Included

### Hardware Abstraction (`lib/edumkii/`)
- **Buttons** - State machine with short/long/double click detection
- **Joystick** - Deadzone filtering, change detection
- **Accelerometer** - Tilt detection, position mapping

### Audio Engine (`lib/audio/`)
- **Waveforms** - Sine, square, sawtooth, triangle
- **Envelope** - ADSR with predefined profiles
- **Filters** - Low-pass, soft clipping, gain control

---

## 🚀 Quick Start

### 1. Add to Your Project

Copy the `lib/` folder to your CCS project:

```
YourProject/
├── lib/
│   ├── edumkii/
│   └── audio/
├── main.c
└── ti_msp_dl_config.h
```

### 2. Include in Your Code

```c
#include "lib/edumkii/edumkii.h"
#include "lib/audio/audio_engine.h"
#include "lib/audio/audio_envelope.h"
#include "lib/audio/audio_filters.h"
```

### 3. Create Hardware Objects

```c
// Buttons
Button_t btn_s1, btn_s2, btn_joy_sel;

// Joystick
Joystick_t joystick;

// Accelerometer
Accelerometer_t accel;

// Audio
Envelope_t envelope;
```

### 4. Initialize

```c
int main(void) {
    SYSCFG_DL_init();
    
    // Init hardware
    Button_Init(&btn_s1);
    Button_Init(&btn_s2);
    Button_Init(&btn_joy_sel);
    Joystick_Init(&joystick, 100);  // 100 = deadzone
    Accel_Init(&accel, 100);
    
    // Init audio
    Audio_Init(8000);  // 8 kHz sample rate
    Envelope_Init(&envelope, &ADSR_PIANO);
    
    // ...
}
```

### 5. Update in SysTick (100 Hz)

```c
void SysTick_Handler(void) {
    Button_Update(&btn_s1, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_MKII_PIN);
    Button_Update(&btn_s2, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_MKII_PIN);
    Button_Update(&btn_joy_sel, GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
}
```

### 6. Use in Main Loop

```c
while (1) {
    // Update inputs
    Joystick_Update(&joystick, gSynthState.joy_x, gSynthState.joy_y);
    Accel_Update(&accel, gSynthState.accel_x, gSynthState.accel_y, gSynthState.accel_z);
    
    // Handle button events
    ButtonEvent_t event = Button_GetEvent(&btn_s1);
    if (event == BTN_EVENT_SHORT_CLICK) {
        Envelope_NoteOn(&envelope);
    }
    
    // Handle joystick
    if (joystick.x_changed) {
        uint8_t key = Joystick_GetKeyIndex(&joystick, 7);
        Audio_SetFrequency(frequencies[key]);
    }
    
    // Handle accelerometer
    int8_t tilt = Accel_GetTilt(&accel);
    // Use tilt for octave shift...
}
```

### 7. Generate Audio (in Timer ISR)

```c
void TIMG7_IRQHandler(void) {
    // Process envelope
    Envelope_Process(&envelope);
    
    // Generate sample
    int16_t sample = Audio_GenerateSample();
    
    // Apply envelope
    sample = (sample * Envelope_GetAmplitude(&envelope)) / 1000;
    
    // Apply filters
    sample = Filter_LowPass(sample);
    sample = Filter_SoftClip(sample, 1600);
    
    // Output to PWM
    uint16_t pwm_val = Audio_SampleToPWM(sample, 2048, 4095);
    DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, pwm_val, DL_TIMER_CC_0_INDEX);
}
```

---

## 📚 API Reference

### Buttons

```c
void Button_Init(Button_t *btn);
void Button_Update(Button_t *btn, uint32_t gpio_port, uint32_t gpio_pin);
ButtonEvent_t Button_GetEvent(Button_t *btn);
bool Button_IsPressed(Button_t *btn);
```

**Events:**
- `BTN_EVENT_NONE` - No event
- `BTN_EVENT_SHORT_CLICK` - Short click (< 200ms)
- `BTN_EVENT_LONG_PRESS` - Long press (> 500ms)
- `BTN_EVENT_DOUBLE_CLICK` - Double click

### Joystick

```c
void Joystick_Init(Joystick_t *joy, uint16_t deadzone);
void Joystick_Update(Joystick_t *joy, uint16_t raw_x, uint16_t raw_y);
int16_t Joystick_GetX(Joystick_t *joy);  // -2048 to +2047
int16_t Joystick_GetY(Joystick_t *joy);
uint8_t Joystick_GetKeyIndex(Joystick_t *joy, uint8_t num_keys);
uint8_t Joystick_GetVolume(Joystick_t *joy);  // 0-100% or 255=no change
bool Joystick_IsCentered(Joystick_t *joy);
```

### Accelerometer

```c
void Accel_Init(Accelerometer_t *accel, uint16_t deadzone);
void Accel_Update(Accelerometer_t *accel, int16_t raw_x, int16_t raw_y, int16_t raw_z);
int8_t Accel_GetTilt(Accelerometer_t *accel);  // -1, 0, +1
uint8_t Accel_GetScalePosition(Accelerometer_t *accel);  // 0-7
int16_t Accel_GetXDeviation(Accelerometer_t *accel);
int16_t Accel_GetYDeviation(Accelerometer_t *accel);
bool Accel_IsFlat(Accelerometer_t *accel);
```

### Audio Engine

```c
void Audio_Init(uint16_t sample_rate_hz);
void Audio_SetFrequency(uint32_t frequency_hz);
void Audio_SetWaveform(Waveform_t waveform);
int16_t Audio_GenerateSample(void);
int16_t Audio_GenerateWaveform(uint8_t index, Waveform_t waveform);
```

**Waveforms:**
- `WAVE_SINE` - Pure tone
- `WAVE_SQUARE` - Bright, harsh
- `WAVE_SAWTOOTH` - Buzzy
- `WAVE_TRIANGLE` - Mellow

### Envelope

```c
void Envelope_Init(Envelope_t *env, const ADSR_Profile_t *profile);
void Envelope_NoteOn(Envelope_t *env);
void Envelope_NoteOff(Envelope_t *env);
void Envelope_Process(Envelope_t *env);
uint16_t Envelope_GetAmplitude(Envelope_t *env);  // 0-1000
```

**Predefined Profiles:**
- `ADSR_PIANO` - Fast attack, medium decay
- `ADSR_ORGAN` - Instant attack, no decay
- `ADSR_STRINGS` - Slow attack, long sustain
- `ADSR_BASS` - Medium attack, short decay
- `ADSR_LEAD` - Fast attack, long sustain

### Filters

```c
int16_t Filter_LowPass(int16_t new_sample);
int16_t Filter_LowPassAlpha(int16_t new_sample, uint8_t alpha);
int16_t Filter_HighPass(int16_t new_sample);
int16_t Filter_SoftClip(int16_t sample, int16_t threshold);
int16_t Filter_HardClip(int16_t sample, int16_t limit);
int16_t Filter_GainWithFreqCompensation(int16_t sample, uint8_t gain, uint32_t frequency_hz);
uint16_t Audio_SampleToPWM(int16_t sample, uint16_t pwm_center, uint16_t pwm_max);
```

---

## 🎯 Design Philosophy

### Why This Structure?

1. **Reusability** - Drop into any project
2. **Clean separation** - Hardware vs. application logic
3. **Easy testing** - Test modules independently
4. **Clear API** - Self-documenting code
5. **No dependencies** - Only requires TI DriverLib

### Deadzone Explained

All inputs (joystick, accelerometer) use deadzone filtering:

```
Before:  2048 → 2049 → 2050 → 2048 → 2047  (noise!)
After:   2048 → 2048 → 2048 → 2048 → 2048  (stable!)
```

Benefits:
- Prevents jitter
- Stops value drift
- Reduces CPU load
- Holds values when released

---

## 📝 Examples

See `example_main.c` for complete working example.

---

## 🔧 Configuration

### Adjust Deadzone

```c
Joystick_Init(&joy, 50);   // Small deadzone (more sensitive)
Joystick_Init(&joy, 200);  // Large deadzone (less sensitive)
```

### Custom ADSR Profile

```c
ADSR_Profile_t my_profile = {
    .attack_samples = 400,    // 50ms @ 8kHz
    .decay_samples = 2400,    // 300ms
    .sustain_level = 850,     // 85%
    .release_samples = 1600   // 200ms
};

Envelope_Init(&env, &my_profile);
```

### Change Sample Rate

```c
Audio_Init(16000);  // 16 kHz sample rate

// Don't forget to update SysConfig:
// TIMER1.timerPeriod = "62.5 us"
```

---

## 🐛 Troubleshooting

### Joystick not holding value
- Check deadzone is set (recommended: 100)
- Verify `Joystick_Update()` called every loop
- Check for other code modifying values

### Buttons not responding
- Verify `Button_Update()` called from SysTick @ 100 Hz
- Check GPIO pins match your hardware
- Use `Button_IsPressed()` to test raw input

### Audio distortion
- Reduce `AUDIO_GAIN_BOOST` (try 4-8)
- Use `Filter_SoftClip()` with threshold 1600
- Check sample rate matches SysConfig

### Accelerometer drift
- Increase deadzone (try 150-200)
- Calibrate neutral values for your board
- Use `Accel_IsFlat()` to detect stability

---

## 📄 License

MIT License - Feel free to use in any project!

---

## 🙏 Credits

Based on MSPM0G3507 Synthesizer Project v27.0
- Button state machine with deadzone filtering
- Musical scale system
- Professional audio synthesis

---

## 🚀 Next Steps

1. Copy `lib/` to your project
2. Run `example_main.c` to test
3. Start building your own application!
4. Share improvements back to the community

Happy coding! 🎵✨