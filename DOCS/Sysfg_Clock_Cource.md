# 🎓 SYSCONFIG KURS - Clock System for MSPM0G3507

## 📚 INNHOLDSFORTEGNELSE

1. [Clock Hierarchy Overview](#clock-hierarchy)
2. [Clock Sources (SYSOSC, LFCLK, SYSPLL)](#clock-sources)
3. [Clock Tree og Derivation](#clock-tree)
4. [Peripheral Clock Assignment](#peripheral-clocks)
5. [PWM Timer Clock Deep Dive](#pwm-clocks)
6. [ADC Clock Configuration](#adc-clocks)
7. [SPI Clock Configuration](#spi-clocks)
8. [Praktiske Eksempler](#examples)
9. [Common Pitfalls](#pitfalls)

---

## 🌳 1. CLOCK HIERARCHY OVERVIEW {#clock-hierarchy}

MSPM0G3507 har en **hierarkisk clock struktur**:

```
                    ┌─────────────┐
                    │   SYSOSC    │ 32 MHz (internal oscillator)
                    │  (default)  │
                    └──────┬──────┘
                           │
              ┌────────────┴────────────┐
              │                         │
         ┌────▼────┐              ┌────▼────┐
         │ SYSPLL  │              │  MCLK   │ Main CPU clock
         │(optional)│             └────┬────┘
         └────┬────┘                   │
              │                  ┌─────┴─────┐
              │                  │           │
         ┌────▼────┐        ┌───▼───┐  ┌───▼───┐
         │  MFCLK  │        │ ULPCLK│  │ BUSCLK│
         │ 4 MHz   │        │ 32kHz │  │  =MCLK│
         └────┬────┘        └───┬───┘  └───┬───┘
              │                 │           │
        ┌─────┴─────┐          │      ┌────▼────┐
        │           │          │      │Peripherals│
    ┌───▼───┐  ┌───▼───┐     │      │ (ADC, SPI,│
    │ PWM   │  │ Timer │     │      │  GPIO....) │
    │ Timers│  │ G     │     │      └───────────┘
    └───────┘  └───────┘     │
                              │
                         ┌────▼────┐
                         │  LFCLK  │ Low freq peripherals
                         │  32kHz  │
                         └─────────┘
```

---

## ⚡ 2. CLOCK SOURCES {#clock-sources}

### **2.1 SYSOSC (System Oscillator)**

**Hva:** Internal RC oscillator  
**Frekvens:** 32 MHz (factory calibrated)  
**Bruk:** Default clock source for alt  
**Power:** Low power mode supported

```javascript
// I SysConfig SYSCTL module:
SYSCTL.forceDefaultClkConfig = true;  // Bruker SYSOSC
```

**Fordeler:**
- ✅ Ingen eksterne komponenter nødvendig
- ✅ Rask oppstart
- ✅ God nok for de fleste applikasjoner

**Ulemper:**
- ⚠️ Mindre nøyaktig enn crystal oscillator
- ⚠️ Kan driftes av temperatur

---

### **2.2 SYSPLL (System Phase-Locked Loop)**

**Hva:** PLL som multipliserer clock frekvens  
**Input:** SYSOSC (32 MHz)  
**Output:** Opptil 80 MHz  
**Bruk:** Når du trenger høyere clock speeds

```javascript
// Din .syscfg har dette enabled:
SYSCTL.SYSPLL_CLK0En = true;   // Enable PLL output 0
SYSCTL.SYSPLL_CLK1En = true;   // Enable PLL output 1
SYSCTL.SYSPLL_CLK2XEn = true;  // Enable PLL 2X output
SYSCTL.SYSPLL_CLK2XDiv = 9;    // Divider for CLK2X
```

**PLL Calculation:**
```
SYSPLL_CLK2X = SYSOSC × (QDIV / PDIV) × 2
             = 32 MHz × (default config)
             = Variable output

MCLK = SYSPLL_CLK2X / CLK2XDiv
     = SYSPLL_CLK2X / 9  (in your config)
```

**Fordeler:**
- ✅ Høyere performance
- ✅ Fleksibel clock generering

**Ulemper:**
- ⚠️ Høyere strømforbruk
- ⚠️ Lengre oppstartstid
- ⚠️ Må sjekke `SYSPLL_GOOD` interrupt

---

### **2.3 LFCLK (Low Frequency Clock)**

**Hva:** 32 kHz oscillator  
**Bruk:** Low-power timers, RTC, WDT  
**Source:** Internal LFOSC eller external crystal

```javascript
SYSCTL.EXCLKSource = "ULPCLK";  // Ultra-low power clock
```

**Når brukes:**
- ⏰ Real-time clock (RTC)
- ⏱️ Watchdog timer
- 💤 Low-power modes

---

## 🌲 3. CLOCK TREE OG DERIVATION {#clock-tree}

### **3.1 MCLK (Main Clock)**

**Hva:** CPU core clock  
**Default:** 32 MHz (SYSOSC)  
**Max:** 80 MHz (med SYSPLL)

```javascript
// Din konfigurasjon:
SYSCTL.MCLKSource = "HSCLK";  // High-speed clock (SYSOSC eller SYSPLL)
```

**MCLK drives:**
- ✅ CPU core
- ✅ Flash memory controller
- ✅ DMA
- ✅ Debug interface

---

### **3.2 BUSCLK (Bus Clock)**

**Hva:** Peripheral bus clock  
**Relation:** BUSCLK = MCLK (typisk)  
**Bruk:** De fleste peripherals

```javascript
// BUSCLK er automatisk = MCLK
// Brukes av: ADC, SPI, I2C, GPIO, etc.
```

**BUSCLK frequency påvirker:**
- 🔄 ADC sample rate (indirectly)
- 🔄 SPI baud rate (max)
- 🔄 Timer resolution

---

### **3.3 MFCLK (Medium Frequency Clock)**

**Hva:** Fixed 4 MHz clock  
**Source:** Derived fra SYSOSC  
**Bruk:** Spesifikke timers

```javascript
SYSCTL.MFCLKEn = true;  // Enable MFCLK
```

**Brukes av:**
- ⏱️ TIMG0 (PWM_AUDIO i ditt prosjekt)
- ⏱️ Andre general-purpose timers

---

### **3.4 ULPCLK (Ultra-Low Power Clock)**

**Hva:** 32 kHz low-power clock  
**Bruk:** Low-power peripherals

```javascript
SYSCTL.EXCLKSource = "ULPCLK";
SYSCTL.UDIV = "2";  // Divider
```

---

## ⏰ 4. PERIPHERAL CLOCK ASSIGNMENT {#peripheral-clocks}

Hver peripheral får sin clock fra en bestemt source:

### **Peripheral Clock Sources:**

| Peripheral | Clock Source | Frequency | Configured By |
|------------|--------------|-----------|---------------|
| **CPU Core** | MCLK | 32 MHz | SYSCTL.MCLKSource |
| **ADC0/ADC1** | BUSCLK | 32 MHz | Automatic (= MCLK) |
| **SPI0/SPI1** | BUSCLK | 32 MHz | Automatic (= MCLK) |
| **I2C0/I2C1** | BUSCLK | 32 MHz | Automatic (= MCLK) |
| **TIMG0** (PWM) | MFCLK | 4 MHz | PWM1.clockSource |
| **TIMG7** (Sample) | BUSCLK | 32 MHz | TIMER1.clockSource |
| **GPIO** | BUSCLK | 32 MHz | Automatic |
| **VREF** | BUSCLK | 32 MHz | VREF.advClkSrc |

---

## 🎵 5. PWM TIMER CLOCK DEEP DIVE {#pwm-clocks}

### **5.1 PWM Clock Source Selection**

I SysConfig, under PWM module:

```javascript
PWM1.$name = "PWM_AUDIO";
PWM1.peripheral.$assign = "TIMG0";  // Timer Group 0
// Clock source er implicit: MFCLK (4 MHz)
```

**Hvorfor MFCLK (4 MHz)?**
- ✅ Lavere frekvens → finere duty cycle resolution
- ✅ Mindre power consumption
- ✅ God nok for audio (ikke trenger 32 MHz)

---

### **5.2 PWM Frequency Calculation**

**Formula:**
```
PWM_freq = Clock_freq / (timerCount + 1)
```

**Ditt prosjekt:**
```javascript
PWM1.timerCount = 4095;  // 12-bit counter
Clock_freq = 4 MHz (MFCLK)

PWM_freq = 4,000,000 / (4095 + 1)
         = 4,000,000 / 4096
         = 976.56 Hz
```

**Dette betyr:**
- 🎵 PWM carrier frequency: **~977 Hz**
- 🎵 Dette er for høyt for direkte audio!
- 🎵 Du må bruke low-pass filter for å få audio ut

---

### **5.3 Duty Cycle Resolution**

```
Resolution = timerCount + 1
           = 4096 levels (12-bit)

Min step = 1 / 4096 = 0.024% duty cycle change
```

**For audio synthesis:**
```c
// Duty cycle range: 0 - 4095
uint16_t duty = 2048 + audio_sample;  // 2048 = center (50%)

DL_TimerG_setCaptureCompareValue(PWM_AUDIO_INST, duty, DL_TIMER_CC_0_INDEX);
```

---

### **5.4 PWM Timer Periode Konfigurasjon**

```javascript
// Hvis du vil endre PWM frequency:
PWM1.timerCount = X;  // Periode i clock cycles

// Nye PWM_freq = Clock_freq / (X + 1)
```

**Eksempler:**

| timerCount | PWM Frequency | Resolution | Use Case |
|------------|---------------|------------|----------|
| 255 | 15.625 kHz | 8-bit | Fast switching |
| 1023 | 3.906 kHz | 10-bit | Servo control |
| 4095 | 976 Hz | 12-bit | **Audio (ditt prosjekt)** |
| 9999 | 400 Hz | High | Low frequency PWM |

---

## 🔬 6. ADC CLOCK CONFIGURATION {#adc-clocks}

### **6.1 ADC Clock Source**

```javascript
// ADC bruker automatisk BUSCLK (= MCLK = 32 MHz)
ADC121.peripheral.$assign = "ADC0";
// Clock source: BUSCLK (implicit)
```

**Intern ADC clock derivation:**
```
ADC_CLK = BUSCLK / sampClkDiv

Din konfigurasjon:
├─ ADC0: sampClkDiv = 1 (default)
│        ADC_CLK = 32 MHz / 1 = 32 MHz
│
└─ ADC1: sampClkDiv = DL_ADC12_CLOCK_DIVIDE_8
         ADC_CLK = 32 MHz / 8 = 4 MHz
```

---

### **6.2 ADC Sample Time**

```javascript
ADC121.sampleTime0 = "125 us";  // Sample window
```

**Hva betyr dette?**

```
Sample time = 125 µs
Clock period = 1 / 32 MHz = 31.25 ns

Sample clock cycles = 125 µs / 31.25 ns
                    = 4000 cycles
```

**Total conversion time:**
```
T_conversion = T_sample + T_conversion_fixed
             = 125 µs + ~0.5 µs
             = ~125.5 µs per sample
```

**Max sample rate:**
```
Sample_rate = 1 / T_conversion
            = 1 / 125.5 µs
            = ~7968 Hz
```

---

### **6.3 ADC Sequence Mode**

```javascript
ADC121.samplingOperationMode = "sequence";
ADC121.endAdd = 2;  // Sample 3 channels (0, 1, 2)
```

**Total conversion time for all channels:**
```
T_total = T_conversion × (endAdd + 1)
        = 125.5 µs × 3
        = ~376.5 µs

Effective sample rate per channel:
= 1 / 376.5 µs
= ~2656 Hz per channel
```

---

### **6.4 ADC Clock Division (ADC1)**

```javascript
ADC122.sampClkDiv = "DL_ADC12_CLOCK_DIVIDE_8";
```

**Hvorfor bruke clock divider?**

- ✅ Lavere clock → mindre noise i analog måling
- ✅ Bedre for accelerometer (trenger ikke høy speed)
- ✅ Lower power consumption

**ADC1 sample rate:**
```
ADC_CLK = 32 MHz / 8 = 4 MHz
Sample time = 125 µs
Clock cycles = 125 µs × 4 MHz = 500 cycles

Max sample rate = 1 / 125.5 µs ≈ 8 kHz (per channel)
```

---

## 📡 7. SPI CLOCK CONFIGURATION {#spi-clocks}

### **7.1 SPI Clock Source**

```javascript
SPI1.peripheral.$assign = "SPI1";
// Clock source: BUSCLK (32 MHz) - implicit
```

---

### **7.2 SPI Baud Rate (Bit Rate)**

```javascript
SPI1.targetBitRate = 10000000;  // 10 MHz
```

**Hvordan fungerer dette?**

```
SPI_CLK = BUSCLK / divider

For å oppnå 10 MHz fra 32 MHz:
divider = 32 MHz / 10 MHz = 3.2

SysConfig velger nærmeste divider: 4
Actual SPI_CLK = 32 MHz / 4 = 8 MHz

Derfor:
Requested: 10 MHz
Actual: 8 MHz ✅ (nærmeste mulige)
```

---

### **7.3 SPI Clock Divider Tabell**

| Target Baud | BUSCLK (32 MHz) | Divider | Actual Baud | Error |
|-------------|-----------------|---------|-------------|-------|
| 1 MHz | 32 MHz | 32 | 1 MHz | 0% |
| 2 MHz | 32 MHz | 16 | 2 MHz | 0% |
| 4 MHz | 32 MHz | 8 | 4 MHz | 0% |
| 8 MHz | 32 MHz | 4 | 8 MHz | 0% |
| **10 MHz** | 32 MHz | 3.2 → 4 | **8 MHz** | -20% |
| 16 MHz | 32 MHz | 2 | 16 MHz | 0% |

**Konklusjon:** 10 MHz er ikke perfekt mulig fra 32 MHz BUSCLK!

---

### **7.4 Hvorfor SPI Clock Speed Matter**

```
For ST7735 LCD (din setup):
├─ Max SPI clock: 15 MHz (datasheet)
├─ Din setting: 10 MHz (→ actual 8 MHz)
└─ Result: Safe margin ✅

Hvis du øker til 16 MHz:
├─ Actual: 16 MHz
├─ Close to max (15 MHz)
└─ Risk: Timing violations ⚠️
```

**Anbefaling for LCD:**
- ✅ 8 MHz (divider 4) - Safe, good speed
- ⚠️ 16 MHz (divider 2) - Risky, too fast
- ❌ 32 MHz (divider 1) - Will NOT work

---

## 💡 8. PRAKTISKE EKSEMPLER {#examples}

### **Eksempel 1: Endre PWM Frequency for Audio**

**Scenario:** Du vil ha 8 kHz sample rate for audio.

**Løsning:**

```javascript
// I SysConfig PWM module:
PWM1.timerCount = 499;  // New period

// Beregning:
PWM_freq = 4 MHz / (499 + 1)
         = 4 MHz / 500
         = 8000 Hz
         = 8 kHz ✅

Resolution = 500 levels (9-bit)
```

**Trade-off:**
- ✅ Perfekt sample rate for audio
- ⚠️ Lavere resolution (9-bit vs 12-bit)
- ⚠️ Mindre smooth audio

---

### **Eksempel 2: Øke ADC Sample Rate**

**Scenario:** Du trenger 44.1 kHz audio sample rate.

**Løsning:**

```javascript
// I SysConfig ADC module:
ADC121.sampleTime0 = "1 us";  // Minimum sample time

// Beregning:
T_conversion = 1 µs + 0.5 µs = 1.5 µs
Sample_rate = 1 / 1.5 µs ≈ 666 kHz (max per channel)

For 3 channels:
Effective rate = 666 kHz / 3 ≈ 222 kHz per channel ✅
```

**Men:**
- ⚠️ Kortere sample time → mindre nøyaktig
- ⚠️ High-impedance sources trenger lengre sample time
- ✅ For audio: 125 µs er god balanse

---

### **Eksempel 3: Clock for High-Speed SPI**

**Scenario:** Du vil maksimere LCD refresh rate.

**Løsning:**

```javascript
// Øk SPI clock til max safe speed:
SPI1.targetBitRate = 16000000;  // 16 MHz

// Actual: 16 MHz (divider 2)
// LCD max: 15 MHz
// Margin: 1 MHz over spec ⚠️

// Anbefaling:
SPI1.targetBitRate = 8000000;  // 8 MHz (safer)
```

---

## ⚠️ 9. COMMON PITFALLS {#pitfalls}

### **Pitfall 1: Clock Mismatch**

**Problem:**
```javascript
// Du setter PWM timer til å bruke BUSCLK (32 MHz):
PWM1.timerCount = 4095;

// Men glemmer at MFCLK (4 MHz) er default!
// Actual PWM freq = 4 MHz / 4096 = 977 Hz ❌
// Forventet: 32 MHz / 4096 = 7812 Hz
```

**Løsning:** Sjekk alltid hvilken clock source peripheral bruker!

---

### **Pitfall 2: ADC Sample Rate vs. Interrupt Rate**

**Problem:**
```javascript
// ADC sample time: 125 µs
// ADC sequence: 3 channels
// Total time: 375 µs

// Men timer interrupt hver 125 µs:
TIMER1.timerPeriod = "125 us";

// Konflikt! ADC ikke ferdig før ny interrupt! ❌
```

**Løsning:** Timer period må være ≥ ADC total conversion time!

---

### **Pitfall 3: SPI Clock Too Fast**

**Problem:**
```javascript
SPI1.targetBitRate = 32000000;  // Max possible

// Men LCD max er 15 MHz!
// Result: Corrupted data på LCD ❌
```

**Løsning:** Alltid sjekk peripheral datasheet for max clock!

---

### **Pitfall 4: PWM Resolution vs. Frequency**

**Problem:**
```
Ønsker både høy frequency OG høy resolution:
PWM_freq = 100 kHz
Resolution = 12-bit (4096 levels)

Nødvendig clock:
Clock = PWM_freq × (2^resolution)
      = 100 kHz × 4096
      = 409.6 MHz ❌ (impossible!)
```

**Løsning:** Trade-off mellom frequency og resolution!

```
Med 4 MHz clock:
├─ High freq (100 kHz) → 6-bit resolution (64 levels)
└─ High resolution (12-bit) → Low freq (977 Hz)
```

---

## 📊 CLOCK CONFIGURATION SUMMARY FOR DITT PROSJEKT

```javascript
// SYSTEM CLOCKS
MCLK:    32 MHz  (SYSOSC → HSCLK)
BUSCLK:  32 MHz  (= MCLK)
MFCLK:   4 MHz   (Fixed, for PWM)
ULPCLK:  32 kHz  (Low power)

// PERIPHERAL CLOCKS
PWM_AUDIO (TIMG0):  4 MHz   → 977 Hz PWM (12-bit)
TIMER_SAMPLE (TIMG7): 32 MHz → 8 kHz interrupts
ADC0 (Joy/Mic):     32 MHz  → ~8 kHz sample rate
ADC1 (Accel):       4 MHz   → ~8 kHz sample rate
SPI_LCD (SPI1):     32 MHz  → 8 MHz actual baud
I2C0/I2C1:          32 MHz  → 100 kHz/400 kHz I2C speed
```

---

## ✅ QUICK REFERENCE CHEAT SHEET

### **Når skal jeg endre hva?**

| Ønske | Endre Dette | Effect |
|-------|-------------|--------|
| Raskere CPU | `SYSCTL.MCLKSource` → SYSPLL | Higher MCLK |
| Raskere ADC | `ADC.sampleTime0` → lavere | Faster conversion |
| Raskere SPI | `SPI.targetBitRate` → høyere | Faster LCD refresh |
| Finere PWM | `PWM.timerCount` → høyere | More resolution |
| Høyere PWM freq | `PWM.timerCount` → lavere | Less resolution |
| Spare strøm | `SYSCTL.powerPolicy` → STANDBY | Lower power modes |

---

## 🎓 QUIZ - Test Din Forståelse!

**Q1:** Hvis MCLK = 32 MHz, hva er max SPI baud rate?  
**A:** 16 MHz (divider 2), men sjekk peripheral max først!

**Q2:** PWM med timerCount = 999, MFCLK = 4 MHz. Hva er PWM freq?  
**A:** 4 MHz / 1000 = 4 kHz

**Q3:** ADC sample time = 10 µs, sequence = 4 channels. Total tid?  
**A:** ~40 µs (10 µs × 4)

**Q4:** Kan jeg få 10 MHz SPI fra 32 MHz BUSCLK?  
**A:** Nei, nærmeste er 8 MHz (divider 4) eller 16 MHz (divider 2)

**Q5:** Hvorfor bruker PWM_AUDIO 4 MHz i stedet for 32 MHz?  
**A:** For å få finere duty cycle resolution ved lav PWM frequency!

---

## 🔗 HVOR FINNE DETTE I SYSCFG?

```
SysConfig GUI:
├─ SYSCTL module
│  ├─ Clock Configuration
│  │  ├─ MCLK Source
│  │  ├─ PLL Settings
│  │  └─ Clock Enables
│  └─ Power Policy
│
├─ PWM module
│  ├─ Timer Instance (TIMG0/etc)
│  ├─ Timer Count (periode)
│  └─ PWM Mode
│
├─ ADC module
│  ├─ Clock Divider
│  ├─ Sample Time
│  └─ Sequence Configuration
│
└─ SPI module
   ├─ Target Bit Rate
   └─ Actual calculated baud
```

---

## 📚 FURTHER READING

- TI MSPM0G3507 Technical Reference Manual (TRM)
- MSPM0 SDK Clock Configuration Guide
- SysConfig User Guide

**Lykke til med clock configuration!** ⏰🎉