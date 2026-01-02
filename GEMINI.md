# GEMINI.md - Motion Music Studio

## Project Overview

**Motion Music Studio** is an embedded synthesizer built on the Texas Instruments **MSPM0G3507 LaunchPad**. The project uses the **BOOSTXL-EDUMKII** for an interactive music experience controlled by a joystick, buttons, and an accelerometer.

The synthesizer features a wavetable synthesis audio engine with four waveforms (Sine, Sawtooth, Square, Triangle). The latest version (v31.0) boasts significant audio quality improvements, including a 48 kHz sample rate, a MATHACL biquad anti-aliasing filter, linear interpolation for smoother output, and an OPA buffer for direct speaker output.

Controls are mapped as follows:
*   **Joystick:** Controls pitch and volume/modulation.
*   **Accelerometer:** Modifies the sound based on the board's tilt (pitch bend/filter).
*   **Buttons:** Switch instruments and modes.
*   **RGB LED and LCD:** Provide visual feedback.

The project is developed in Code Composer Studio (Theia) with the MSPM0 SDK.

## Building and Running

The following instructions are based on the `V31_COMPLETE_UPGRADE_GUIDE.md` file.

### 1. Backup Current Files

Before making any changes, it's recommended to back up your main files:

```bash
copy main.c main_v30_backup.c
copy ti_msp_dl_config.syscfg ti_msp_dl_config_v30_backup.syscfg
```

### 2. Replace Files with v31.0

To upgrade to the latest version, copy the new files:

```bash
copy ti_msp_dl_config_48KHZ_OPA.syscfg ti_msp_dl_config.syscfg
copy main_48KHZ_COMPLETE.c main.c
copy audio_engine_FIXED.c lib\audio\audio_engine.c
```

### 3. Clean and Build the Project

In Code Composer Studio:

1.  Right-click the project and select **Clean Project**.
2.  Once the clean is complete, go to **Project > Build All** (or press F7).

The project should build without any errors.

### 4. Flash and Test

1.  Go to **Debug > Flash** to flash the program to the LaunchPad.
2.  Reset the board.
3.  Use the joystick and buttons to play notes and test the audio output.

## Development Conventions

The codebase demonstrates several good development practices:

*   **Clear and Detailed Comments:** The `main.c` file is extensively commented, explaining the purpose of different code sections, button controls, audio output, and various musical and technical features.
*   **Modular Structure:** The project is organized into modules for different functionalities, such as `lcd_driver`, `audio_engine`, and `edumkii` (for the BoosterPack).
*   **Header Guards:** Header files use `#ifndef`/`#define`/`#endif` guards to prevent multiple inclusions.
*   **Descriptive Naming:** Variables, functions, and constants are given clear and descriptive names, which improves code readability.
*   **Use of `static inline`:** The project uses `static inline` for small, performance-critical functions to reduce function call overhead.
*   **State-Driven Logic:** The main loop is state-driven, using a `SynthState_t` struct to manage the synthesizer's current state.

## Key Files

*   `README.md`: Provides a high-level overview of the project, its features, and the required hardware.
*   `main.c`: The main application file, containing the core logic for the synthesizer, including audio generation, control handling, and the main loop.
*   `V31_COMPLETE_UPGRADE_GUIDE.md`: A detailed guide for upgrading the project to version 31.0, including instructions for building and running the project.
*   `ti_msp_dl_config.syscfg`: The system configuration file for the MSPM0 microcontroller, used to configure peripherals like timers, ADC, DAC, and OPA.
*   `lib/`: This directory contains the core libraries for the audio engine, and the EDUMKII BoosterPack.
*   `DOCS/`: This directory contains additional documentation for the project.
