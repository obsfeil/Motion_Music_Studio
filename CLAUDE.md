# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Motion Music Studio** is an embedded synthesizer and MIDI controller built on the Texas Instruments **MSPM0G3507 LaunchPad** with the **BOOSTXL-EDUMKII** Educational BoosterPack. It operates as both a standalone synthesizer (DAC audio output) and a MIDI controller (UART transmission to PC).

**Current Version:** v31.0 (Professional Audio Quality)
- 48 kHz sample rate (3x improvement from v30)
- MATHACL biquad anti-aliasing filter
- Linear interpolation for smoother output
- OPA buffer for direct 8Ω speaker drive

**Hardware:** MSPM0G3507 LaunchPad + BOOSTXL-EDUMKII BoosterPack

## Build Commands

### Code Composer Studio (Primary IDE)

This project is designed for **Code Composer Studio (CCS)** with the MSPM0 SDK.

**Clean and Build:**
```
Right-click project → Clean Project
Project → Build All (or F7)
```

**Flash and Debug:**
```
Debug → Flash (or F11)
Resume (F8) to run
```

**Force SysConfig Regeneration:**
If you encounter `ti_msp_dl_config.h not found` errors:
```
1. Double-click ti_msp_dl_config.syscfg in Project Explorer
2. Save file (Ctrl+S) to force generation of ti_msp_dl_config.h/c
```

### Python MIDI Client (Companion Tool)

**Install Dependencies:**
```bash
pip install pyserial sounddevice numpy
```

**Run MIDI Player:**
```bash
python uart_audio_player.py
```

The script auto-detects the serial port and provides 8-voice polyphonic MIDI playback on your PC.

## Architecture Overview

### Core Components

**main.c** (2,000+ lines)
- Main application logic and state machine
- Audio generation ISR (`TIMG7_IRQHandler` @ 48 kHz)
- MIDI message generation and transmission
- Control input processing (buttons, joystick, accelerometer)
- 24-position harmonic progression system
- "Greensleeves" automatic performance mode
- **Known Issue:** Contains redundant MIDI implementation that should use `midi_handler.h`

**main.h**
- `SynthState_t` structure - central state container with `volatile` flags for ISR safety
- Library includes for audio engine and EDUMKII peripherals

**ti_msp_dl_config.syscfg**
- Hardware configuration file (TI SysConfig tool)
- Generates `ti_msp_dl_config.h/c`
- Configures: DAC12, ADC12, UART, TIMG7, SysTick, MATHACL, OPA buffer
- **Critical:** Any peripheral changes must be made here, not in generated files

**lcd_driver.c/h**
- ST7735 LCD display driver
- SPI communication for visual feedback

### Library Structure

**lib/audio/** - Audio synthesis engine
- `audio_engine.c/h` - Wavetable synthesis (Sine, Square, Sawtooth, Triangle)
  - MATHACL hardware-accelerated sine generation
  - Complete waveform tables with ±2048 range
- `audio_envelope.c/h` - ADSR envelope generator
  - 5 predefined profiles: PIANO, ORGAN, STRINGS, BASS, LEAD
- `audio_filters.c/h` - Signal processing
  - Low-pass filter, soft/hard clipping, gain compensation
  - Biquad IIR anti-aliasing filter (v31.0)
  - Linear interpolation

**lib/edumkii/** - Hardware abstraction for BOOSTXL-EDUMKII
- `edumkii_buttons.c/h` - Button state machine with short/long/double-click detection
- `edumkii_joystick.c/h` - Analog input with deadzone filtering
- `edumkii_accel.c/h` - BMI160 accelerometer for tilt/motion control
- `edumkii.h` - Umbrella header

**midi_handler.h** (Header-only)
- Complete MIDI 1.0 protocol implementation
- Note/frequency conversion tables and utilities
- **Currently unused - main.c has duplicate implementation (architectural debt)**

### Audio Signal Flow

```
TIMG7 ISR (every 20.83 µs = 48 kHz)
  ↓
MATHACL_Sine(phase) → Generate waveform
  ↓
Apply ADSR envelope
  ↓
Apply volume control
  ↓
Filter_GainWithFreqCompensation()
  ↓
Filter_LowPass() (simple IIR)
  ↓
Filter_SoftClip()
  ↓
BiquadFilter_Process() (anti-aliasing, v31.0)
  ↓
Interpolate_Linear() (sub-sample precision, v31.0)
  ↓
Audio_WriteDAC12(sample)
  ↓
DL_DAC12_output12(DAC0, value)
  ↓
DAC12 (PA15) → OPA buffer → Speaker output
```

### Control Mapping

**S1 Button:**
- Short: Cycle through 5 instruments (PIANO → ORGAN → STRINGS → BASS → LEAD)
- Long: Toggle Major/Minor musical mode
- Double: Toggle effects (Vibrato/Tremolo) on/off

**S2 Button:**
- Short: Play/Stop audio
- Long: Cycle chord mode (Off → Major → Minor)
- Double: Toggle Arpeggiator

**Joystick:**
- X-Axis: Select musical key (C, D, E, F, G, A, B) with deadzone hold
- Y-Axis: Master volume control (0-100%)
- Select Short: Activate Greensleeves performance mode
- Select Long: Reset synthesizer to default state

**Accelerometer:**
- X-Axis (Tilt Left/Right): Navigate 24-position harmonic progression
  - Positions include: vii↓, I, ii, iii, IV, V, vi, vii, I↑, and 7th chords
- Y-Axis (Tilt Forward/Back): Octave shift (±1 octave)

### Harmonic Progression System

The project features a sophisticated 24-position harmonic progression controlled by accelerometer X-axis tilt:

- Diatonic chords in selected key
- 7th chord variations
- Multiple inversions for voice leading
- Musically-aware transitions between positions

This is one of the project's unique features and represents significant domain knowledge in music theory.

## Key Architectural Patterns

### Volatile Variables for ISR Safety
All variables modified in ISRs are declared `volatile` in `SynthState_t`:
```c
volatile uint16_t joy_x, joy_y;
volatile int16_t accel_x, accel_y, accel_z;
volatile bool audio_playing;
```

When reading these in the main loop, create local copies to avoid race conditions:
```c
uint16_t joy_x_local = gSynthState.joy_x;  // Read once
// Use joy_x_local for calculations
```

### Timer Wrap-Around Handling
Hardware timers wrap around. Use proper elapsed time calculation:
```c
#define TIMER_ELAPSED(now, start) \
    ((now) >= (start) ? ((now) - (start)) : (TIMER_MAX_VALUE - (start) + (now)))
```

### Deadzone Filtering
Joystick and accelerometer use deadzone filtering to prevent jitter. Values hold stable when input is released. This pattern is critical for stable note selection.

### Fixed-Point Arithmetic
Audio uses Q15 fixed-point (16-bit signed, range ±2048) for efficiency:
- All waveform samples: -2048 to +2047
- ADSR amplitude: 0-1000 (0-100%)
- Biquad filter coefficients: Q15 format

## Important Development Notes

### SysConfig Workflow
1. **Always edit `ti_msp_dl_config.syscfg`, never the generated `.h/.c` files**
2. Generated files are overwritten on each build
3. To add/modify peripherals, use the SysConfig GUI
4. Pin assignments are configured here (see `DOCS/Komplett_Pin_Map.md` for reference)

### Upgrading Between Versions
The project includes comprehensive upgrade guides:
- `V31_QUICK_START.md` - Quick 3-step upgrade to v31.0
- `V31_COMPLETE_UPGRADE_GUIDE.md` - Detailed technical guide
- Backup files before upgrading: `main.c`, `ti_msp_dl_config.syscfg`

### Hardware Output Configuration (v31.0)
- **DAC12:** PA15 (internal connection to OPA)
- **OPA Output:** Check `ti_msp_dl_config.syscfg` for pin assignment (likely PA16)
- **External Circuit:** OPA_OUT → [100Ω] → [10µF capacitor] → 8Ω Speaker
- Can also connect to piezo buzzer if speaker unavailable

### MIDI Implementation
- Baud rate: 921600 (USB-MIDI speed, not standard 31250)
- Transmits: Note On/Off, Program Change, Control Change (Volume)
- Channel: 0 (MIDI channel 1)
- Python client provides polyphonic playback (8 voices)

### Known Issues & Technical Debt
1. **Redundant MIDI Code:** `main.c` contains full MIDI implementation that duplicates `midi_handler.h`. Should refactor to use the header-only library.
2. **Documentation Lag:** Many files in `DOCS/` are outdated and don't reflect current v31.0 architecture.
3. **Version Inconsistency:** LCD may display v28.2.1 while actual feature set is v31.0.

## Testing and Debugging

### Verify Audio Output
1. Connect BOOSTXL-EDUMKII to LaunchPad
2. Connect speaker to OPA output pin (v31.0) or use headphone jack
3. Flash firmware via CCS debugger
4. Use joystick to change notes - should hear audio immediately
5. For v31.0: Verify OPA output pin in SysConfig before connecting speaker

### MIDI Testing
1. Run `python uart_audio_player.py` on PC
2. Script auto-detects COM port and enters MIDI mode
3. Play notes on device - should hear through PC speakers
4. Press Ctrl+C to stop script

### Common Build Issues

**"ti_msp_dl_config.h not found":**
- Open `ti_msp_dl_config.syscfg` and save (Ctrl+S)

**"Cannot find -ldriverlib":**
- Verify MSPM0 SDK path in Project Properties
- Check: Build → ARM Linker → File Search Path

**Audio Distortion:**
- Reduce volume (joystick Y-axis down)
- Check gain settings in filter configuration
- Verify sample rate matches SysConfig timer period

**No Sound from Speaker (v31.0):**
- Verify OPA output pin in SysConfig
- Check speaker wiring and polarity
- Confirm 10µF capacitor orientation (+ to OPA side)
- Test with headphones first

## File Organization

```
Motion_Music_studio/
├── main.c                    # Core application (2000+ lines)
├── main.h                    # State structure and includes
├── lcd_driver.c/h           # LCD display driver
├── midi_handler.h           # MIDI protocol (unused, should refactor)
├── ti_msp_dl_config.syscfg  # Hardware configuration (EDIT THIS)
├── lib/
│   ├── audio/              # Synthesis engine
│   │   ├── audio_engine.c/h
│   │   ├── audio_envelope.c/h
│   │   └── audio_filters.c/h
│   └── edumkii/            # BoosterPack HAL
│       ├── edumkii_buttons.c/h
│       ├── edumkii_joystick.c/h
│       └── edumkii_accel.c/h
├── DOCS/                   # Documentation (some outdated)
│   ├── BUILD_INSTRUCTIONS.md
│   ├── Komplett_Pin_Map.md
│   └── README.md
├── V31_QUICK_START.md      # v31.0 upgrade guide
├── V31_COMPLETE_UPGRADE_GUIDE.md  # Detailed v31.0 docs
├── uart_audio_player.py    # PC MIDI client
└── example_main.c          # Library usage examples
```

## Performance Characteristics

**CPU Usage (v31.0 @ 48 kHz, 80 MHz system clock):**
- Per-sample cycles available: 1,667
- Per-sample cycles used: ~300 (18%)
- Headroom: 82% (plenty for additional features)

**Memory Usage:**
- FLASH: ~20 KB / 128 KB (15%)
- SRAM: ~1.3 KB / 32 KB (4%)
- Stack: 512 bytes

**Audio Quality:**
- Sample rate: 48 kHz (professional audio standard)
- Bit depth: 12-bit DAC (4096 levels)
- SNR: ~78 dB (with biquad filter)
- Nyquist frequency: 24 kHz (covers full human hearing range)

## Musical Features

**5 Instruments with Unique Characteristics:**
- PIANO: Fast attack, medium decay, bright
- ORGAN: Instant attack, sustained, classic
- STRINGS: Slow attack, long sustain, smooth
- BASS: Medium attack, short decay, punchy
- LEAD: Fast attack, long sustain, vibrato

**Effects:**
- Vibrato: Pitch modulation via LFO
- Tremolo: Amplitude modulation
- Per-instrument configuration

**Musical Modes:**
- Major and Minor scales
- 7 keys: C, D, E, F, G, A, B
- Automatic scale transposition

**Chord System:**
- Major and Minor chord modes
- Root + 3rd + 5th generation
- Arpeggiator with Up/Down/UpDown patterns

## Code Style Observations

- Extensive inline documentation and ASCII art diagrams
- Norwegian comments mixed with English (project origin)
- Defensive programming: bounds checking, overflow protection
- Explicit type casts to avoid signed/unsigned warnings
- Constants over magic numbers where critical (LCD positions, audio ranges)
- ISR-safe patterns with volatile and atomic reads
