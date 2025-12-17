# 🎵⚙️ Pitch Bend vs Motor Control: Perfekt Analogi!

## 🎯 Konseptet: "Tweening" i Musikk og Motorer

### Hva du beskriver:
1. **"Vrenge lyden"** = Pitch bend (smooth overgang)
2. **"Hoppe 2 oktaver"** = Diskret skifte (direkte hopp)

Dette er **IDENTISK** med motor kontroll!

---

## 🎵 Musikk: Pitch Bend + Oktavhopp

### Pitch Bend (Smooth "Vreng")
```c
// Gradvis endring av frekvens (som en gitar-bend)
void pitch_bend_smooth() {
    float start_freq = 440.0f;   // A4
    float end_freq = 493.88f;    // B4
    float duration = 0.5f;        // 500ms
    
    for (float t = 0; t < 1.0f; t += 0.01f) {
        // Lineær interpolering (tweening)
        float current_freq = start_freq + (end_freq - start_freq) * t;
        set_frequency(current_freq);
        delay_ms(5);
    }
}
```

**Resultat:** Lyden "glir" smooth fra A til B 🎵

### Oktavhopp (Diskret Skifte)
```c
// Direkte hopp (2 oktaver = × 4 i frekvens)
void octave_jump() {
    float current_freq = 440.0f;   // A4
    
    // Hopp direkte opp 2 oktaver
    current_freq = current_freq * 4.0f;  // A6 = 1760 Hz
    set_frequency(current_freq);
}
```

**Resultat:** Lyden "hopper" umiddelbart opp! 🎵↗️

---

## ⚙️ Motor: Akselerasjon + Gir-skifte

### Smooth Akselerasjon (= Pitch Bend)
```c
// Gradvis økning av hastighet
void motor_accelerate_smooth() {
    int16_t start_rpm = 0;
    int16_t end_rpm = 3000;
    float duration = 2.0f;  // 2 sekunder
    
    for (float t = 0; t < 1.0f; t += 0.01f) {
        // Lineær interpolering (tweening)
        int16_t current_rpm = start_rpm + (end_rpm - start_rpm) * t;
        set_motor_speed(current_rpm);
        delay_ms(20);
    }
}
```

**Resultat:** Motor akselererer smooth fra 0 til 3000 RPM ⚙️

### Gir-skifte (= Oktavhopp)
```c
// Direkte skifte til høyere gir
void shift_gear() {
    float current_gear_ratio = 1.0f;   // Gir 1
    
    // Hopp til gir 3 (2.5x gir ratio)
    current_gear_ratio = 2.5f;
    set_gear_ratio(current_gear_ratio);
    
    // Motor RPM blir plutselig 2.5x lavere,
    // men hjul-hastighet forblir samme!
}
```

**Resultat:** "Hopp" i motor RPM ved gir-skifte! ⚙️↗️

---

## 📊 Side-by-Side Sammenligning

| Musikk | Motor | Konsept |
|--------|-------|---------|
| **Pitch Bend** | **Akselerasjon** | Smooth interpolering |
| Start: 440 Hz | Start: 0 RPM | Begin state |
| End: 880 Hz | End: 3000 RPM | Target state |
| Tween: 0.5s | Ramp: 2.0s | Transition time |
| Smooth glide 🎵 | Smooth ramp ⚙️ | User experience |
| | | |
| **Oktavhopp** | **Gir-skifte** | Diskret hopp |
| 440 Hz → 1760 Hz | 3000 RPM → 1200 RPM | Instant change |
| × 4 frekvens | ÷ 2.5 RPM | Ratio change |
| Direkte hopp 🎵↗️ | Direkte skifte ⚙️↗️ | No interpolation |

---

## 🎓 Teknisk Implementasjon: Tweening

### Musikk - Pitch Bend med Akselerometeret
```c
// Fra din synthesizer - akselerometer styrer pitch bend
void apply_pitch_bend() {
    // Les akselerometer (tilt)
    int16_t accel_y = g_synthState.accel_y;
    
    // Konverter til pitch bend (-12 til +12 semitones)
    float bend_semitones = ((float)accel_y - ACCEL_ZERO_G) / 100.0f;
    bend_semitones = CLAMP(bend_semitones, -12.0f, 12.0f);
    
    // Beregn ny frekvens (1 semitone = 2^(1/12) = 1.059463)
    float bend_ratio = powf(2.0f, bend_semitones / 12.0f);
    float bent_freq = g_synthState.frequency * bend_ratio;
    
    // Smooth interpolering (tweening)
    static float current_freq = 440.0f;
    float alpha = 0.1f;  // Smoothing factor
    current_freq = current_freq * (1.0f - alpha) + bent_freq * alpha;
    
    set_frequency(current_freq);
}
```

**Dette er tweening!** 🎵

### Motor - Smooth Akselerasjon med Encoder Feedback
```c
// Motor kontroll - encoder gir posisjon
void motor_smooth_control() {
    // Les encoder
    int32_t current_pos = g_motor_state.encoder_position;
    int32_t target_pos = g_motor_state.target_position;
    
    // Beregn error
    int32_t error = target_pos - current_pos;
    
    // PID kontroller (P-ledd gir smooth ramp)
    float Kp = 0.5f;
    int16_t control = (int16_t)(error * Kp);
    
    // Smooth interpolering (tweening via PID)
    static int16_t current_speed = 0;
    float alpha = 0.1f;  // Smoothing factor
    current_speed = current_speed * (1.0f - alpha) + control * alpha;
    
    set_motor_speed(current_speed);
}
```

**Dette er også tweening!** ⚙️

---

## 🎮 Kontrollmetoder

### 1. Lineær Interpolering (LERP)
```c
// Både musikk og motor
float lerp(float start, float end, float t) {
    return start + (end - start) * t;
}

// Musikk: Pitch bend
float freq = lerp(440.0f, 880.0f, 0.5f);  // 660 Hz

// Motor: Akselerasjon
int16_t rpm = (int16_t)lerp(0, 3000, 0.5f);  // 1500 RPM
```

### 2. Smooth Damp (Eksponensiell)
```c
// Mer naturlig bevegelse
float smooth_damp(float current, float target, float smoothing) {
    return current + (target - current) * smoothing;
}

// Musikk: Pitch wheel
static float current_pitch = 440.0f;
current_pitch = smooth_damp(current_pitch, target_pitch, 0.1f);

// Motor: Speed control
static int16_t current_rpm = 0;
current_rpm = (int16_t)smooth_damp(current_rpm, target_rpm, 0.1f);
```

### 3. Ease-In/Ease-Out (S-kurve)
```c
// Smooth start og smooth stopp
float ease_in_out(float t) {
    return t * t * (3.0f - 2.0f * t);  // Smoothstep
}

// Musikk: Vibrato fade-in
float vibrato_depth = ease_in_out(time) * MAX_VIBRATO;

// Motor: Smooth start/stop
float speed_multiplier = ease_in_out(time);
int16_t rpm = (int16_t)(MAX_RPM * speed_multiplier);
```

---

## 🎯 Praktisk Eksempel: Akselerometer → Pitch Bend

### Fra Din Kode
```c
// main.c - Dette er faktisk "motor control" for lyd!
void Process_Accelerometer() {
    // Les "sensor" (akselerometer = posisjon)
    volatile uint16_t accel_y = g_synthState.accel_y;
    
    // Beregn "error" (avvik fra senter)
    int16_t deviation = (int16_t)accel_y - ACCEL_ZERO_G;
    
    // Konverter til "kontroll signal" (pitch bend)
    float bend_amount = (float)deviation / (ACCEL_1G_VALUE * 2.0f);
    bend_amount = CLAMP(bend_amount, -1.0f, 1.0f);
    
    // Smooth "aktuering" (tween frequency)
    float target_freq = BASE_FREQ * powf(2.0f, bend_amount);
    g_synthState.frequency = smooth_damp(
        g_synthState.frequency, 
        target_freq, 
        0.1f  // Smoothing
    );
}
```

**Dette ER motor kontroll!** Bare for lyd i stedet for aktuator!

---

## 🚗 Real-World Motor Analogi

### Bil med Akselerasjon + Gir
```c
// Scenario: Bil som akselererer på motorvei

// Phase 1: Smooth akselerasjon i gir 1 (= pitch bend)
for (float t = 0; t < 1.0f; t += 0.01f) {
    float speed = lerp(0, 60, t);  // 0 → 60 km/h over tid
    set_car_speed(speed);
}

// Phase 2: Skifte til gir 2 (= oktavhopp)
shift_gear(2);  // Umiddelbart hopp i motor RPM!

// Phase 3: Akselerere igjen (= pitch bend)
for (float t = 0; t < 1.0f; t += 0.01f) {
    float speed = lerp(60, 120, t);  // 60 → 120 km/h
    set_car_speed(speed);
}
```

**Kjennes som din synthesizer, ikke sant?** 🎵🚗

---

## 🎹 Synthesizer som "Motor"

### Konseptuelt:
```
SYNTHESIZER          = MOTOR SYSTEM
---------------------------------------------
Oscillator           = Motor
Frequency            = RPM
Pitch Bend           = Akselerasjon
Oktavhopp            = Gir-skifte
Vibrato (LFO)        = Ripple/oscillation
Volume               = Torque/kraft
Waveform             = Motor type
Envelope (ADSR)      = Ramp-up/down profil
```

### Kontroll Loop:
```
┌─────────────────────────────────────────┐
│         CLOSED-LOOP SYSTEM              │
│                                         │
│  Sensor → Error → Controller → Output  │
│    ↑                             ↓      │
│    └──────────── Feedback ───────┘      │
└─────────────────────────────────────────┘

SYNTH:
  Accelerometer → Deviation → Pitch Bend → Frequency
       ↑                                      ↓
       └──────────── (ears listen) ──────────┘

MOTOR:
  Encoder → Error → PID → PWM → Motor RPM
      ↑                          ↓
      └──────── Feedback ─────────┘
```

**Identisk struktur!** 🎯

---

## 💻 Kode: Implementer Pitch Bend i Din Synth

### Legg til i main.c:
```c
// Global state for pitch bend
static float current_bent_freq = FREQ_DEFAULT;

/**
 * @brief Apply smooth pitch bend from accelerometer
 * @note Called from Process_Input() at SENSOR_UPDATE_HZ
 */
static void Apply_Pitch_Bend(void) {
    // Read accelerometer Y-axis (tilt forward/back)
    uint16_t accel_y_local = g_synthState.accel_y;
    
    // Calculate deviation from center (normalized -1 to +1)
    float deviation = ((float)accel_y_local - ACCEL_ZERO_G) / (ACCEL_1G_VALUE * 2.0f);
    deviation = CLAMP(deviation, -1.0f, 1.0f);
    
    // Map to pitch bend range (±2 octaves = ±24 semitones)
    float bend_semitones = deviation * PITCH_BEND_RANGE;  // ±24
    
    // Calculate frequency ratio (semitone = 2^(1/12))
    float bend_ratio = powf(2.0f, bend_semitones / 12.0f);
    
    // Calculate target frequency
    float target_freq = g_synthState.frequency * bend_ratio;
    target_freq = CLAMP(target_freq, FREQ_MIN, FREQ_MAX);
    
    // TWEENING: Smooth interpolation (exponential smoothing)
    float alpha = 0.15f;  // Smoothing factor (0.0 = no change, 1.0 = instant)
    current_bent_freq = current_bent_freq * (1.0f - alpha) + target_freq * alpha;
    
    // Apply bent frequency to audio engine
    phase_increment = (uint32_t)((current_bent_freq * 4294967296.0) / SAMPLE_RATE);
    
    // Visual feedback on LCD
    if (fabs(bend_semitones) > 0.5f) {
        g_synthState.display_update_needed = true;
    }
}
```

### Oktavhopp Funksjon:
```c
/**
 * @brief Jump up/down by octaves (discrete shift)
 * @param octaves Number of octaves to shift (positive = up, negative = down)
 */
static void Octave_Jump(int8_t octaves) {
    // Calculate frequency ratio (octave = × 2)
    float ratio = powf(2.0f, (float)octaves);
    
    // DISCRETE JUMP: No interpolation!
    g_synthState.frequency *= ratio;
    g_synthState.frequency = CLAMP(g_synthState.frequency, FREQ_MIN, FREQ_MAX);
    
    // Update immediately
    Update_Phase_Increment();
    
    // Visual feedback
    g_synthState.display_update_needed = true;
    
    // Audio "click" to hear the jump
    // (optional: add short click sound)
}
```

### Integrer i Process_Input():
```c
static void Process_Input(void) {
    // ... existing code ...
    
    // Apply pitch bend from accelerometer (smooth)
    Apply_Pitch_Bend();
    
    // Octave jump on button hold + joystick
    static uint32_t last_octave_change = 0;
    if (g_synthState.btn_s1 && g_synthState.joy_pressed) {
        uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
        if (TIMER_ELAPSED(now, last_octave_change) > (SYSCLK_FREQUENCY / 2)) {
            // S1 + Joy = Up octave
            Octave_Jump(+1);  // Discrete jump!
            last_octave_change = now;
        }
    } else if (g_synthState.btn_s2 && g_synthState.joy_pressed) {
        uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
        if (TIMER_ELAPSED(now, last_octave_change) > (SYSCLK_FREQUENCY / 2)) {
            // S2 + Joy = Down octave
            Octave_Jump(-1);  // Discrete jump!
            last_octave_change = now;
        }
    }
}
```

---

## 🎛️ Tuning Parameters

### Tweening Smoothness
```c
// Slower response (more smoothing)
float alpha = 0.05f;  // Very smooth, "lazy" feeling

// Balanced
float alpha = 0.15f;  // Default, good for most cases

// Faster response (less smoothing)
float alpha = 0.5f;   // Snappy, responsive feeling
```

**Motor Analogy:** Dette er som "stiffness" i PID-kontroller!

### Pitch Bend Range
```c
// Conservative (±1 oktav = ±12 semitones)
#define PITCH_BEND_RANGE 12

// Standard (±2 oktaver = ±24 semitones)
#define PITCH_BEND_RANGE 24

// Extreme (±3 oktaver = ±36 semitones)
#define PITCH_BEND_RANGE 36
```

**Motor Analogy:** Dette er som "max acceleration"!

---

## 📊 Comparison Matrix

| Feature | Synthesizer | Motor | Implementation |
|---------|-------------|-------|----------------|
| **Smooth Change** | Pitch bend | Acceleration | `lerp()` / `smooth_damp()` |
| **Discrete Jump** | Octave shift | Gear shift | Direct assignment |
| **Feedback** | Ears listen | Encoder reads | Closed-loop |
| **Sensor** | Accelerometer | Position sensor | ADC/GPIO |
| **Control** | Frequency | Speed/position | PWM/Compare |
| **Smoothing** | Alpha filter | PID I-term | Exponential average |
| **Limits** | Min/Max freq | Min/Max RPM | `CLAMP()` |
| **Response** | Real-time | Real-time | ISR-driven |

---

## 🎯 Key Insights

### 1. Tweening = Smooth Control
```c
// Same algorithm, different domain!

// Music:
freq = freq * 0.9f + target_freq * 0.1f;

// Motor:
rpm = rpm * 0.9f + target_rpm * 0.1f;
```

### 2. Discrete Shifts = Step Changes
```c
// Same concept, different application!

// Music (octave):
freq = freq * 2.0f;  // Up one octave

// Motor (gear):
gear_ratio = 2.5f;   // Change gear ratio
effective_rpm = motor_rpm / gear_ratio;
```

### 3. Closed-Loop Control
```
Both systems need feedback to sound/perform well!

Synth WITHOUT feedback: Drift, out of tune
Motor WITHOUT feedback: Drift, lose position

Synth WITH feedback: Stable, in tune
Motor WITH feedback: Precise positioning
```

---

## 💡 Further Analogies

| Music Term | Motor Term | Both Mean |
|------------|------------|-----------|
| Vibrato | Oscillation | Periodic variation |
| Tremolo | Pulsing | Amplitude modulation |
| Portamento | Slew rate | Maximum rate of change |
| Attack | Ramp-up | Time to reach speed |
| Release | Ramp-down | Time to stop |
| Sustain | Hold | Maintain level |
| Resonance | Q-factor | Frequency selectivity |

---

## 🚀 Exercise: Add "Motor-Style" Features to Synth

### 1. Add Velocity Limiting (Slew Rate)
```c
// Limit how fast frequency can change (portamento)
#define MAX_FREQ_CHANGE_PER_SECOND 1000.0f  // Hz/s

void limit_frequency_change(float target_freq) {
    float max_change = MAX_FREQ_CHANGE_PER_SECOND / SENSOR_UPDATE_HZ;
    float diff = target_freq - g_synthState.frequency;
    
    if (fabs(diff) > max_change) {
        // Limit the change
        diff = (diff > 0) ? max_change : -max_change;
    }
    
    g_synthState.frequency += diff;
}
```

### 2. Add "Gear Ratios" (Frequency Multipliers)
```c
// Discrete frequency multipliers like gears
const float FREQ_GEARS[] = {
    0.5f,   // "Gear 1" - half frequency
    1.0f,   // "Gear 2" - normal
    2.0f,   // "Gear 3" - double (octave up)
    4.0f    // "Gear 4" - quadruple (2 octaves up)
};
```

### 3. Add PID-Style Control
```c
// PID controller for frequency (like motor position control)
typedef struct {
    float Kp, Ki, Kd;
    float integral, last_error;
} PID_t;

float pid_update(PID_t *pid, float target, float current, float dt) {
    float error = target - current;
    pid->integral += error * dt;
    float derivative = (error - pid->last_error) / dt;
    pid->last_error = error;
    
    return pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;
}
```

---

## 🎓 Conclusion

**"Tweening" i musikk = "Ramping" i motor kontroll**

- ✅ Pitch bend = Akselerasjon
- ✅ Oktavhopp = Gir-skifte  
- ✅ Smooth control = PID
- ✅ Feedback loops = Stability
- ✅ Same math = Different domains!

**Dette er hvorfor embedded systems engineering er så kraftig:**  
De samme teknikkene fungerer overalt! 🎵⚙️🤖

---

**Want me to add this "motor-style" control to your synthesizer code?** 🚀
