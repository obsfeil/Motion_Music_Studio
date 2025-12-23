# 🔌 KOMPLETT JUMPER MAP - MSPM0G3507 LaunchPad + BoosterPack MKII

## 📍 LAUNCHPAD JUMPERE (fra TI LP-MSPM0G3507 User Guide)

Basert på Table 2-1 i offisiell dokumentasjon.

---

## 🔴 **KRITISKE JUMPERE (MÅ SJEKKES!)**

### **J5, J6, J7 - RGB LED**

Disse styrer tilkobling til **onboard RGB LED** på LaunchPad:

| Jumper | Pin | LED Farge | Default | Din Setting |
|--------|-----|-----------|---------|-------------|
| **J5** | **PB22** | **Blue** | INSTALLED | **✅ INSTALLED** |
| **J6** | **PB26** | **Red** | INSTALLED | **✅ INSTALLED** |
| **J7** | **PB27** | **Green** | INSTALLED | **✅ INSTALLED** |

**Funksjon:**
```
INSTALLED (jumper på):
├─ PB22/26/27 → Connected to onboard RGB LED
├─ Pins er AKTIVE LAV (LOW = ON)
└─ DETTE ER KORREKT FOR DITT PROSJEKT ✅

REMOVED (jumper av):
├─ PB22/26/27 → Frigjort for andre formål
├─ Onboard RGB LED virker IKKE
└─ Må bruke eksterne LEDs på andre pins
```

**VIKTIG FOR DITT PROSJEKT:**
- ✅ **KEEP INSTALLED** - Du bruker onboard RGB LED!
- ✅ Din .syscfg er konfigurert for PB22/26/27
- ✅ RGB LED viser waveform status i synthesizer

---

### **J8 - S1 Button og BSL Invoke**

| Jumper | Pin | Funksjon | Default | Din Setting |
|--------|-----|----------|---------|-------------|
| **J8** | **PA18** | S1 Button + BSL | INSTALLED | **⚠️ AVHENGER** |

**Funksjon:**
```
INSTALLED:
├─ PA18 koblet til S1 button (onboard LaunchPad)
└─ Også brukt for BSL (bootloader) invoke

REMOVED:
├─ Frigjør PA18 for andre formål
└─ S1 button virker IKKE
```

**FOR DITT PROSJEKT:**
- ⚠️ **Din .syscfg bruker IKKE PA18 for buttons**
- ✅ Du bruker PA15 (J4.33 på BoosterPack) for S1
- ✅ Du bruker PA14 (J4.32 på BoosterPack) for S2
- ℹ️ **ANBEFALING:** REMOVE J8 for å frigjøre PA18 (eller la være hvis ikke brukt)

**MERK:** PA18 er også ADC1 CH3 (Accel Y)! Se conflict nedenfor.

---

### **J10 - 5V Power Header**

| Jumper | Pins | Funksjon | Default | Din Setting |
|--------|------|----------|---------|-------------|
| **J10** | 5V, GND | External 5V power | NA (header) | **ℹ️ NO JUMPER** |

**Funksjon:**
```
Dette er en HEADER (ikke jumper):
├─ Brukes til å koble ekstern 5V forsyning
├─ Eller for å tappe ut 5V fra LaunchPad
└─ Ikke relevant for BoosterPack MKII setup
```

**FOR DITT PROSJEKT:**
- ℹ️ Ikke nødvendig - BoosterPack får strøm via main headers
- ✅ Kan ignoreres

---

### **J11 - 3V3 Power Header**

| Jumper | Pins | Funksjon | Default | Din Setting |
|--------|------|----------|---------|-------------|
| **J11** | 3V3, GND | External 3.3V power | NA (header) | **ℹ️ NO JUMPER** |

**Funksjon:**
```
Dette er en HEADER (ikke jumper):
├─ Brukes til å koble ekstern 3.3V forsyning
├─ Eller for å tappe ut 3.3V fra LaunchPad
└─ Ikke relevant for BoosterPack MKII setup
```

**FOR DITT PROSJEKT:**
- ℹ️ Ikke nødvendig
- ✅ Kan ignoreres

---

### **J13 - Analog Power**

| Jumper | Function | Default | Din Setting |
|--------|----------|---------|-------------|
| **J13** | Power to thermistor + OPA2365 | INSTALLED | **⚠️ SJEKK** |

**Funksjon:**
```
INSTALLED:
├─ 3.3V til thermistor circuit
├─ 3.3V til OPA2365 op-amp
└─ Onboard analog sensors enabled

REMOVED:
├─ Frigjør strøm
└─ Onboard analog circuits disabled
```

**FOR DITT PROSJEKT:**
- ⚠️ **ANBEFALING:** REMOVE hvis ikke brukt (spar strøm)
- ✅ BoosterPack MKII har sine egne sensorer
- ℹ️ Ikke kritisk - kan være installed

---

### **J16, J17, J18 - Light Sensor Circuit**

| Jumper | Pin | Funksjon | Default | Din Setting |
|--------|-----|----------|---------|-------------|
| **J16** | PA22 | OPA0_OUT for light sensor | INSTALLED | **⚠️ CONFLICT!** |
| **J17** | PA27 | OPA0_IN0- for light sensor | INSTALLED | **❌ CONFLICT!** |
| **J18** | PA26 | OPA0_IN0+ for light sensor | INSTALLED | **❌ CONFLICT!** |

**KRITISK CONFLICT OPPDAGET! ⚠️**

```
J17 (PA27): Onboard light sensor ← KONFLIKT! → BoosterPack MKII Mikrofon (J1.6)
J18 (PA26): Onboard light sensor ← KONFLIKT! → BoosterPack MKII Joy Y (J3.26)
```

**LØSNING FOR DITT PROSJEKT:**

```
✅ REMOVE J17 (PA27) - Frigjør for mikrofon
✅ REMOVE J18 (PA26) - Frigjør for joystick Y
⚠️ REMOVE J16 (PA22) - Ikke nødvendig uten J17/18
```

**Etter fjerning:**
- ✅ PA27 → Mikrofon (BoosterPack MKII)
- ✅ PA26 → Joystick Y (BoosterPack MKII)
- ✅ PA22 → Frigjort (ikke i bruk)

---

### **J19, J20 - I2C Pull-up Resistors**

| Jumper | Pin | Funksjon | Default | Din Setting |
|--------|-----|----------|---------|-------------|
| **J19** | PA0 | I2C0 SDA pull-up to 3.3V | [1]-[2] (3.3V) | **✅ INSTALLED** |
| **J20** | PA1 | I2C0 SCL pull-up to 3.3V | [1]-[2] (3.3V) | **✅ INSTALLED** |

**Funksjon:**
```
INSTALLED [1]-[2]:
├─ PA0 → 3.3V via pull-up resistor
├─ PA1 → 3.3V via pull-up resistor
└─ Required for I2C communication

REMOVED:
├─ No pull-up
└─ I2C vil IKKE virke uten eksterne pull-ups
```

**FOR DITT PROSJEKT:**
- ✅ **KEEP INSTALLED** - Du bruker I2C0 (OPT3001 light sensor)
- ✅ I2C krever pull-up resistors
- ✅ [1]-[2] position er riktig (3.3V)

---

### **J21, J22 - UART Selection**

| Jumper | Pin | Funksjon | Default | Din Setting |
|--------|-----|----------|---------|-------------|
| **J21** | PA10 | UART0_TX → XDS debugger | [1]-[2] (XDS) | **⚠️ AVHENGER** |
| **J22** | PA11 | UART0_RX → XDS debugger | [1]-[2] (XDS) | **⚠️ AVHENGER** |

**Funksjon:**
```
INSTALLED [1]-[2] (XDS):
├─ PA10/PA11 → Connected to XDS110 debugger
├─ Kan bruke UART backchannel via USB
└─ Nyttig for printf debugging

REMOVED:
├─ PA10/PA11 → Frigjort for andre formål
└─ UART backchannel virker IKKE
```

**FOR DITT PROSJEKT:**
- ✅ Din .syscfg bruker PA10/PA11 for I2C1 (TMP006 temp sensor)
- ⚠️ **CONFLICT:** Kan ikke bruke BÅDE I2C1 og UART backchannel!
- ℹ️ **ANBEFALING:** REMOVE J21/J22 hvis du bruker I2C1
- ℹ️ **ALTERNATIV:** La være installed hvis du ikke trenger I2C1

---

## 🔵 **ANDRE JUMPERE (Mindre kritiske)**

### **J101 - XDS110-ET Isolation Block**

| Jumper | Funksjon | Default | Din Setting |
|--------|----------|---------|-------------|
| **J101** | Connection to XDS110 debugger | INSTALLED | **✅ INSTALLED** |

**Funksjon:**
```
INSTALLED (alle 8 pins):
├─ GND, 5V, 3V3, RXD, TXD, NRST, SWDIO, SWCLK
├─ Connected to onboard XDS110 debugger
└─ Kan flash og debug via USB

REMOVED:
├─ Disconnect from XDS110
├─ Kan bruke external debugger via J102/J103
└─ Brukes for low-power measurements
```

**FOR DITT PROSJEKT:**
- ✅ **KEEP INSTALLED** - Du trenger debugging!
- ✅ Lar deg flash kode via CCS

---

### **J9 - Thermistor Selection**

| Jumper | Pin | Funksjon | Default | Din Setting |
|--------|-----|----------|---------|-------------|
| **J9** | PB24 | Thermistor circuit | [1]-[2] (PB24) | **⚠️ IKKE BRUKT** |

**FOR DITT PROSJEKT:**
- ℹ️ Du bruker ikke onboard thermistor
- ℹ️ PB24 brukes til VREF output i din .syscfg
- ⚠️ **ANBEFALING:** REMOVE J9 for å frigjøre PB24

**MERK:** Din .syscfg bruker PA23 (ikke PB24) for VREF, så dette er OK.

---

### **J14, J15 - Switch Selection**

| Jumper | Funksjon | Default | Din Setting |
|--------|----------|---------|-------------|
| **J14** | SW1 → PB23 (BP header) | [1]-[2] (PB23) | **ℹ️ IKKE BRUKT** |
| **J15** | SW2 → PA16 (BP header) | [1]-[2] (PA16) | **⚠️ SJEKK** |

**FOR DITT PROSJEKT:**
- ℹ️ Du bruker ikke onboard SW1/SW2
- ✅ Du bruker BoosterPack MKII buttons (S1/S2)
- ⚠️ **PA16 brukes til SPI MISO!**
- ℹ️ **ANBEFALING:** REMOVE J15 for å unngå conflict

---

## 📊 **OPPSUMMERING - DITT PROSJEKT**

### **✅ JUMPERE SOM MÅ VÆRE INSTALLED:**

| Jumper | Funksjon | Hvorfor |
|--------|----------|---------|
| **J5** | RGB Blue (PB22) | Onboard LED brukes i synthesizer ✅ |
| **J6** | RGB Red (PB26) | Onboard LED brukes i synthesizer ✅ |
| **J7** | RGB Green (PB27) | Onboard LED brukes i synthesizer ✅ |
| **J19** | I2C0 SDA pull-up | I2C krever pull-up ✅ |
| **J20** | I2C0 SCL pull-up | I2C krever pull-up ✅ |
| **J101** | XDS110 debugger | Flash/debug via USB ✅ |

---

### **❌ JUMPERE SOM MÅ FJERNES:**

| Jumper | Funksjon | Hvorfor |
|--------|----------|---------|
| **J17** | Light sensor (PA27) | **CONFLICT!** PA27 brukes til Mikrofon ❌ |
| **J18** | Light sensor (PA26) | **CONFLICT!** PA26 brukes til Joystick Y ❌ |
| **J16** | Light sensor (PA22) | Ikke nødvendig uten J17/18 ⚠️ |

---

### **⚠️ JUMPERE SOM KAN FJERNES (valgfritt):**

| Jumper | Funksjon | Anbefaling |
|--------|----------|------------|
| **J8** | S1 button (PA18) | Kan fjernes hvis PA18 trengs til annet ℹ️ |
| **J13** | Analog power | Kan fjernes for å spare strøm ℹ️ |
| **J15** | SW2 selection | Kan fjernes (PA16 brukes til SPI) ⚠️ |
| **J21/J22** | UART backchannel | Fjern hvis I2C1 brukes ℹ️ |

---

## 🔧 **FYSISK JUMPER LOCATIONS**

```
LaunchPad (top view):

    [USB]
     ┃
  J101 ══════╗
             ║
    J5  J6  J7  (RGB LED - TOP RIGHT)
    ══  ══  ══
    
    [BoosterPack Headers]
    ║         ║
    J16 J17 J18  (Light sensor - LEFT SIDE)
    ══  ══  ══
    
    J19 J20  (I2C pull-ups - LEFT SIDE)
    ══  ══
    
    J21 J22  (UART - LEFT SIDE)
    ══  ══
```

---

## ✅ **QUICK CHECKLIST**

Før du starter prosjektet, sjekk fysisk på LaunchPad:

- [ ] **J5 (Blue):** INSTALLED ✅
- [ ] **J6 (Red):** INSTALLED ✅
- [ ] **J7 (Green):** INSTALLED ✅
- [ ] **J17 (PA27):** REMOVED ❌
- [ ] **J18 (PA26):** REMOVED ❌
- [ ] **J19 (I2C SDA):** INSTALLED ✅
- [ ] **J20 (I2C SCL):** INSTALLED ✅
- [ ] **J101 (Debugger):** INSTALLED ✅

**Hvis alle disse er korrekt → Klar til å flashe!** 🚀

---

## 🆘 **TROUBLESHOOTING**

### **Problem: RGB LED virker ikke**
**Check:** J5/J6/J7 installed? LEDs aktive lav (clearPins = ON)?

### **Problem: Joystick gir 0xFFFF eller ingen respons**
**Check:** J17/J18 removed? PA26/PA27 frigjort fra light sensor?

### **Problem: I2C virker ikke**
**Check:** J19/J20 installed? Pull-ups nødvendig for I2C!

### **Problem: Kan ikke flashe via CCS**
**Check:** J101 installed? XDS110 debugger må være koblet til!

---

## 📸 **PHOTO GUIDE TIP**

Ta et bilde av LaunchPad før/etter jumper changes for referanse! 📷
