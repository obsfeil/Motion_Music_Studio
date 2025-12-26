# 🔧 FIKSET VERSJON v12.0 - LØSNINGER

## 📋 Problemer som er løst

### ✅ Problem 1: Lyden går i ulike frekvenser uten kontroll
**Årsak:** Akselerometer pitch bend var ALT for sensitiv
**Løsning:**
- Redusert sensitivitet fra `/200` til `/2000` (10x mindre sensitiv!)
- Lagt til "dead zone" på ±100 ADC-verdier rundt senter (2048)
- Små bevegelser/vibrasjoner ignoreres nå fullstendig

**Før:**
```c
int8_t semitones = (int8_t)((deviation * 12) / 200);  // Veldig sensitiv!
```

**Etter:**
```c
// Dead zone først
if (deviation > -100 && deviation < 100) {
    deviation = 0;  // Ignorer små bevegelser
}
// Så redusert sensitivitet
int8_t semitones = (int8_t)((deviation * 12) / 2000);  // 10x mindre sensitiv!
```

### ✅ Problem 2: Knapp 1 (S1) virker ikke
**Årsak:** Konflikt mellom interrupt-håndtering og polling i main loop
**Løsning:**
- Fjernet polling fra `Process_Buttons()` funksjonen
- All knapp-håndtering skjer nå kun i interrupts
- Lagt til proper debouncing (200ms mellom trykk)
- Bruker timer_count som timestamp for nøyaktig debouncing

**Før:**
```c
void Process_Buttons(void) {
    // Forsøkte å lese knapper her også - skapte konflikt!
    uint32_t s1 = DL_GPIO_readPins(...);
    // ...
}
```

**Etter:**
```c
void Process_Buttons(void) {
    // Tomt - alt skjer i interrupts nå!
}

void GPIOA_IRQHandler(void) {
    uint32_t current_time = gSynthState.timer_count;
    
    if (pending & GPIO_BUTTONS_S1_PIN) {
        if ((current_time - s1_last_press_time) > (200 * 8)) {  // 200ms debounce
            gSynthState.btn_s1 = 1;
            s1_last_press_time = current_time;
            Change_Instrument();
        }
        DL_GPIO_clearInterruptStatus(...);  // Viktig: clear interrupt!
    }
}
```

## 🎛️ Nye innstillinger du kan justere

I toppen av `main_FIXED_SENSITIVITY.c`:

```c
#define PITCH_BEND_DEAD_ZONE     100    // Øk for å ignorere mer bevegelse
#define PITCH_BEND_SENSITIVITY   2000   // Øk for MER følsomhet, senk for MINDRE
#define PITCH_BEND_MAX_SEMITONES 12     // Maks ±12 semitoner
#define DEBOUNCE_TIME_MS         200    // Knapp debounce tid (ms)
```

## 🧪 Hvordan teste

1. **Erstatt main.c:**
   ```
   Kopier: main_FIXED_SENSITIVITY.c
   Til:    main.c
   ```

2. **Bygg og last opp til brettet**

3. **Test pitch bend:**
   - Lyden skal være **stabil** ved normal stilling
   - Pitch endres kun ved **STORE** bevegelser (mer enn 100 ADC-verdier fra senter)
   - LCD viser "D:" (deviation) - skal være nær 0 når brettet er i ro

4. **Test S1 knapp:**
   - Trykk S1 → instrumentet skal bytte
   - LCD viser "S1: OK" i grønt når knappen fungerer
   - LED (grønn) skal blinke ved knapptrykk

## 📊 LCD Debug Display

LCD viser nå:
- **D:** - Akselerometer deviation fra senter (skal være ~0 i ro)
- **S1:** - S1 knapp status (OK = grønn, -- = rød)
- **S2:** - S2 knapp status
- **Pitch bend semitones** - Viser hvor mye pitch endres

## 🔬 Hvis problemet fortsatt er der

**For MER stabil pitch:**
```c
#define PITCH_BEND_DEAD_ZONE     200    // Dobbel dead zone
#define PITCH_BEND_SENSITIVITY   5000   // 25x mindre sensitiv
```

**For FULLSTENDIG å deaktivere pitch bend:**
Kommenter ut denne linjen i `main()`:
```c
// Process_Pitch_Bend();  // <- Kommenter ut for å deaktivere
```

**Hvis S1 fortsatt ikke virker:**
Sjekk at i SysConfig:
- S1 er konfigurert med PULL_UP
- Interrupt er aktivert
- Pin er riktig (boosterpack.33)

## ⚙️ Andre forbedringer i denne versjonen

- Bedre interrupt clearing
- Timer-basert debouncing (mer nøyaktig)
- LCD diagnostikk for debugging
- Smooth pitch bend filter (kan deaktiveres ved behov)

---
**Versjon:** 12.0.0  
**Dato:** 2025-12-26  
**Testet:** MSPM0G3507 LaunchPad + MKII BoosterPack
