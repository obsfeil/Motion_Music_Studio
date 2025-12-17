<<<<<<< HEAD
## Example Summary

Empty project using DriverLib.
This example shows a basic empty project using DriverLib with just main file
and SysConfig initialization.

## Peripherals & Pin Assignments

| Peripheral | Pin | Function |
| --- | --- | --- |
| DEBUGSS | PA20 | Debug Clock |
| DEBUGSS | PA19 | Debug Data In Out |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0G3507](https://www.ti.com/tool/LP-MSPM0G3507) for LaunchPad information, including user guide and hardware files.

| Pin | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| --- | --- | --- | --- | --- |
| PA20 | DEBUGSS | SWCLK | N/A | <ul><li>PA20 is used by SWD during debugging<br><ul><li>`J101 15:16 ON` Connect to XDS-110 SWCLK while debugging<br><li>`J101 15:16 OFF` Disconnect from XDS-110 SWCLK if using pin in application</ul></ul> |
| PA19 | DEBUGSS | SWDIO | N/A | <ul><li>PA19 is used by SWD during debugging<br><ul><li>`J101 13:14 ON` Connect to XDS-110 SWDIO while debugging<br><li>`J101 13:14 OFF` Disconnect from XDS-110 SWDIO if using pin in application</ul></ul> |

### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0G3507 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP-MSPM0G3507 User's Guide](https://www.ti.com/lit/slau873).

## Example Usage

Compile, load and run the example.
=======
# 🎵 Motion Music Studio

**Motion Music Studio** er en embedded synthesizer bygget på Texas Instruments **MSPM0G3507 LaunchPad**. Prosjektet bruker **BOOSTXL-EDUMKII** for å skape et interaktivt musikkinstrument som styres av joystick, knapper og bevegelse (akselerometer).

Prosjektet er utviklet i Code Composer Studio (Theia) med MSPM0 SDK.

---

## ✨ Funksjoner

* **Lydmotor:** Wavetable-syntese med 4 bølgeformer (Sinus, Sagtan, Firkant, Trekant).
* **Høy kvalitet:** Oppgradert til **12-bit PWM** lydutgang (80 MHz systemklokke).
* **Kontroll:**
    * 🕹️ **Joystick:** Styrer tonehøyde (Pitch) og volum/modulasjon.
    * 👋 **Akselerometer (BMI160):** Endrer lyden basert på hvordan du vipper brettet (Pitch bend / Filter).
    * 🔘 **Knapper:** Bytter instrumenter og moduser.
* **Feedback:**
    * 🌈 **RGB LED:** Visuell respons på lyd og modus.
    * 🖥️ **LCD Skjerm:** Viser bølgeform og status (WIP).

---

## 🛠️ Maskinvare

For å kjøre dette prosjektet trenger du:
1.  **MCU:** [LP-MSPM0G3507 LaunchPad](https://www.ti.com/tool/LP-MSPM0G3507)
2.  **Add-on:** [Educational BoosterPack MKII (BOOSTXL-EDUMKII)](https://www.ti.com/tool/BOOSTXL-EDUMKII)

### 🔌 Pin-konfigurasjon (PinMap)
Dette oppsettet er konfigurert via SysConfig for å matche EDUMKII:

| Komponent | Funksjon | Pinne (MSPM0) |
| :--- | :--- | :--- |
| **Lyd** | PWM Buzzer | **PB13** |
| **Joystick** | Analog Y (Pitch) | **PA25** |
| **Joystick** | Analog X (Mod) | **PB2** |
| **Knapper** | S1 (Venstre) | **PA18** |
| **Knapper** | S2 (Høyre) | **PB21** |
| **Sensorer** | I2C SDA (Accel/Lys) | **PA0** |
| **Sensorer** | I2C SCL (Accel/Lys) | **PA1** |
| **RGB LED** | Rød / Grønn / Blå | **PB26 / PB22 / PB27** |
| **LCD** | SPI (CLK/MOSI) | **PB9 / PB8** |
| **LCD** | Control (RS/CS
>>>>>>> c05fb7a19cb98a6730d0eb09a722c70b8a873413
