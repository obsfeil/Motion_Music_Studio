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
