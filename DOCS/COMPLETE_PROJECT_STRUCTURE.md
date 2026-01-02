# 📂 Complete Project Structure

This document outlines the actual structure of the **Motion Music Studio** project as of version 29.3.

## 📁 Root Directory

```
Motion_Music_studio/
├── main.c
├── main.h
├── midi_handler.h
├── ti_msp_dl_config.syscfg
├── uart_audio_player.py
├── lib/
├── DOCS/
├── ti/
├── .cproject
└── ... (IDE and config files)
```

## Key Files & Directories

### 📄 `main.c`
-   **The Core of the Application.** This massive file contains almost all of the project's logic.
-   **Responsibilities:**
    -   The main application loop (`while(1)`).
    -   Initialization of all hardware peripherals.
    -   Reading and processing all inputs (buttons, joystick, accelerometer).
    -   The main audio generation Interrupt Service Routine (`TIMG7_IRQHandler`).
    -   The synthesis engine, including waveform generation, effects, and envelopes.
    -   The musical logic for scales, chords, and the harmonic progression system.
    -   The "Greensleeves" performance mode logic.
    -   A **complete, self-contained MIDI protocol implementation** for sending messages over UART.
    -   All LCD drawing and UI update logic.

### 📄 `main.h`
-   Defines the global `SynthState_t` struct, which holds the volatile state of the system.
-   Includes the necessary headers from the `lib/` directory to make them available to `main.c`.

### 📄 `ti_msp_dl_config.syscfg`
-   **The Hardware Abstraction Layer.** This is a TI SysConfig file that defines the complete hardware configuration for the MSPM0G3507 MCU.
-   It is the **source of truth** for all pin mappings, peripheral settings, and clock configurations.
-   Generates `ti_msp_dl_config.c` and `ti_msp_dl_config.h` during the build process.

### 🐍 `uart_audio_player.py`
-   A companion Python script that runs on a host PC.
-   It connects to the device's serial port, receives the MIDI messages sent by `main.c`, and plays them through a high-quality, 8-voice polyphonic software synthesizer.
-   This script is essential for using the device as a MIDI controller.

### ⚠️ `midi_handler.h` (Architectural Note)
-   This header file contains a clean, well-structured implementation of the MIDI protocol.
-   **Crucially, it is NOT USED by `main.c`.**
-   The main application uses a redundant, copy-pasted implementation of the same MIDI logic. This is a significant architectural flaw that should be addressed in a future refactoring. The goal would be to remove the duplicate code from `main.c` and have it call the functions defined in this header.

### 📁 `lib/` Directory
-   Contains pre-written libraries for interfacing with hardware components.
-   **`lib/audio/`**: A basic audio engine library.
    -   `audio_engine.c/.h`: Provides waveform lookup tables (Sine, Saw, etc.) and a phase accumulator.
    -   `audio_envelope.c/.h`: Provides an ADSR envelope generator.
    -   `audio_filters.c/.h`: Provides simple audio filters like low-pass and soft clipping.
-   **`lib/edumkii/`**: A driver library for the BOOSTXL-EDUMKII.
    -   `edumkii.h`: Main include file.
    -   `edumkii_accel.c/.h`: Accelerometer driver.
    -   `edumkii_buttons.c/.h`: Button driver with debouncing.
    -   `edumkii_joystick.c/.h`: Joystick driver.

### 📁 `DOCS/` Directory
-   Contains all project documentation, including this file.
-   Key documents include:
    -   `README.md`: A high-level overview of the project.
    -   `CHANGELOG.md`: A log of new features and changes.
    -   `BUILD_INSTRUCTIONS.md`: Guide for building and running the project.
    -   `Komplett_Pin_Map.md`: The definitive hardware pin map.

### 📁 `ti/` Directory
-   Contains the low-level **TI Driver Library** source files, which are part of the MSPM0 SDK. These files provide the APIs (e.g., `DL_GPIO_setPins`, `DL_ADC12_getMemResult`) used throughout the project to control the MCU's peripherals.