/*
 * buttons.h
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>
#include <stdbool.h>
#include "main.h"              // ← Gir tilgang til ALT fra main.h
#include "ti_msp_dl_config.h"

extern volatile uint32_t g_system_ticks;
// Funksjoner
void Buttons_Init(void);
void Handle_GPIO_Interrupt(void);

#endif /* BUTTONS_H_ */