# 🤖 MSPM0 Synthesizer → Robot Controller Migration Guide

## 📋 Overview

This guide transforms the **MSPM0G3507 Audio Synthesizer** into a **Robot Controller** platform while maintaining the same architecture and development methodology.

**What stays the same:**
- ✅ Phase-based development approach
- ✅ LCD driver and graphics system
- ✅ Joystick/button input handling
- ✅ Startup file and build system
- ✅ Interrupt-driven architecture

**What changes:**
- 🔄 Audio PWM → Motor PWM (H-bridge control)
- 🔄 Waveform generation → Motor speed/direction control
- 🔄 Waveform display → Sensor dashboard
- 🔄 Frequency/volume → Speed/steering
- ➕ Distance sensors (ultrasonic)
- ➕ Battery monitoring
- ➕ Autonomous mode

---

## 🎯 Project Phases

### Phase 1: Basic Motor Control (Week 1)
**Goal:** Replace audio synthesis with motor control
- PWM for 2 DC motors via H-bridge
- Joystick control (speed + steering)
- LCD displays motor speeds
- Buttons for start/stop

### Phase 2: Sensor Dashboard (Week 2)
**Goal:** Add sensor feedback and display
- 2x HC-SR04 ultrasonic sensors (front/back)
- Battery voltage monitoring (ADC)
- Real-time dashboard on LCD
- Obstacle warnings

### Phase 3: Autonomous Mode (Week 3+)
**Goal:** Implement autonomous behavior
- Obstacle avoidance algorithm
- Mode switching (manual/autonomous)
- Path recording/playback (optional)
- Advanced features (line following, etc.)

---

## 🔧 Hardware Changes

### Components to Add

| Component | Quantity | Purpose | Connection |
|-----------|----------|---------|------------|
| **L298N H-Bridge** | 1 | Motor driver | PWM + power |
| **DC Motors (6V)** | 2 | Drive wheels | Via L298N |
| **HC-SR04** | 2 | Distance sensors | GPIO + Timer |
| **LiPo 2S Battery** | 1 | Power (7.4V) | Via L298N |
| **Voltage Divider** | 1 | Battery monitor | ADC input |
| **Robot Chassis** | 1 | Physical platform | Mechanical |

### Pin Mapping Changes

#### REMOVE (From Synthesizer):
```
PB.10: PWM Audio Output → REPURPOSE for motor control
```

#### ADD (For Robot Controller):

**Motor Control (PWM):**
```
PB.10: Left Motor Forward   (TIMG0_CCP0)
PB.11: Left Motor Reverse   (TIMG0_CCP1)
PB.12: Right Motor Forward  (TIMG1_CCP0)
PB.13: Right Motor Reverse  (TIMG1_CCP1)
```

**Distance Sensors:**
```
PA.10: Front Trigger (GPIO Output)
PA.11: Front Echo    (GPIO Input + Capture)
PA.12: Back Trigger  (GPIO Output)
PA.13: Back Echo     (GPIO Input + Capture)
```

**Battery Monitor:**
```
PA.24: Battery Voltage (ADC1_CH0 via voltage divider)
       - 10kΩ resistor to battery
       - 10kΩ resistor to ground
       - Measures 0-8.2V as 0-3.3V on ADC
```

**Keep from Synthesizer:**
```
// LCD (SPI1)
PB.6:  LCD_CS
PB.7:  LCD_MISO (POCI)
PB.8:  LCD_MOSI (PICO)
PB.9:  LCD_SCLK
PB.15: LCD_RST
PB.17: LCD_DC

// Joystick (ADC0)
PA.26: JOY_X (ADC0_CH1)
PA.27: JOY_Y (ADC0_CH0)

// Buttons (GPIO)
PA.14: S2 Button
PA.15: Joystick Press
PA.16: S1 Button

// RGB LED
PB.16: LED_RED
PB.18: LED_BLUE
PB.19: LED_GREEN
```

---

## 📝 Code Changes

### 1. Update `main.h` - State Structure

**REPLACE:**
```c
// Synthesizer state
typedef struct {
    Waveform_t waveform;
    Mode_t mode;
    float frequency;
    uint8_t volume;
    int8_t pitchBend;
    bool audio_playing;
    // ... rest of synth state
} SynthState_t;
```

**WITH:**
```c
// Robot controller state
typedef enum {
    MOTOR_STOP,
    MOTOR_FORWARD,
    MOTOR_REVERSE,
    MOTOR_TURN_LEFT,
    MOTOR_TURN_RIGHT,
    MOTOR_MODE_COUNT
} MotorMode_t;

typedef struct {
    // Motor control
    MotorMode_t motor_mode;
    int16_t left_motor_speed;   // -4095 to +4095 (negative = reverse)
    int16_t right_motor_speed;  // -4095 to +4095
    bool motors_enabled;
    
    // Control mode
    bool autonomous_mode;       // false = manual, true = autonomous
    
    // Sensors
    uint16_t distance_front_cm; // 0-400 cm
    uint16_t distance_back_cm;  // 0-400 cm
    float battery_voltage;      // 6.0-8.4V typical for 2S LiPo
    
    // Input (keep from synthesizer)
    uint16_t joy_x;
    uint16_t joy_y;
    bool joy_pressed;
    bool btn_s1;
    bool btn_s2;
    
    // Display
    bool display_update_needed;
} RobotState_t;

// Global state
RobotState_t g_robotState;
```

**ADD Constants:**
```c
// Motor control constants
#define MAX_MOTOR_SPEED         4095    // 12-bit PWM
#define MIN_MOTOR_SPEED         0
#define MOTOR_DEADZONE          200     // Ignore small joystick movements

// Distance sensor constants
#define DISTANCE_OBSTACLE_CM    20      // Obstacle warning threshold
#define DISTANCE_CRITICAL_CM    10      // Emergency stop threshold
#define DISTANCE_TIMEOUT_US     30000   // 30ms timeout (5m max range)

// Battery monitoring
#define BATTERY_MIN_VOLTAGE     6.0f    // Low battery warning
#define BATTERY_MAX_VOLTAGE     8.4f    // Fully charged 2S LiPo
#define BATTERY_DIVIDER_RATIO   2.0f    // 10k/10k voltage divider

// Update rates
#define MOTOR_UPDATE_HZ         50      // 50 Hz motor update
#define SENSOR_UPDATE_HZ        10      // 10 Hz sensor reading
#define DISPLAY_UPDATE_HZ       10      // 10 Hz display refresh
```

---

### 2. Update `main.c` - Core Functions

#### REPLACE Audio Generation:
```c
// DELETE: Audio synthesis functions
static void Update_Phase_Increment(void);
static void Generate_Audio_Sample(void);
```

#### WITH Motor Control:
```c
//=============================================================================
// MOTOR CONTROL
//=============================================================================

/**
 * @brief Set motor PWM values for differential drive
 * @param left_speed: -4095 to +4095 (negative = reverse)
 * @param right_speed: -4095 to +4095 (negative = reverse)
 */
static void Set_Motor_Speeds(int16_t left_speed, int16_t right_speed) {
    // Clamp speeds to valid range
    left_speed = CLAMP(left_speed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    right_speed = CLAMP(right_speed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    
    // Left motor
    if (left_speed >= 0) {
        // Forward
        DL_TimerG_setCaptureCompareValue(MOTOR_LEFT_FWD_INST, left_speed, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(MOTOR_LEFT_REV_INST, 0, DL_TIMER_CC_0_INDEX);
    } else {
        // Reverse
        DL_TimerG_setCaptureCompareValue(MOTOR_LEFT_FWD_INST, 0, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(MOTOR_LEFT_REV_INST, -left_speed, DL_TIMER_CC_0_INDEX);
    }
    
    // Right motor
    if (right_speed >= 0) {
        // Forward
        DL_TimerG_setCaptureCompareValue(MOTOR_RIGHT_FWD_INST, right_speed, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(MOTOR_RIGHT_REV_INST, 0, DL_TIMER_CC_0_INDEX);
    } else {
        // Reverse
        DL_TimerG_setCaptureCompareValue(MOTOR_RIGHT_FWD_INST, 0, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(MOTOR_RIGHT_REV_INST, -right_speed, DL_TIMER_CC_0_INDEX);
    }
    
    g_robotState.left_motor_speed = left_speed;
    g_robotState.right_motor_speed = right_speed;
}

/**
 * @brief Set motor mode (convenience function)
 */
static void Set_Motor_Mode(MotorMode_t mode) {
    g_robotState.motor_mode = mode;
    
    switch (mode) {
        case MOTOR_STOP:
            Set_Motor_Speeds(0, 0);
            break;
            
        case MOTOR_FORWARD:
            Set_Motor_Speeds(MAX_MOTOR_SPEED / 2, MAX_MOTOR_SPEED / 2);
            break;
            
        case MOTOR_REVERSE:
            Set_Motor_Speeds(-MAX_MOTOR_SPEED / 2, -MAX_MOTOR_SPEED / 2);
            break;
            
        case MOTOR_TURN_LEFT:
            Set_Motor_Speeds(-MAX_MOTOR_SPEED / 3, MAX_MOTOR_SPEED / 3);
            break;
            
        case MOTOR_TURN_RIGHT:
            Set_Motor_Speeds(MAX_MOTOR_SPEED / 3, -MAX_MOTOR_SPEED / 3);
            break;
            
        default:
            Set_Motor_Speeds(0, 0);
            break;
    }
}

/**
 * @brief Emergency stop - immediately halt all motors
 */
static void Emergency_Stop(void) {
    Set_Motor_Speeds(0, 0);
    g_robotState.motors_enabled = false;
    
    // Flash red LED
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
    DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN | GPIO_RGB_BLUE_PIN);
}
```

---

### 3. ADD Sensor Functions

```c
//=============================================================================
// SENSOR READING
//=============================================================================

/**
 * @brief Measure distance using HC-SR04 ultrasonic sensor
 * @param trigger_port: GPIO port for trigger pin
 * @param trigger_pin: GPIO pin for trigger
 * @param echo_port: GPIO port for echo pin
 * @param echo_pin: GPIO pin for echo
 * @return Distance in centimeters (0 = timeout/error)
 */
static uint16_t Measure_Distance_Ultrasonic(
    GPIOA_Regs *trigger_port, uint32_t trigger_pin,
    GPIOA_Regs *echo_port, uint32_t echo_pin) {
    
    // Send 10us trigger pulse
    DL_GPIO_clearPins(trigger_port, trigger_pin);
    delay_us(2);
    DL_GPIO_setPins(trigger_port, trigger_pin);
    delay_us(10);
    DL_GPIO_clearPins(trigger_port, trigger_pin);
    
    // Wait for echo to go high (with timeout)
    uint32_t timeout = SYSCLK_FREQUENCY / 100000; // 10ms
    while (!DL_GPIO_readPins(echo_port, echo_pin) && timeout--) {
        __NOP();
    }
    if (timeout == 0) return 0; // Timeout
    
    // Measure echo pulse width
    uint32_t start_time = DL_Timer_getTimerCount(TIMER_DISTANCE_INST);
    
    timeout = SYSCLK_FREQUENCY / 33; // 30ms max (5m range)
    while (DL_GPIO_readPins(echo_port, echo_pin) && timeout--) {
        __NOP();
    }
    
    uint32_t end_time = DL_Timer_getTimerCount(TIMER_DISTANCE_INST);
    
    if (timeout == 0) return 0; // Timeout
    
    // Calculate distance
    // Distance (cm) = (pulse_width_seconds * speed_of_sound_cm/s) / 2
    // speed_of_sound = 34300 cm/s at 20°C
    uint32_t pulse_width_ticks = end_time - start_time;
    uint32_t pulse_width_us = (pulse_width_ticks * 1000000) / SYSCLK_FREQUENCY;
    uint16_t distance_cm = (pulse_width_us * 343) / 20000; // Divide by 2 and convert
    
    return distance_cm;
}

/**
 * @brief Read all sensors and update state
 */
static void Update_Sensors(void) {
    // Distance sensors
    g_robotState.distance_front_cm = Measure_Distance_Ultrasonic(
        GPIOA, DISTANCE_FRONT_TRIG_PIN,
        GPIOA, DISTANCE_FRONT_ECHO_PIN
    );
    
    delay_ms(50); // Wait between sensor readings
    
    g_robotState.distance_back_cm = Measure_Distance_Ultrasonic(
        GPIOA, DISTANCE_BACK_TRIG_PIN,
        GPIOA, DISTANCE_BACK_ECHO_PIN
    );
    
    // Battery voltage is updated via ADC interrupt
    
    // Check for critical obstacles
    if (g_robotState.distance_front_cm < DISTANCE_CRITICAL_CM &&
        g_robotState.distance_front_cm > 0) {
        Emergency_Stop();
    }
    
    g_robotState.display_update_needed = true;
}
```

---

### 4. REPLACE Input Processing

**REPLACE:**
```c
static void Process_Input(void) {
    // Joystick X: Frequency
    // Joystick Y: Volume
    // S1: Waveform select
    // S2: Play/Stop
}
```

**WITH:**
```c
//=============================================================================
// INPUT PROCESSING
//=============================================================================

/**
 * @brief Process joystick input for manual control
 */
static void Process_Manual_Control(void) {
    // Get joystick deflection from center
    int16_t joy_x = (int16_t)g_robotState.joy_x - JOY_ADC_CENTER;
    int16_t joy_y = (int16_t)g_robotState.joy_y - JOY_ADC_CENTER;
    
    // Apply deadzone
    if (abs(joy_x) < JOY_DEADZONE) joy_x = 0;
    if (abs(joy_y) < JOY_DEADZONE) joy_y = 0;
    
    // Check if joystick is neutral
    if (joy_x == 0 && joy_y == 0) {
        Set_Motor_Speeds(0, 0);
        return;
    }
    
    // Calculate base speed from Y axis (forward/backward)
    int16_t base_speed = (joy_y * MAX_MOTOR_SPEED) / (JOY_ADC_MAX / 2);
    
    // Calculate turn amount from X axis (differential steering)
    int16_t turn_amount = (joy_x * MAX_MOTOR_SPEED) / (JOY_ADC_MAX / 2);
    
    // Apply differential steering
    int16_t left_speed = base_speed - (turn_amount / 2);
    int16_t right_speed = base_speed + (turn_amount / 2);
    
    // Update motors
    Set_Motor_Speeds(left_speed, right_speed);
    g_robotState.display_update_needed = true;
}

/**
 * @brief Simple obstacle avoidance algorithm
 */
static void Process_Autonomous_Control(void) {
    uint16_t front = g_robotState.distance_front_cm;
    
    if (front < DISTANCE_OBSTACLE_CM && front > 0) {
        // Obstacle detected - avoid it
        Set_Motor_Mode(MOTOR_STOP);
        delay_ms(300);
        
        Set_Motor_Mode(MOTOR_REVERSE);
        delay_ms(800);
        
        // Turn random direction (use timer count for pseudo-random)
        if (DL_Timer_getTimerCount(TIMER_DISTANCE_INST) & 0x01) {
            Set_Motor_Mode(MOTOR_TURN_LEFT);
        } else {
            Set_Motor_Mode(MOTOR_TURN_RIGHT);
        }
        delay_ms(600);
    } else {
        // No obstacle - drive forward
        Set_Motor_Mode(MOTOR_FORWARD);
    }
}

/**
 * @brief Process button inputs
 */
static void Process_Buttons(void) {
    // S1: Toggle autonomous mode
    static bool last_s1 = false;
    if (g_robotState.btn_s1 && !last_s1) {
        g_robotState.autonomous_mode = !g_robotState.autonomous_mode;
        
        if (g_robotState.autonomous_mode) {
            // Green LED = autonomous
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_BLUE_PIN);
        } else {
            // Blue LED = manual
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN | GPIO_RGB_GREEN_PIN);
        }
        
        g_robotState.display_update_needed = true;
    }
    last_s1 = g_robotState.btn_s1;
    
    // S2 or Joy Press: Emergency stop / Enable motors
    static bool last_s2 = false;
    static bool last_joy = false;
    
    if ((g_robotState.btn_s2 && !last_s2) || (g_robotState.joy_pressed && !last_joy)) {
        if (g_robotState.motors_enabled) {
            Emergency_Stop();
        } else {
            g_robotState.motors_enabled = true;
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_RED_PIN);
        }
        g_robotState.display_update_needed = true;
    }
    last_s2 = g_robotState.btn_s2;
    last_joy = g_robotState.joy_pressed;
}

/**
 * @brief Main input processing dispatcher
 */
static void Process_Input(void) {
    static uint32_t last_update = 0;
    uint32_t now = DL_Timer_getTimerCount(TIMER_DISTANCE_INST);
    
    if ((now - last_update) < (SYSCLK_FREQUENCY / MOTOR_UPDATE_HZ)) {
        return;
    }
    last_update = now;
    
    // Process buttons first (mode switching)
    Process_Buttons();
    
    // Only process motor control if enabled
    if (!g_robotState.motors_enabled) {
        return;
    }
    
    // Control mode selection
    if (g_robotState.autonomous_mode) {
        Process_Autonomous_Control();
    } else {
        Process_Manual_Control();
    }
}
```

---

### 5. REPLACE Display Functions

**REPLACE:**
```c
static void Update_Display(void) {
    // Show waveform, frequency, volume
}
```

**WITH:**
```c
//=============================================================================
// DISPLAY UPDATE
//=============================================================================

/**
 * @brief Update LCD dashboard with robot status
 */
static void Update_Display(void) {
    LCD_Clear(COLOR_BLACK);
    
    char str[32];
    
    // === TITLE BAR ===
    const char *mode_names[] = {"STOP", "FWD", "REV", "LEFT", "RIGHT"};
    snprintf(str, 32, "MODE: %s", mode_names[g_robotState.motor_mode]);
    LCD_DrawString(5, 5, str, COLOR_YELLOW);
    
    // Status indicator
    if (g_robotState.motors_enabled) {
        LCD_FillCircle(115, 10, 4, COLOR_GREEN);
    } else {
        LCD_DrawRect(111, 6, 8, 8, COLOR_RED);
    }
    
    // === MOTOR SPEEDS ===
    LCD_DrawString(5, 25, "Motors:", COLOR_CYAN);
    
    // Left motor
    snprintf(str, 32, "L:%5d", g_robotState.left_motor_speed);
    LCD_DrawString(5, 35, str, COLOR_WHITE);
    uint8_t left_bar = (abs(g_robotState.left_motor_speed) * 80) / MAX_MOTOR_SPEED;
    uint16_t left_color = g_robotState.left_motor_speed >= 0 ? COLOR_GREEN : COLOR_RED;
    LCD_FillRect(45, 37, left_bar, 6, left_color);
    
    // Right motor
    snprintf(str, 32, "R:%5d", g_robotState.right_motor_speed);
    LCD_DrawString(5, 47, str, COLOR_WHITE);
    uint8_t right_bar = (abs(g_robotState.right_motor_speed) * 80) / MAX_MOTOR_SPEED;
    uint16_t right_color = g_robotState.right_motor_speed >= 0 ? COLOR_GREEN : COLOR_RED;
    LCD_FillRect(45, 49, right_bar, 6, right_color);
    
    // === DISTANCE SENSORS ===
    LCD_DrawString(5, 65, "Distance:", COLOR_CYAN);
    
    snprintf(str, 32, "Front: %3d cm", g_robotState.distance_front_cm);
    uint16_t front_color = g_robotState.distance_front_cm < DISTANCE_OBSTACLE_CM ? 
                          COLOR_RED : COLOR_WHITE;
    LCD_DrawString(10, 80, str, front_color);
    
    snprintf(str, 32, "Back:  %3d cm", g_robotState.distance_back_cm);
    LCD_DrawString(10, 92, str, COLOR_WHITE);
    
    // === BATTERY ===
    LCD_DrawString(5, 110, "Battery:", COLOR_CYAN);
    snprintf(str, 32, "%.1fV", g_robotState.battery_voltage);
    uint16_t batt_color = g_robotState.battery_voltage < BATTERY_MIN_VOLTAGE ? 
                         COLOR_RED : COLOR_GREEN;
    LCD_DrawString(10, 122, str, batt_color);
    
    // Battery bar
    LCD_DrawRect(4, 135, 120, 12, COLOR_WHITE);
    float batt_percent = (g_robotState.battery_voltage - BATTERY_MIN_VOLTAGE) / 
                        (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);
    batt_percent = CLAMP(batt_percent, 0.0f, 1.0f);
    uint8_t batt_width = (uint8_t)(batt_percent * 118);
    LCD_FillRect(5, 136, batt_width, 10, batt_color);
    
    // === STATUS MESSAGE ===
    if (!g_robotState.motors_enabled) {
        LCD_DrawString(25, 155, "** STOPPED **", COLOR_RED);
    } else if (g_robotState.autonomous_mode) {
        LCD_DrawString(20, 155, "[AUTONOMOUS]", COLOR_GREEN);
    } else {
        LCD_DrawString(30, 155, "[MANUAL]", COLOR_BLUE);
    }
}
```

---

### 6. REPLACE Interrupt Handlers

**REPLACE:**
```c
// Timer for audio sample generation
void TIMG7_IRQHandler(void) {
    switch (DL_Timer_getPendingInterrupt(TIMER_SAMPLE_INST)) {
        case DL_TIMER_IIDX_ZERO:
            Generate_Audio_Sample();
            break;
        default:
            break;
    }
}
```

**WITH:**
```c
// Timer for sensor updates (10 Hz)
void TIMG7_IRQHandler(void) {
    switch (DL_Timer_getPendingInterrupt(TIMER_DISTANCE_INST)) {
        case DL_TIMER_IIDX_ZERO:
            // Trigger sensor update in main loop
            static uint8_t sensor_counter = 0;
            if (++sensor_counter >= (MOTOR_UPDATE_HZ / SENSOR_UPDATE_HZ)) {
                sensor_counter = 0;
                // Set flag for main loop to read sensors
                // (don't do lengthy sensor reading in ISR)
            }
            break;
        default:
            break;
    }
}
```

**REPLACE:**
```c
// ADC for joystick
void ADC0_IRQHandler(void) {
    switch (DL_ADC12_getPendingInterrupt(ADC_MIC_JOY_INST)) {
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            g_robotState.joy_y = DL_ADC12_getMemResult(...);
            break;
        case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
            g_robotState.joy_x = DL_ADC12_getMemResult(...);
            break;
        default:
            break;
    }
}
```

**ADD:**
```c
// ADC for battery monitoring
void ADC1_IRQHandler(void) {
    switch (DL_ADC12_getPendingInterrupt(ADC_BATTERY_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            uint16_t adc_value = DL_ADC12_getMemResult(ADC_BATTERY_INST, DL_ADC12_MEM_IDX_0);
            // Convert ADC reading to voltage
            // Voltage divider: Vbatt -> 10kΩ -> ADC -> 10kΩ -> GND
            // ADC reads Vbatt/2, then we multiply back
            g_robotState.battery_voltage = (adc_value * 3.3f * BATTERY_DIVIDER_RATIO) / 4095.0f;
            break;
        default:
            break;
    }
}
```

**KEEP:**
```c
// GPIO buttons (unchanged)
void GPIOA_IRQHandler(void) {
    uint32_t status = DL_GPIO_getEnabledInterruptStatus(GPIOA, ...);
    
    if (status & GPIO_BUTTONS_S1_PIN) {
        g_robotState.btn_s1 = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_S1_PIN);
    }
    
    if (status & GPIO_BUTTONS_S2_PIN) {
        g_robotState.btn_s2 = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_S2_PIN);
    }
    
    if (status & GPIO_BUTTONS_JOY_SEL_PIN) {
        g_robotState.joy_pressed = true;
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_BUTTONS_JOY_SEL_PIN);
    }
}
```

---

### 7. UPDATE Main Loop

**REPLACE:**
```c
int main(void) {
    // Initialize system
    SYSCFG_DL_init();
    
    // Enable interrupts
    NVIC_EnableIRQ(TIMG7_INT_IRQn);
    NVIC_EnableIRQ(ADC0_INT_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    __enable_irq();
    
    // Initialize LCD
    LCD_Init();
    LCD_Clear(COLOR_BLACK);
    
    // Splash screen
    LCD_DrawString(20, 50, "MSPM0G3507", COLOR_CYAN);
    LCD_DrawString(15, 70, "Synthesizer", COLOR_WHITE);
    LCD_DrawString(35, 90, "Phase 1+2", COLOR_YELLOW);
    delay_ms(2000);
    
    // Initialize audio
    Update_Phase_Increment();
    
    // Start ADC
    DL_ADC12_startConversion(ADC_MIC_JOY_INST);
    
    // Initial display
    Update_Display();
    
    // Main loop
    while (1) {
        Process_Input();
        
        static uint32_t last_display = 0;
        uint32_t now = DL_Timer_getTimerCount(TIMER_SAMPLE_INST);
        
        if ((now - last_display) > (SYSCLK_FREQUENCY / 10)) {
            last_display = now;
            
            if (g_synthState.display_update_needed) {
                Update_Display();
                g_synthState.display_update_needed = false;
            }
        }
    }
}
```

**WITH:**
```c
int main(void) {
    // Initialize system
    SYSCFG_DL_init();
    
    // Enable interrupts
    NVIC_EnableIRQ(TIMG7_INT_IRQn);          // Timer for updates
    NVIC_EnableIRQ(ADC0_INT_IRQn);           // Joystick ADC
    NVIC_EnableIRQ(ADC1_INT_IRQn);           // Battery ADC
    NVIC_EnableIRQ(GPIOA_INT_IRQn);          // Buttons
    __enable_irq();
    
    // Initialize LCD
    LCD_Init();
    LCD_Clear(COLOR_BLACK);
    
    // Splash screen
    LCD_DrawString(15, 50, "MSPM0G3507", COLOR_CYAN);
    LCD_DrawString(5, 70, "Robot Controller", COLOR_WHITE);
    LCD_DrawString(25, 90, "Phase 1-3", COLOR_YELLOW);
    LCD_DrawString(10, 110, "Press S2 to START", COLOR_GREEN);
    delay_ms(3000);
    
    // Initialize robot state
    g_robotState.motors_enabled = false;
    g_robotState.autonomous_mode = false;
    g_robotState.motor_mode = MOTOR_STOP;
    g_robotState.left_motor_speed = 0;
    g_robotState.right_motor_speed = 0;
    g_robotState.distance_front_cm = 0;
    g_robotState.distance_back_cm = 0;
    g_robotState.battery_voltage = 7.4f;
    
    // Start motors at zero speed
    Set_Motor_Speeds(0, 0);
    
    // Start ADCs
    DL_ADC12_startConversion(ADC_JOY_INST);
    DL_ADC12_startConversion(ADC_BATTERY_INST);
    
    // Initial display
    Update_Display();
    
    // Set LED to blue (manual mode)
    DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_BLUE_PIN);
    
    // Main loop
    while (1) {
        // Process input (motor control)
        Process_Input();
        
        // Update sensors periodically
        static uint32_t last_sensor = 0;
        uint32_t now = DL_Timer_getTimerCount(TIMER_DISTANCE_INST);
        
        if ((now - last_sensor) > (SYSCLK_FREQUENCY / SENSOR_UPDATE_HZ)) {
            last_sensor = now;
            Update_Sensors();
        }
        
        // Update display periodically
        static uint32_t last_display = 0;
        if ((now - last_display) > (SYSCLK_FREQUENCY / DISPLAY_UPDATE_HZ)) {
            last_display = now;
            
            if (g_robotState.display_update_needed) {
                Update_Display();
                g_robotState.display_update_needed = false;
            }
        }
    }
}
```

---

## 🔧 SysConfig Changes

### 1. Timer Configuration

**REMOVE:**
```
PWM_AUDIO (TIMG0):
- Single PWM output for audio
- 12-bit resolution (4096 levels)
- Update rate: varies with frequency
```

**ADD:**
```
MOTOR_LEFT (TIMG0):
- Channel 0: Left Motor Forward
- Channel 1: Left Motor Reverse
- 12-bit resolution (4096 levels)
- Frequency: 20 kHz (ultrasonic, quiet)

MOTOR_RIGHT (TIMG1):
- Channel 0: Right Motor Forward
- Channel 1: Right Motor Reverse
- 12-bit resolution (4096 levels)
- Frequency: 20 kHz
```

**MODIFY:**
```
TIMER_SAMPLE (TIMG7) → TIMER_DISTANCE (TIMG7):
- Change: 8 kHz → 50 Hz (motor update rate)
- Period: 125 μs → 20 ms
- Keep: Same interrupt structure
```

### 2. ADC Configuration

**KEEP:**
```
ADC_MIC_JOY (ADC0):
- MEM0: Microphone → DELETE (not needed)
- MEM1: Joystick Y → KEEP
- MEM2: Joystick X → KEEP
```

**MODIFY:**
```
ADC_ACCEL (ADC1) → ADC_BATTERY (ADC1):
- Remove: All accelerometer channels
- Add: MEM0 - Battery voltage (PA.24 / ADC1_CH0)
- Reference: VDDA (3.3V)
- Sampling: Continuous, 100 Hz
```

### 3. GPIO Configuration

**ADD Outputs (Motor Control):**
```
GPIOB:
- PB.10: Motor Left Forward (TIMG0_CCP0)
- PB.11: Motor Left Reverse (TIMG0_CCP1)
- PB.12: Motor Right Forward (TIMG1_CCP0)
- PB.13: Motor Right Reverse (TIMG1_CCP1)
```

**ADD Outputs (Distance Sensors):**
```
GPIOA:
- PA.10: Front Trigger (GPIO Output, Push-Pull)
- PA.12: Back Trigger (GPIO Output, Push-Pull)
```

**ADD Inputs (Distance Sensors):**
```
GPIOA:
- PA.11: Front Echo (GPIO Input, Pull-Down)
- PA.13: Back Echo (GPIO Input, Pull-Down)
```

**ADD Input (Battery Monitor):**
```
GPIOA:
- PA.24: Battery Voltage (ADC1_CH0, Analog Input)
```

**KEEP (From Synthesizer):**
```
All LCD pins (SPI1)
All button pins (S1, S2, JOY_SEL)
All RGB LED pins
All joystick ADC pins
```

### 4. SPI Configuration

**NO CHANGES** - LCD driver stays the same

### 5. I2C Configuration

**OPTIONAL:**
- Can remove I2C_SENSORS if not using accelerometer
- Or keep for future Phase 3 expansion (IMU, sensors)

---

## 📦 Hardware Setup Guide

### Power Distribution

```
[2S LiPo Battery 7.4V]
        |
        +--- [L298N Vin] --------> Motors (6V)
        |
        +--- [Voltage Divider] --> PA.24 (ADC, max 3.3V)
        |       (10kΩ + 10kΩ)
        |
        +--- [Buck Converter] ---> MSPM0 Vin (3.3V)
                (or 3.3V LDO)
```

### L298N H-Bridge Connections

```
L298N Module          MSPM0G3507
─────────────────     ─────────────
IN1 (Left Fwd)  <---- PB.10 (PWM)
IN2 (Left Rev)  <---- PB.11 (PWM)
IN3 (Right Fwd) <---- PB.12 (PWM)
IN4 (Right Rev) <---- PB.13 (PWM)
ENA (Left En)   <---- 3.3V or PWM
ENB (Right En)  <---- 3.3V or PWM

OUT1 & OUT2     ----> Left Motor
OUT3 & OUT4     ----> Right Motor

+12V            <---- Battery +7.4V
GND             <---- Battery GND + MSPM0 GND
+5V             ----> (Optional 5V output)
```

**Important:** Connect ALL grounds together (Battery, L298N, MSPM0)!

### HC-SR04 Ultrasonic Sensors

```
Front Sensor          MSPM0G3507
─────────────────     ─────────────
VCC             <---- 3.3V or 5V
Trig            <---- PA.10 (GPIO Out)
Echo            ----> PA.11 (GPIO In)
GND             <---- GND

Back Sensor           MSPM0G3507
─────────────────     ─────────────
VCC             <---- 3.3V or 5V
Trig            <---- PA.12 (GPIO Out)
Echo            ----> PA.13 (GPIO In)
GND             <---- GND
```

### Battery Voltage Divider

```
Battery+ (7.4V)
      |
     [10kΩ]
      |
      +----------> PA.24 (ADC1_CH0)
      |
     [10kΩ]
      |
    GND

Output voltage = Vbatt / 2
For 2S LiPo (6.0V - 8.4V):
  ADC reads: 3.0V - 4.2V (safe for 3.3V ADC)
```

---

## 🧪 Testing Procedure

### Phase 1: Motor Control (Hardware Test)

1. **Power Test:**
   ```
   - Connect battery (MOTORS OFF)
   - Verify MSPM0 powers up
   - Check 3.3V rail with multimeter
   - Verify LCD displays splash screen
   ```

2. **Motor Test (No load):**
   ```
   - Enable motors (press S2)
   - Move joystick forward slowly
   - Both wheels should spin forward
   - Move joystick backward
   - Both wheels should spin backward
   ```

3. **Steering Test:**
   ```
   - Move joystick left
   - Left wheel slower/stopped, right wheel forward
   - Move joystick right
   - Right wheel slower/stopped, left wheel forward
   ```

4. **Emergency Stop:**
   ```
   - While motors running, press S2
   - Motors should stop immediately
   - Red LED should light up
   ```

### Phase 2: Sensor Dashboard

1. **Distance Sensor Test:**
   ```
   - Enable motors
   - Place hand 30cm in front of sensor
   - LCD should show "Front: ~30 cm"
   - Move hand closer
   - Value should decrease
   ```

2. **Battery Monitor Test:**
   ```
   - Check LCD battery voltage
   - Compare with multimeter on battery
   - Should be within 0.2V
   - Test with different battery charge levels
   ```

3. **Dashboard Update Test:**
   ```
   - Move joystick
   - Motor speed bars should update immediately
   - Distance values update every 100ms
   - Battery voltage updates continuously
   ```

### Phase 3: Autonomous Mode

1. **Obstacle Avoidance:**
   ```
   - Press S1 to enable autonomous
   - LED should turn green
   - Robot drives forward
   - Place obstacle <20cm ahead
   - Robot should: stop → reverse → turn → continue
   ```

2. **Mode Switching:**
   ```
   - While in autonomous, press S1
   - LED turns blue (manual mode)
   - Joystick control should work
   - Press S1 again
   - Returns to autonomous (green LED)
   ```

3. **Safety Systems:**
   ```
   - While running, press S2 (emergency stop)
   - Motors stop, red LED
   - Automatic stop if obstacle <10cm
   - Low battery warning (<6.5V)
   ```

---

## 🐛 Troubleshooting

### Problem: Motors don't spin

**Checks:**
- [ ] Battery voltage >6V?
- [ ] L298N ENA/ENB pins high?
- [ ] PWM signals present on IN1-IN4? (use oscilloscope)
- [ ] Motor wires connected correctly?
- [ ] `motors_enabled = true` in code?

### Problem: Motors spin backward

**Fix:**
- Swap motor wires (OUT1↔OUT2 or OUT3↔OUT4)
- Or invert PWM logic in code

### Problem: Distance sensor always reads 0

**Checks:**
- [ ] Sensor powered (VCC = 3.3V or 5V)?
- [ ] Trigger pulse generated? (oscilloscope on Trig pin)
- [ ] Echo pulse received? (oscilloscope on Echo pin)
- [ ] Timeout value correct? (30ms for 5m range)
- [ ] Object in range (2cm - 400cm)?

### Problem: Battery voltage reading incorrect

**Checks:**
- [ ] Voltage divider: exactly 10kΩ + 10kΩ?
- [ ] ADC reference = VDDA (3.3V)?
- [ ] Formula correct: `Vadc = Vbatt / 2`?
- [ ] ADC bit depth: 12-bit (4095 max)?

### Problem: Joystick doesn't control robot

**Checks:**
- [ ] ADC reading joystick? (check `joy_x`, `joy_y` values)
- [ ] Deadzone too large? (reduce `JOY_DEADZONE`)
- [ ] `autonomous_mode = false`?
- [ ] `motors_enabled = true`?

### Problem: LCD doesn't update

**Checks:**
- [ ] `display_update_needed` flag set?
- [ ] Display update rate too slow? (increase `DISPLAY_UPDATE_HZ`)
- [ ] SPI communication working? (test with static image)

---

## 📚 Additional Resources

### Datasheets
- **MSPM0G3507:** [Link to TI datasheet]
- **L298N H-Bridge:** [Link to datasheet]
- **HC-SR04 Ultrasonic:** [Link to datasheet]
- **ST7735 LCD:** [Link to datasheet]

### TI Resources
- **MSPM0 SDK:** C:\ti\mspm0_sdk_2_08_00_04\
- **MSPM0 Examples:** SDK → examples → nortos
- **MSPM0 Forum:** e2e.ti.com

### Code Examples
- **PWM Motor Control:** SDK → timerg → pwm
- **ADC Battery Monitor:** SDK → adc12 → voltage_monitor
- **GPIO Input Capture:** SDK → gpio → input_capture

---

## 🎯 Summary Checklist

### Code Changes
- [ ] Update `main.h` with `RobotState_t`
- [ ] Replace audio functions with motor control
- [ ] Add sensor reading functions
- [ ] Update input processing (joystick → motors)
- [ ] Replace display with dashboard
- [ ] Update interrupt handlers
- [ ] Modify main loop

### Hardware Changes
- [ ] Add L298N H-bridge module
- [ ] Connect 2x DC motors
- [ ] Add 2x HC-SR04 sensors
- [ ] Create battery voltage divider
- [ ] Wire power distribution
- [ ] Connect all grounds

### SysConfig Changes
- [ ] Configure TIMG0/1 for motor PWM
- [ ] Update TIMG7 period (8kHz → 50Hz)
- [ ] Remove accelerometer ADC channels
- [ ] Add battery voltage ADC channel
- [ ] Add GPIO for distance sensors
- [ ] Add GPIO for motor control

### Testing
- [ ] Power system test
- [ ] Motor control test (manual)
- [ ] Distance sensor test
- [ ] Battery monitor test
- [ ] Dashboard display test
- [ ] Autonomous mode test
- [ ] Emergency stop test

---

## 🚀 Next Steps

After completing this migration:

1. **Test thoroughly** - Use checklist above
2. **Calibrate sensors** - Distance, battery voltage
3. **Tune parameters** - Motor speeds, thresholds
4. **Add features** - Line following, PID control
5. **Share project** - TI E2E forum, GitHub

**Good luck with your robot! 🤖**

---

*Generated for MSPM0G3507 Robot Controller Migration*  
*Based on Audio Synthesizer Phase 1+2 Project*  
*Version 1.0 - Ready for Phase 3 expansion*
