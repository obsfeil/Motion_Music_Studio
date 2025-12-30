# 🎵 MSPM0 Synthesizer v29.0 - Harmonic Progression Guide

## 🎹 Nye Kontroller

| Kontroll | Funksjon | Eksempel |
|----------|----------|----------|
| **S1 Short** | Bytt instrument | PIANO → ORGAN → STRINGS → BASS → LEAD |
| **S1 Long** | Bytt mode | Major ↔ Minor |
| **S1 Double** | Effekter på/av | Vibrato, tremolo, filters |
| **JOY_X** | Velg toneart | C → D → E → F → G → A → B |
| **JOY_Y** | Volum | 0-100% |
| **ACCEL_X** | Harmonier (12 pos) | I → ii → iii → IV → V → vi → vii° → V7 → I/3 → ii7 → vi7 → IVmaj7 |
| **ACCEL_Y** | Oktavskift | ↕ (tilt frem/bak) |

## 🎼 12-Posisjons Harmonisk System

### C Major (Dur):
```
Position 1:  I      - C major    (C-E-G)          [Tonic]
Position 2:  ii     - D minor    (D-F-A)          [Supertonic]
Position 3:  iii    - E minor    (E-G-B)          [Mediant]
Position 4:  IV     - F major    (F-A-C)          [Subdominant]
Position 5:  V      - G major    (G-B-D)          [Dominant]
Position 6:  vi     - A minor    (A-C-E)          [Submediant]
Position 7:  vii°   - B dim      (B-D-F)          [Leading tone]
Position 8:  V7     - G7         (G-B-D-F)        [Dominant 7th]
Position 9:  I/3    - C/E        (E-G-C)          [First inversion]
Position 10: ii7    - Dm7        (D-F-A-C)        [Supertonic 7th]
Position 11: vi7    - Am7        (A-C-E-G)        [Submediant 7th]
Position 12: IVmaj7 - Fmaj7      (F-A-C-E)        [Subdominant maj7]
```

### C Minor (Moll):
```
Position 1:  i      - C minor    (C-Eb-G)         [Tonic]
Position 2:  ii°    - D dim      (D-F-Ab)         [Supertonic dim]
Position 3:  III    - Eb major   (Eb-G-Bb)        [Mediant]
Position 4:  iv     - F minor    (F-Ab-C)         [Subdominant]
Position 5:  V      - G major    (G-B-D)          [Dominant - always major!]
Position 6:  VI     - Ab major   (Ab-C-Eb)        [Submediant]
Position 7:  vii°   - B dim      (B-D-F)          [Leading tone]
Position 8:  V7     - G7         (G-B-D-F)        [Dominant 7th]
Position 9:  i/3    - Cm/Eb      (Eb-G-C)         [First inversion]
Position 10: ii°7   - Dm7b5      (D-F-Ab-C)       [Half-diminished]
Position 11: VI7    - Abmaj7     (Ab-C-Eb-G)      [Submediant maj7]
Position 12: iv7    - Fm7        (F-Ab-C-Eb)      [Subdominant 7th]
```

## 🎸 Instrumenter (Forbedret Kontrast)

### 1. PIANO (Cyan)
- **Karakter**: Bright, percussive, quick decay
- **ADSR**: Attack=40ms, Decay=1200ms, Sustain=65%, Release=600ms
- **Bølgeform**: Triangle
- **Harmonics**: 2 (rich overtones)
- **Effekter**: None (clean)

### 2. ORGAN (Red)
- **Karakter**: Sustained, rich, church-like
- **ADSR**: Attack=0ms, Decay=0ms, Sustain=100%, Release=200ms
- **Bølgeform**: Sine
- **Harmonics**: 3 (full harmonic series)
- **Effekter**: Vibrato 25% (leslie effect)

### 3. STRINGS (Yellow)
- **Karakter**: Warm, slow evolving, orchestral
- **ADSR**: Attack=3200ms, Decay=4000ms, Sustain=90%, Release=5000ms
- **Bølgeform**: Sawtooth
- **Harmonics**: 1 (warm tone)
- **Effekter**: Vibrato 20%, Tremolo 15%

### 4. BASS (Blue)
- **Karakter**: Deep, punchy, resonant
- **ADSR**: Attack=80ms, Decay=400ms, Sustain=95%, Release=600ms
- **Bølgeform**: Sine
- **Harmonics**: 0 (pure fundamental)
- **Effekter**: None (clean low end)

### 5. LEAD (Green)
- **Karakter**: Sharp, bright, aggressive
- **ADSR**: Attack=20ms, Decay=800ms, Sustain=90%, Release=1200ms
- **Bølgeform**: Square
- **Harmonics**: 2 (bright overtones)
- **Effekter**: Vibrato 40%, Tremolo 8%

## 🎵 Musikalske Eksempler

### Klassiske Progressioner i C Major:
```
I → IV → V → I          (C → F → G → C)        [Pop progression]
I → vi → IV → V         (C → Am → F → G)       [50s progression]
ii → V → I              (Dm → G → C)           [Jazz turnaround]
I → V7 → I              (C → G7 → C)           [Perfect cadence]
```

### Jazz/Extended Harmony:
```
I → IVmaj7 → ii7 → V7   (C → Fmaj7 → Dm7 → G7) [Jazz progression]
vi7 → ii7 → V7 → I      (Am7 → Dm7 → G7 → C)   [Circle of fifths]
```

### Minor Mode Progressioner:
```
i → iv → V → i          (Cm → Fm → G → Cm)     [Minor progression]
i → VI → III → V        (Cm → Ab → Eb → G)     [Andalusian cadence]
```

## 🎮 Hvordan Bruke Det

### 1. Velg Toneart:
- Beveg **JOY_X** venstre/høyre for å velge C, D, E, F, G, A, eller B

### 2. Velg Mode:
- Hold **S1 Long** for å bytte mellom Major (dur) og Minor (moll)

### 3. Spill Harmonier:
- Vipp brettet til **venstre** → Position 1 (I)
- Vipp gradvis til **høyre** → Position 2-12
- Du kan "spille" gjennom en progressjon ved å vippe kontrollert!

### 4. Bytt Instrument:
- Trykk **S1 Short** for å bla gjennom PIANO → ORGAN → STRINGS → BASS → LEAD

### 5. Eksperimenter:
- Prøv forskjellige instrument med samme harmonier
- Strings gir varme orkester-pad
- Lead gir skarpe synth-linjer
- Bass gir dype grooves

## 📊 Display-Informasjon

Skjermen viser nå:
- **Topp venstre**: Nåværende toneart + mode (eks: "C MAJ" eller "A MIN")
- **Topp høyre**: Nåværende harmoni (eks: "I", "V7", "ii7")
- **Instrument**: Navn på aktivt instrument
- **Frekvens**: Grunntone i Hz

## 🚀 Tips & Tricks

1. **Lær progressioner**: Start med I-IV-V-I i C Major
2. **Smooth transitions**: Bruk portamento ved å vippe sakte
3. **Orkestral lyd**: Bruk STRINGS med lange harmonier
4. **Funky bass**: Bruk BASS med korte, rytmiske bevegelser
5. **Jazz sound**: Prøv 7th-akkorder (pos 8-12) med ORGAN

## 🎼 Oppnå Spesifikke Sounds

### Ambient/Pad:
- Instrument: STRINGS
- Mode: Major
- Harmonier: I → IVmaj7 → vi7 (sakte bevegelser)

### Funky Bass:
- Instrument: BASS
- Mode: Major
- Harmonier: I → IV → V (raske switches)

### Jazz Organ:
- Instrument: ORGAN
- Mode: Major
- Harmonier: ii7 → V7 → I (med vibrato on)

### Epic Lead:
- Instrument: LEAD
- Mode: Minor
- Harmonier: i → VI → III → V

Lykke til med musikkskapingen! 🎶
