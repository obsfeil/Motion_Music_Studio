/**
 * @file lcd_driver.h
 * @brief ST7735S LCD Driver - Minimal Header for Compilation
 */

#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// LCD SPECIFICATIONS
//=============================================================================
#define LCD_WIDTH           128
#define LCD_HEIGHT          128

//=============================================================================
// COLOR DEFINITIONS (RGB565 format)
//=============================================================================
#define LCD_COLOR_BLACK     0x0000
#define LCD_COLOR_WHITE     0xFFFF
#define LCD_COLOR_RED       0xF800
#define LCD_COLOR_GREEN     0x07E0
#define LCD_COLOR_BLUE      0x001F
#define LCD_COLOR_YELLOW    0xFFE0
#define LCD_COLOR_CYAN      0x07FF
#define LCD_COLOR_MAGENTA   0xF81F
#define LCD_COLOR_ORANGE    0xFD20
#define LCD_COLOR_PURPLE    0x801F
#define LCD_COLOR_GRAY      0x8410
#define LCD_COLOR_DARKGRAY  0x4208

//=============================================================================
// FONT SIZE ENUM
//=============================================================================
typedef enum {
    FONT_SMALL = 1,
    FONT_MEDIUM = 2,
    FONT_LARGE = 3
} LCD_FontSize_t;

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
void LCD_Init(void);
void LCD_FillScreen(uint16_t color);
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void LCD_PrintString(uint16_t x, uint16_t y, const char* str, 
                     uint16_t fg_color, uint16_t bg_color, LCD_FontSize_t size);
void LCD_PrintNumber(uint16_t x, uint16_t y, int32_t num, 
                     uint16_t fg_color, uint16_t bg_color, LCD_FontSize_t size);

#endif /* LCD_DRIVER_H_ */