/*
 * buttons.h
 * Håndterer S1, S2 og Joystick input
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>
#include <stdbool.h>

// Definer funksjoner som main.c får lov til å bruke
void Buttons_Init(void);
void Handle_GPIO_Interrupt(void); // Vi flytter interrupt-logikken hit

#endif /* BUTTONS_H_ */