/**
 * @file edumkii_joystick.c
 * @brief Joystick Implementation with Latching Logic
 */

#include "edumkii_joystick.h"
#include <stdint.h>
#include <stdbool.h>

// Statisk dørvakt som husker om vi har lov til å trigge nytt toneskifte
static bool joy_x_ready = true;

void Joystick_Init(Joystick_t *joy, uint16_t deadzone) {
    joy->deadzone = deadzone;
    joy->raw_x = 2048;
    joy->raw_y = 2048;
    joy->x_changed = false;
    joy->y_changed = false;
}

void Joystick_Update(Joystick_t *joy, uint16_t raw_x, uint16_t raw_y) {
    joy->raw_x = raw_x;
    joy->raw_y = raw_y;

    // --- JOY X LATCH LOGIC (Nøyaktig som v27) ---
    // Sjekk om spaken er i midten (Dødsone)
    if (raw_x > 1800 && raw_x < 2300) {
        joy_x_ready = true; // Senter -> Klar for nytt dytt
    }

    // Sett flagget true kun ved første dytt utenfor senter
    joy->x_changed = false; 
    if (joy_x_ready) {
        if (raw_x < 1000 || raw_x > 3000) {
            joy->x_changed = true;
            joy_x_ready = false; // LÅS! Må tilbake til midten før x_changed blir true igjen
        }
    }

    // --- JOY Y VOLUME LOGIC ---
    // Sjekk om spaken er langt nok unna senter til å endre volum
    int16_t deviation_y = (int16_t)raw_y - 2048;
    if (deviation_y < -300 || deviation_y > 300) {
        joy->y_changed = true;
    } else {
        joy->y_changed = false;
    }
}

uint8_t Joystick_GetVolume(Joystick_t *joy) {
    // Returnerer volum 0-100 basert på Joy Y posisjon
    uint32_t vol = ((uint32_t)joy->raw_y * 100) / 4095;
    if (vol > 100) vol = 100;
    return (uint8_t)vol;
}

// Hjelpefunksjon for å finne ut hvilken vei vi dyttet
int8_t Joystick_GetDirectionX(Joystick_t *joy) {
    if (joy->raw_x < 1000) return -1; // Venstre
    if (joy->raw_x > 3000) return 1;  // Høyre
    return 0;
}