# 🎵 Motion Music Studio - Complete Synth & MIDI Controller

## 1. Overview

**Motion Music Studio** is an advanced embedded synthesizer and MIDI controller built on the Texas Instruments **MSPM0G3507 LaunchPad** and the **BOOSTXL-EDUMKII** Educational BoosterPack.

It is a highly interactive musical instrument that uses a joystick, buttons, and an accelerometer to control a sophisticated synthesis engine. The project operates in a dual mode:

1.  **Standalone Synthesizer:** It generates audio directly through its onboard Digital-to-Analog Converter (DAC), which can be connected to headphones or speakers.
2.  **MIDI Controller:** It simultaneously transmits MIDI data over a high-speed UART (921600 baud). A companion Python script (`uart_audio_player.py`) is provided to run on a PC, which receives this data and plays it through a high-quality polyphonic software synthesizer.

The current firmware version is **v29.3**, which introduces a powerful harmonic progression system and a special "Greensleeves" performance mode.

## 2. Features

### Synthesis Engine
-   **5 Unique Instruments:** PIANO, ORGAN, STRINGS, BASS, and LEAD, each with a distinct ADSR envelope, waveform, and effects profile.
-   **Multiple Waveforms:** Sine (hardware-accelerated via MATHACL), Square, Sawtooth, and Triangle.
-   **Effects:** Per-instrument Vibrato and Tremolo.
-   **Filters:** Low-pass filter and soft clipping for sound shaping.
-   **Chord Mode:** Play major or minor chords.
-   **Arpeggiator:** Simple Up, Down, and Up/Down arpeggiation modes.

### Musical Control
-   **Advanced Harmonic Progression:** A 24-position system controlled by the accelerometer allows for smooth, musically-aware chord changes through diatonic chords, 7ths, and inversions.
-   **Key & Mode Selection:** Play in any key (C-B) and switch between Major and Minor modes.
-   **Octave Shifting:** Tilt the device forward or backward to shift the octave up or down.
-   **Greensleeves Mode:** An automatic performance of the traditional 16th-century melody "Greensleeves".

### MIDI Output
-   **Real-time MIDI Messages:** Transmits Note On, Note Off, Control Change (Volume), and Program Change (Instrument) messages.
-   **High-Speed UART:** Runs at 921600 baud for low-latency performance.
-   **Polyphonic Python Synth:** The included `uart_audio_player.py` script provides an 8-voice polyphonic synthesizer with a simple ADSR envelope to play the MIDI data from the device.

## 3. Controls

| Control | Action | Function |
|---|---|---|
| **S1 Button** | Short Press | Cycle through the 5 instruments. |
| | Long Press | Toggle between Major and Minor musical modes. |
| | Double Click | Toggle all effects (Vibrato, Tremolo) on or off. |
| **S2 Button** | Short Press | Play or Stop the sound. |
| | Long Press | Cycle through chord modes (Off, Major, Minor). |
| | Double Click | Toggle the Arpeggiator on or off. |
| **Joystick** | X-Axis | Select the musical key (C, D, E, F, G, A, B). |
| | Y-Axis | Control the master volume (0-100%). |
| | Select (Short) | Activate **Greensleeves Mode**. |
| | Select (Long) | Reset the synthesizer to its default state. |
| **Accelerometer**| X-Axis (Tilt L/R) | Navigate the 24-position harmonic progression. |
| | Y-Axis (Tilt F/B)| Shift the octave up or down. |

## 4. System Architecture

### Hardware
-   **MCU:** TI MSPM0G3507
-   **Peripherals:**
    -   **DAC12:** For local audio output.
    -   **ADC12 (x2):** For reading the Joystick and Accelerometer.
    -   **UART (921600 baud):** For MIDI data transmission.
    -   **TIMG7 (16 kHz):** Serves as the master audio sample clock.
    -   **SysTick (100 Hz):** For debouncing buttons.
    -   **MATHACL:** Hardware co-processor used for sine wave generation.
-   **Pin Configuration:** All hardware pins are configured in `ti_msp_dl_config.syscfg`. Refer to `Komplett_Pin_Map.md` for a user-friendly guide.

### Software
-   **`main.c`:** The core of the application. Contains the main loop, all control logic, the audio generation ISR (`TIMG7_IRQHandler`), and MIDI message generation.
-   **`main.h`:** Defines the main `SynthState_t` data structure and includes the necessary library headers.
-   **`lib/`:** Contains libraries for controlling the audio engine and the EDU-MKII BoosterPack peripherals.
-   **`midi_handler.h`:** **(Unused)** This file contains a clean MIDI protocol implementation. However, the current version of `main.c` uses a redundant, copy-pasted implementation. This is a known architectural issue that should be refactored.
-   **`uart_audio_player.py`:** A Python script that acts as the MIDI client on a host PC. It auto-detects the serial port, receives MIDI data, and provides high-quality, polyphonic audio output.

## 5. How to Build and Run

### Embedded Firmware
1.  **IDE:** This project is intended for **Code Composer Studio (CCS)**.
2.  **Dependencies:** Ensure you have the MSPM0 SDK installed.
3.  **Build:** Import the project into CCS and build it. The build should produce no warnings.
4.  **Flash:** Use the CCS debugger to flash the firmware to the MSPM0G3507 LaunchPad.

### Python MIDI Player
1.  **Prerequisites:**
    -   Python 3
    -   Install required libraries:
        ```bash
        pip install pyserial sounddevice numpy
        ```
2.  **Connect:** Connect the LaunchPad to your PC via USB. The device will appear as a serial port.
3.  **Run:** Execute the script from your terminal:
    ```bash
    python uart_audio_player.py
    ```
4.  **Operation:** The script will automatically detect the serial port and the protocol (it defaults to MIDI). As you use the device, the Python script will play the corresponding notes through your computer's default audio output.

## 6. Known Issues
-   **Redundant MIDI Code:** `main.c` contains a full MIDI implementation that should be replaced by the code in `midi_handler.h`.
-   **Version Mismatch:** The firmware reports `v28.2.1` on the LCD at startup, but the feature set described in the comments is `v29.3`.
-   **Documentation:** Many of the documents in the `DOCS` folder are outdated and do not reflect the current architecture or feature set. This is being actively addressed.
