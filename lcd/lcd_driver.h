/**
 * @file lcd_driver.h
 * @brief ST7735 LCD Driver for MSPM0G3507
 */

#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

//=============================================================================
// COLOR DEFINITIONS (RGB565 format)
//=============================================================================
typedef uint16_t Color_t;

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_ORANGE  0xFC00
#define COLOR_PURPLE  0x8010

//=============================================================================
// LCD DIMENSIONS
//=============================================================================
#define LCD_WIDTH  128
#define LCD_HEIGHT 128

//=============================================================================
// PUBLIC FUNCTION PROTOTYPES
//=============================================================================

/**
 * @brief Initialize LCD display
 */
void LCD_Init(void);

/**
 * @brief Clear entire screen with color
 */
void LCD_Clear(Color_t color);

/**
 * @brief Draw a single pixel
 */
void LCD_DrawPixel(uint16_t x, uint16_t y, Color_t color);

/**
 * @brief Draw a character
 */
void LCD_DrawChar(uint16_t x, uint16_t y, char c, Color_t color);

/**
 * @brief Draw a string
 */
void LCD_DrawString(uint16_t x, uint16_t y, const char* str, Color_t color);

/**
 * @brief Draw a line
 */
void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color_t color);

/**
 * @brief Draw a rectangle outline
 */
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color_t color);

/**
 * @brief Draw a filled rectangle
 */
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color_t color);

/**
 * @brief Draw a circle outline
 */
void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r, Color_t color);

/**
 * @brief Draw a filled circle
 */
void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r, Color_t color);

#endif // LCD_DRIVER_H
