/**
 * @file edumkii.h
 * @brief EDUMKII Hardware Abstraction Library - Main Include
 * @version 1.0.0
 * @date 2025-12-29
 * 
 * This library provides hardware abstraction for the BOOSTXL-EDUMKII
 * BoosterPack, making it easy to reuse code across projects.
 * 
 * Features:
 * - Button state machine (short/long/double click)
 * - Joystick with deadzone filtering
 * - Accelerometer with deadzone filtering
 * - Clean, simple API
 * 
 * Usage:
 *   #include "edumkii.h"
 * 
 * Hardware Requirements:
 * - BOOSTXL-EDUMKII BoosterPack
 * - MSPM0G3507 LaunchPad
 * - Configured with SysConfig (.syscfg)
 * 
 * @author Your Name
 * @copyright MIT License
 */

#ifndef EDUMKII_H_
#define EDUMKII_H_

//=============================================================================
// INCLUDES
//=============================================================================

#include "edumkii_buttons.h"
#include "edumkii_joystick.h"
#include "edumkii_accel.h"

//=============================================================================
// VERSION INFO
//=============================================================================

#define EDUMKII_LIB_VERSION_MAJOR 1
#define EDUMKII_LIB_VERSION_MINOR 0
#define EDUMKII_LIB_VERSION_PATCH 0
#define EDUMKII_LIB_VERSION "1.0.0"

//=============================================================================
// QUICK START GUIDE
//=============================================================================

/*
 * STEP 1: Include this header
 * ----------------------------
 * #include "edumkii.h"
 * 
 * 
 * STEP 2: Create hardware objects
 * --------------------------------
 * Button_t btn_s1, btn_s2, btn_joy_sel;
 * Joystick_t joystick;
 * Accelerometer_t accel;
 * 
 * 
 * STEP 3: Initialize in main()
 * -----------------------------
 * Button_Init(&btn_s1);
 * Button_Init(&btn_s2);
 * Button_Init(&btn_joy_sel);
 * Joystick_Init(&joystick, 100);  // 100 = deadzone
 * Accel_Init(&accel, 100);
 * 
 * 
 * STEP 4: Update in SysTick (100 Hz)
 * -----------------------------------
 * void SysTick_Handler(void) {
 *     Button_Update(&btn_s1, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S1_MKII_PIN);
 *     Button_Update(&btn_s2, GPIO_BUTTONS_PORT, GPIO_BUTTONS_S2_MKII_PIN);
 *     Button_Update(&btn_joy_sel, GPIO_BUTTONS_PORT, GPIO_BUTTONS_JOY_SEL_PIN);
 * }
 * 
 * 
 * STEP 5: Update joystick & accel in main loop
 * ---------------------------------------------
 * while (1) {
 *     Joystick_Update(&joystick, gSynthState.joy_x, gSynthState.joy_y);
 *     Accel_Update(&accel, gSynthState.accel_x, gSynthState.accel_y, gSynthState.accel_z);
 *     
 *     // Use the data...
 *     if (joystick.x_changed) {
 *         uint8_t key = Joystick_GetKeyIndex(&joystick, 7);
 *     }
 * }
 * 
 * 
 * STEP 6: Handle button events
 * -----------------------------
 * ButtonEvent_t event = Button_GetEvent(&btn_s1);
 * switch (event) {
 *     case BTN_EVENT_SHORT_CLICK:
 *         // Do something
 *         break;
 *     case BTN_EVENT_LONG_PRESS:
 *         // Do something else
 *         break;
 *     case BTN_EVENT_DOUBLE_CLICK:
 *         // Do another thing
 *         break;
 * }
 * 
 * 
 * See example_main.c for complete working example!
 */

#endif /* EDUMKII_H_ */
