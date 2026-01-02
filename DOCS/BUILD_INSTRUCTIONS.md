# 🛠️ Build & Run Instructions

This guide provides instructions for building the embedded firmware and running the companion Python MIDI player.

## 1. Embedded Firmware (Code Composer Studio)

### Prerequisites
-   **Texas Instruments Code Composer Studio (CCS):** An up-to-date version.
-   **MSPM0 SDK:** The appropriate Software Development Kit for the MSPM0G3507, installed and recognized by CCS.

### Build Steps
1.  **Import Project:**
    -   Open CCS.
    -   Go to `File` -> `Import...`.
    -   Select `CCS` -> `CCS Projects`.
    -   Browse to the root directory of this project (`Motion_Music_studio`).
    -   Ensure the project is checked and click `Finish`.

2.  **Verify SysConfig Generation:**
    -   The project relies on TI's SysConfig tool to generate hardware configuration files (`ti_msp_dl_config.h/c`).
    -   If you encounter errors like `"ti_msp_dl_config.h not found"`, double-click the `ti_msp_dl_config.syscfg` file in the project explorer.
    -   Let the SysConfig GUI open, then save the file (`Ctrl+S`). This will force the generation of the necessary source files.

3.  **Build the Project:**
    -   Right-click on the project in the Project Explorer.
    -   Select `Build Project` (or press `Ctrl+B`).
    -   The build should complete with **0 errors**.

### Flashing and Running
1.  **Connect Hardware:**
    -   Connect the BOOSTXL-EDUMKII to the MSPM0G3507 LaunchPad.
    -   Connect the LaunchPad to your computer via the USB debugger port.

2.  **Start Debug Session:**
    -   Press `F11` or click the "Debug" icon in CCS.
    -   CCS will build the project (if needed), establish a connection with the board, and flash the firmware.

3.  **Run the Code:**
    -   Once the debugger is loaded and paused at the start of `main`, press `F8` or the "Resume" button to run the code.
    -   The synthesizer should now be active. You can hear audio by connecting headphones or a speaker to the **headphone jack** on the BOOSTXL-EDUMKII.

## 2. Python MIDI Player (PC Client)

The device simultaneously transmits MIDI data over UART. The Python script allows you to hear this MIDI data played through a high-quality polyphonic synthesizer on your computer.

### Prerequisites
-   **Python 3:** An up-to-date installation of Python.
-   **Required Libraries:** You need `pyserial`, `sounddevice`, and `numpy`. Install them using pip:
    ```bash
    pip install pyserial sounddevice numpy
    ```

### Running the Script
1.  **Connect the Device:**
    -   Ensure the LaunchPad is connected to your PC via USB. The device's Application UART will enumerate as a COM port.

2.  **Execute the Script:**
    -   Open a terminal or command prompt.
    -   Navigate to the root directory of the project.
    -   Run the `uart_audio_player.py` script:
        ```bash
        python uart_audio_player.py
        ```

3.  **Operation:**
    -   The script will automatically scan for available serial ports and attempt to connect to the correct one.
    -   It will then enter MIDI mode and wait for data.
    -   As you operate the device (change notes, instruments, etc.), the Python script will play the corresponding sounds through your computer's default audio output.
    -   Press `Ctrl+C` in the terminal to stop the script.

## 🐛 Troubleshooting

-   **Build Error: "ti_msp_dl_config.h not found"**
    -   As mentioned above, open `ti_msp_dl_config.syscfg` in CCS and save it to force file generation.

-   **Build Error: "Cannot find -ldriverlib" or similar linking errors**
    -   This indicates that the project is not correctly linked to the MSPM0 SDK.
    -   Right-click the project -> `Properties`.
    -   Under `Build` -> `ARM Linker` -> `File Search Path`, ensure that the path to the SDK's driver library (`.../source/ti/driverlib/lib/ccs/m0p/ti-driverlib.a`) is correctly included.

-   **Python Script: "No serial ports found!"**
    -   Ensure the LaunchPad is properly connected and that its drivers are installed. Check your operating system's device manager to see if the COM port is visible.

-   **Python Script: "Access is denied" on COM port**
    -   Make sure no other application (like a serial monitor in CCS or another terminal) is using the COM port.