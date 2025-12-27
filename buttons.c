
/*
 * buttons.c
 */

#include "buttons.h"
#include "ti_msp_dl_config.h" // For GPIO definisjoner
#include "main.h"             // For å få tilgang til gSynthState og funksjoner

// Lokale variabler (kun synlig inne i denne filen)
static volatile uint32_t s1_debounce_timer = 0;

// Initialisering (kalles én gang fra main)
void Buttons_Init(void) {
    // Slett eventuelle gamle interrupts
    DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, 
                                 GPIO_BUTTONS_S1_PIN | GPIO_BUTTONS_S2_PIN);
    
    // Aktiver interrupts
    DL_GPIO_enableInterrupt(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN);
    DL_GPIO_enableInterrupt(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN);
}

// Dette var tidligere GPIOA_IRQHandler innholdet
void Handle_GPIO_Interrupt(void) {
    uint32_t pending = DL_GPIO_getEnabledInterruptStatus(
        GPIO_BUTTONS_PORT, 
        GPIO_BUTTONS_S1_PIN | GPIO_BUTTONS_S2_PIN
    );

    // --- S1 KNAPP (Endre Instrument) ---
    if (pending & GPIO_BUTTONS_S1_PIN) {
        Change_Instrument(); // Denne funksjonen må være synlig (se under)
        DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_PIN);
    }

    // --- S2 KNAPP (Play/Stop) ---
    if (pending & GPIO_BUTTONS_S2_PIN) {
        gSynthState.audio_playing = !gSynthState.audio_playing;
        
        if (gSynthState.audio_playing) {
            DL_GPIO_setPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_On();
        } else {
            DL_GPIO_clearPins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
            Trigger_Note_Off();
        }
        DL_GPIO_clearInterruptStatus(GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_PIN);
    }
}