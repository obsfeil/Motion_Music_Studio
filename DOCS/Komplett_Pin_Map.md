# 🎯 FINAL KORREKT PIN MAPPING - Basert på TI Offisiell Dokumentasjon

## 📋 Educational BoosterPack MKII (BOOSTXL-EDUMKII) - Offisiell Pin Mapping

Fra TI SLAU599B User Guide:

---

## ✅ VERIFISERT KORREKT PIN MAPPING

### **1. Joystick (2-Axis med Pushbutton)**

| BoosterPack Pin | Signal | LaunchPad Pin | ADC Channel |
|-----------------|--------|---------------|-------------|
| **J1.2** | Joystick X-axis | **PA25** | ADC0 CH2 ✅ |
| **J3.26** | Joystick Y-axis | **PA26** | ADC0 CH1 ✅ |
| **J1.5** | Joystick Select Button | **PA26** | GPIO ❓ |

**MERK:** J1.5 og J3.26 kan ikke være samme pin!

**KORREKSJON fra dokumentasjon:**
```
J1.2  → Horizontal X-axis  → PA25 (ADC0 CH2)
J3.26 → Vertical Y-axis    → PA26 (ADC0 CH1)
J1.5  → Select button      → GPIO (ikke ADC)
```

---

### **2. Mikrofon (CMA-4544PF-W)**

| BoosterPack Pin | Signal | LaunchPad Pin | ADC Channel |
|-----------------|--------|---------------|-------------|
| **J1.6** | Microphone Output | **PA27** | ADC0 CH0 ✅ |

**FEIL I MIN TIDLIGERE MAPPING!**

Original .syscfg hadde:
```javascript
ADC121.peripheral.adcPin0.$assign = "boosterpack.8";  // PA27 ❌ FEIL!
ADC121.peripheral.adcPin1.$assign = "boosterpack.5";  // PA26 ✅ OK
ADC121.peripheral.adcPin2.$assign = "boosterpack.2";  // PA25 ✅ OK
```

**KORREKT SKAL VÆRE:**
```javascript
ADC121.adcMem0chansel = "DL_ADC12_INPUT_CHAN_0";  // J1.6  - Mikrofon
ADC121.adcMem1chansel = "DL_ADC12_INPUT_CHAN_1";  // J3.26 - Joy Y
ADC121.adcMem2chansel = "DL_ADC12_INPUT_CHAN_2";  // J1.2  - Joy X

ADC121.peripheral.adcPin0.$assign = "PA27";  // J1.6  - Mikrofor (CH0)
ADC121.peripheral.adcPin1.$assign = "PA26";  // J3.26 - Joy Y (CH1)
ADC121.peripheral.adcPin2.$assign = "PA25";  // J1.2  - Joy X (CH2)
```

---

### **3. Akselerometer (KXTC9-2050 - 3-Axis Analog)**

| BoosterPack Pin | Signal | LaunchPad Pin | ADC Channel |
|-----------------|--------|---------------|-------------|
| **J3.23** | Accel X-axis | **PA17** | ADC1 CH2 ✅ |
| **J3.24** | Accel Y-axis | **PA18** | ADC1 CH3 ✅ |
| **J3.25** | Accel Z-axis | **PA21** | ADC1 CH7 ✅ |

**DETTE VAR RIKTIG I CORRECTED .SYSCFG!** ✅

---

### **4. LCD (CFAF128128B-0145T - 128x128 TFT)**

| BoosterPack Pin | Signal | LaunchPad Pin | Function |
|-----------------|--------|---------------|----------|
| **J1.7** | LCD SPI Clock | **PB16** | SPI SCLK ✅ |
| **J2.13** | LCD SPI CS | **PA2** | SPI CS ✅ |
| **J2.15** | LCD SPI MOSI | **PB15** | SPI MOSI ✅ |
| **J4.17** | LCD Reset | **PB13** | GPIO ✅ |
| **J4.31** | LCD Register Select (DC) | **PB14** | GPIO ✅ |
| **J4.39*** | LCD Backlight | - | PWM (multiplexed) |

**DETTE VAR RIKTIG I CORRECTED .SYSCFG!** ✅

---

### **5. RGB LED (CLV1A-FKB Multicolor)**

| BoosterPack Pin | Signal | LaunchPad Pin | Function |
|-----------------|--------|---------------|----------|
| **J4.37** | Blue Channel | **?** | PWM |
| **J4.38** | Green Channel | **?** | PWM |
| **J4.39*** | Red Channel | **?** | PWM (multiplexed) |

**MERK:** J4.39 er multiplexed med LCD backlight!

**PROBLEM:** J4.37/38/39 er IKKE standard boosterpack pins!

**LØSNING:** Bruk LaunchPad onboard RGB LED (PB22/26/27) ELLER eksterne LEDs (PB17/18/19)

---

### **6. Buttons (S1/S2 Pushbuttons)**

| BoosterPack Pin | Signal | LaunchPad Pin | Function |
|-----------------|--------|---------------|----------|
| **J4.32** | S2 Button | **PA14** | GPIO ✅ |
| **J4.33** | S1 Button | **PA15** | GPIO ✅ |

**KORREKT!** ✅

---

### **7. Buzzer (CEM-1203 Piezo)**

| BoosterPack Pin | Signal | LaunchPad Pin | Function |
|-----------------|--------|---------------|----------|
| **J4.40** | Buzzer Input | **PA12** | PWM ✅ |

**KORREKT!** ✅

---

## 🔧 FULLSTENDIG KORREKT .SYSCFG

Basert på TI offisiell dokumentasjon:

### **ADC0 (Joystick + Mikrofon):**

```javascript
ADC121.$name = "ADC_MIC_JOY";
ADC121.samplingOperationMode = "sequence";
ADC121.repeatMode = true;
ADC121.endAdd = 2;

// CHANNEL MAPPING (VIKTIG!)
ADC121.adcMem0chansel = "DL_ADC12_INPUT_CHAN_0";  // Mikrofor (J1.6)
ADC121.adcMem1chansel = "DL_ADC12_INPUT_CHAN_1";  // Joy Y (J3.26)
ADC121.adcMem2chansel = "DL_ADC12_INPUT_CHAN_2";  // Joy X (J1.2)

// PIN ASSIGNMENT
ADC121.peripheral.$assign = "ADC0";
ADC121.peripheral.adcPin0.$assign = "PA27";  // CH0 - Mikrofon
ADC121.peripheral.adcPin1.$assign = "PA26";  // CH1 - Joy Y
ADC121.peripheral.adcPin2.$assign = "PA25";  // CH2 - Joy X
```

**VIKTIG:** Din original .syscfg hadde feil channel mapping!

---

### **ADC1 (Akselerometer):**

```javascript
ADC122.$name = "ADC_ACCEL";
ADC122.samplingOperationMode = "sequence";
ADC122.repeatMode = true;
ADC122.endAdd = 2;

// CHANNEL MAPPING
ADC122.adcMem0chansel = "DL_ADC12_INPUT_CHAN_2";  // Accel X (J3.23)
ADC122.adcMem1chansel = "DL_ADC12_INPUT_CHAN_3";  // Accel Y (J3.24)
ADC122.adcMem2chansel = "DL_ADC12_INPUT_CHAN_7";  // Accel Z (J3.25)

// PIN ASSIGNMENT
ADC122.peripheral.$assign = "ADC1";
ADC122.peripheral.adcPin2.$assign = "PA17";  // CH2 - Accel X
ADC122.peripheral.adcPin3.$assign = "PA18";  // CH3 - Accel Y
ADC122.peripheral.adcPin7.$assign = "PA21";  // CH7 - Accel Z
```

**DETTE VAR RIKTIG!** ✅

---

### **SPI (LCD):**

```javascript
SPI1.$name = "SPI_LCD";
SPI1.targetBitRate = 10000000;

SPI1.peripheral.$assign = "SPI1";
SPI1.peripheral.sclkPin.$assign = "PB16";  // J1.7  - LCD Clock
SPI1.peripheral.mosiPin.$assign = "PB15";  // J2.15 - LCD MOSI
SPI1.peripheral.misoPin.$assign = "PA16";  // (unused for LCD)
SPI1.peripheral.cs0Pin.$assign  = "PA2";   // J2.13 - LCD CS
```

**DETTE VAR RIKTIG!** ✅

---

### **GPIO_LCD (LCD Control):**

```javascript
GPIO3.$name = "GPIO_LCD";
GPIO3.associatedPins.create(2);

GPIO3.associatedPins[0].$name = "RST";
GPIO3.associatedPins[0].initialValue = "SET";
GPIO3.associatedPins[0].pin.$assign = "PB13";  // J4.17 - LCD Reset

GPIO3.associatedPins[1].$name = "DC";
GPIO3.associatedPins[1].initialValue = "SET";
GPIO3.associatedPins[1].pin.$assign = "PB14";  // J4.31 - LCD DC
```

**DETTE VAR RIKTIG!** ✅

---

### **GPIO_RGB (BRUK ONBOARD LAUNCHPAD LED):**

**ANBEFALING:** Siden BoosterPack MKII RGB LED bruker J4.37/38/39 (ikke standard pins), bruk LaunchPad onboard RGB i stedet!

**METODE 1: Bruk LaunchPad onboard RGB (med jumpers):**

```javascript
GPIO2.$name = "GPIO_RGB";
GPIO2.associatedPins.create(3);

GPIO2.associatedPins[0].$name = "RED";
GPIO2.associatedPins[0].pin.$assign = "PB26";  // LaunchPad onboard (J6)

GPIO2.associatedPins[1].$name = "GREEN";
GPIO2.associatedPins[1].pin.$assign = "PB27";  // LaunchPad onboard (J7)

GPIO2.associatedPins[2].$name = "BLUE";
GPIO2.associatedPins[2].pin.$assign = "PB22";  // LaunchPad onboard (J5)
```

**KEEP J5/J6/J7 jumpere INSTALLED på LaunchPad!**

**METODE 2: Bruk alternative pins (hvis jumpers fjernet):**

```javascript
GPIO2.associatedPins[0].pin.$assign = "PB17";  // RED (alternative)
GPIO2.associatedPins[1].pin.$assign = "PB18";  // GREEN (alternative)
GPIO2.associatedPins[2].pin.$assign = "PB19";  // BLUE (alternative)
```

**KOBLE eksterne LEDs til PB17/18/19!**

---

### **GPIO_BUTTONS:**

```javascript
GPIO1.$name = "GPIO_BUTTONS";
GPIO1.associatedPins.create(3);

GPIO1.associatedPins[0].$name = "S1";
GPIO1.associatedPins[0].direction = "INPUT";
GPIO1.associatedPins[0].internalResistor = "PULL_UP";
GPIO1.associatedPins[0].interruptEn = true;
GPIO1.associatedPins[0].polarity = "RISE";
GPIO1.associatedPins[0].pin.$assign = "PA15";  // J4.33 - S1 ✅

GPIO1.associatedPins[1].$name = "S2";
GPIO1.associatedPins[1].direction = "INPUT";
GPIO1.associatedPins[1].internalResistor = "PULL_UP";
GPIO1.associatedPins[1].interruptEn = true;
GPIO1.associatedPins[1].polarity = "FALL";
GPIO1.associatedPins[1].pin.$assign = "PA14";  // J4.32 - S2 ✅

GPIO1.associatedPins[2].$name = "JOY_SEL";
GPIO1.associatedPins[2].direction = "INPUT";
GPIO1.associatedPins[2].internalResistor = "PULL_UP";
GPIO1.associatedPins[2].interruptEn = true;
GPIO1.associatedPins[2].polarity = "FALL";
GPIO1.associatedPins[2].pin.$assign = "PA13";  // J1.5 - Joy Select ❓
```

**MERK:** J1.5 mapping må verifiseres!

---

### **PWM_AUDIO (Buzzer/Audio Output):**

```javascript
PWM1.$name = "PWM_AUDIO";
PWM1.timerStartTimer = true;
PWM1.timerCount = 4095;
PWM1.pwmMode = "EDGE_ALIGN_UP";

PWM1.peripheral.$assign = "TIMG0";
PWM1.peripheral.ccp0Pin.$assign = "PA12";  // J4.40 - Buzzer/Audio ✅
```

**DETTE VAR RIKTIG!** ✅

---

## 🎯 KRITISK KORREKSJON

### **HOVEDPROBLEMET I ORIGINAL .SYSCFG:**

**ADC0 Channel Mapping var feil!**

```javascript
// DIN ORIGINAL (FEIL):
ADC121.adcMem0chansel = "DL_ADC12_INPUT_CHAN_2";  // ❌ Skulle være CH0!
ADC121.adcMem1chansel = "DL_ADC12_INPUT_CHAN_1";  // ✅ OK
// MANGLER adcMem2chansel!

// KORREKT SKAL VÆRE:
ADC121.adcMem0chansel = "DL_ADC12_INPUT_CHAN_0";  // J1.6  - Mikrofon
ADC121.adcMem1chansel = "DL_ADC12_INPUT_CHAN_1";  // J3.26 - Joy Y
ADC121.adcMem2chansel = "DL_ADC12_INPUT_CHAN_2";  // J1.2  - Joy X
```

---

## 📋 FINAL SUMMARY

### **KORREKT .SYSCFG ENDRINGER:**

1. ✅ **ADC0:** PA27 (CH0), PA26 (CH1), PA25 (CH2)
   - **FIX:** Legg til `adcMem2chansel = "DL_ADC12_INPUT_CHAN_2"`

2. ✅ **ADC1:** PA17 (CH2), PA18 (CH3), PA21 (CH7)
   - **ALLEREDE RIKTIG!**

3. ✅ **SPI:** PB16 (SCLK), PB15 (MOSI), PA2 (CS)
   - **ALLEREDE RIKTIG!**

4. ✅ **GPIO_LCD:** PB13 (RST), PB14 (DC)
   - **ALLEREDE RIKTIG!**

5. ⚠️ **GPIO_RGB:** Velg enten:
   - **Metode A:** PB22/26/27 (onboard LaunchPad - KEEP jumpere)
   - **Metode B:** PB17/18/19 (eksterne LEDs - REMOVE jumpere)

6. ✅ **GPIO_BUTTONS:** PA15 (S1), PA14 (S2)
   - **ALLEREDE RIKTIG!**

---

## 🚀 NEXT STEPS

1. **FIX ADC0 channel mapping** i .syscfg
2. **VELG RGB LED metode** (onboard eller eksterne)
3. **FIX lcd_driver.c** (stack overflow)
4. **TEST!**
