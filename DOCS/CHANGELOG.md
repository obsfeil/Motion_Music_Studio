# 🎵 Motion Music Studio - Changelog

## v29.3 - Greensleeves & Advanced Harmonies (Current)

This version represents a major leap in musical sophistication, transforming the synthesizer into a powerful performance and composition tool.

### 🚀 New Features

-   **🍀 Greensleeves Mode:**
    -   An authentic, 16-step sequential performance of the traditional 16th-century English melody "Greensleeves".
    -   Activated with a short press of the **Joystick Select** button.
    -   Features a classic Am-C-G-Am-E-Am progression.
    -   Automatically uses a "STRINGS" instrument for a lute/fiddle-like sound.

-   **🎼 Advanced Harmonic Progression System:**
    -   The **X-axis of the accelerometer** now controls a 24-position harmonic progression system.
    -   This allows for smooth, musically-aware chord changes, moving seamlessly through diatonic chords, inversions, 7th chords, and even chords in different octaves.
    -   Supports both **Major and Minor** modes, with correct chord qualities for each.
    -   Harmonic functions range from `vii°` (diminished leading tone) below the root to `I` three octaves up.

-   **🎹 Dual-Mode Operation: Standalone Synth & MIDI Controller:**
    -   The device now simultaneously generates local audio via the DAC and transmits corresponding **MIDI messages** over UART at 921600 baud.
    -   A companion Python script (`uart_audio_player.py`) is provided to receive and play these MIDI messages on a PC, turning the device into a fully-fledged MIDI controller.

-   **🎶 Extended Chord Voicings:**
    -   The harmony engine now includes Dominant 7th (`V7`), minor 7th (`ii7`, `vi7`), and Major 7th (`IVmaj7`) chords.
    -   Includes chord inversions (e.g., `I/E`).

### 🔧 Architectural Changes & Fixes

-   **MIDI Protocol Integration:**
    -   A complete MIDI protocol implementation has been added directly into `main.c`. This includes functions for creating Note On, Note Off, Control Change, and Program Change messages.
    -   **Note:** This implementation is currently **redundant**. The file `midi_handler.h` exists but is not used. Future work should refactor `main.c` to use the dedicated handler.

-   **Hardware-Accelerated Sine Waves:**
    -   Utilizes the `MATHACL` co-processor to generate mathematically perfect sine waves, offloading the main CPU and improving audio quality.

-   **Refined Control Scheme:**
    -   **S1 Button:** Short press cycles instruments, Long press toggles Major/Minor mode, Double click toggles effects.
    -   **S2 Button:** Short press toggles Play/Stop, Long press cycles chord modes, Double click toggles the arpeggiator.
    -   **Joystick X:** Selects the musical key (C-B).
    -   **Accelerometer Y:** Controls octave shifting.

### ⚠️ Known Issues

-   **Redundant MIDI Code:** As mentioned, `main.c` duplicates the functionality of `midi_handler.h`. This should be refactored.
-   **Version Mismatch:** The version in the `main.c` file header (`v29.3`) differs from the version printed at runtime (`v28.2.1`). This is a cosmetic bug.

---

## v1.1.0 (Legacy)

### 🔧 Fixed Issues
-   Added `volatile` keyword to variables modified in ISRs to prevent compiler optimization bugs.
-   Corrected timer wrap-around calculations.
-   Prevented integer overflows in delay functions by using `uint64_t`.
-   Improved array bounds safety with explicit masking.
-   Resolved signed/unsigned type mixing warnings.
-   Replaced "magic numbers" for LCD positioning with named constants.

### 🆕 New Features
-   Added local copies of volatile variables to prevent race conditions.
-   Improved button debouncing logic.
