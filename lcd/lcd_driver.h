/**
 * @file lcd_driver.h
 * @brief ST7735 LCD Driver for MSPM0G3507
 * 
 * 128x160 RGB LCD driver using SPI
 * Compatible with BOOSTXL-EDUMKII
 */

#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// LCD CONSTANTS
//=============================================================================

#define LCD_WIDTH           128
#define LCD_HEIGHT          160

//=============================================================================
// COLORS (16-bit RGB565)
//=============================================================================

#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_ORANGE        0xFC00
#define COLOR_GRAY          0x8410
#define COLOR_DARKGRAY      0x4208

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================

void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_DrawPixel(uint8_t x, uint8_t y, uint16_t color);
void LCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void LCD_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void LCD_DrawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color);
void LCD_FillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color);
void LCD_DrawChar(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg);
void LCD_DrawString(uint8_t x, uint8_t y, const char* str, uint16_t color);
void LCD_DrawWaveform(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t waveform);
void LCD_DrawSpectrum(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const float* fft_data, uint8_t bins);

#endif /* LCD_DRIVER_H_ */
