/**
 * @file lcd_driver.h
 * @brief ST7735S LCD Driver for CFAF128128B-0145T (128x128 TFT)
 * @details BoosterPack MKII Educational LCD Driver
 * 
 * Pin Configuration (from SysConfig):
 * - SPI: SPI1 (SPI_LCD)
 * - CS: Auto via SPI peripheral
 * - RST: GPIO_LCD_RST
 * - DC: GPIO_LCD_DC
 * 
 * @version 1.0.0
 * @date 2025-12-23
 */

#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

//=============================================================================
// LCD SPECIFICATIONS
//=============================================================================
#define LCD_WIDTH           128
#define LCD_HEIGHT          128
#define LCD_PIXEL_COUNT     (LCD_WIDTH * LCD_HEIGHT)

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
#define LCD_COLOR_NAVY       0x000F   // Dark blue
#define LCD_COLOR_ORANGE     0xFD20   // Orange
#define LCD_COLOR_DARKRED    0x8800   // Dark red
#define LCD_COLOR_DARKGREEN  0x03E0   // Dark green
#define LCD_COLOR_DARKGRAY   0x4208   // Dark gray

//=============================================================================
// FONT SIZE ENUM
//=============================================================================
typedef enum {
    FONT_SMALL = 1,   // 6x8 pixels
    FONT_MEDIUM = 2,  // 12x16 pixels
    FONT_LARGE = 3    // 18x24 pixels
} LCD_FontSize_t;

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================

/**
 * @brief Initialize LCD controller and clear screen
 * @note Must be called before any other LCD functions
 */
void LCD_Init(void);

/**
 * @brief Fill entire screen with a color
 * @param color RGB565 color value
 */
void LCD_FillScreen(uint16_t color);

/**
 * @brief Draw a single pixel
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-127)
 * @param color RGB565 color
 */
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Draw a filled rectangle
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color RGB565 color
 */
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/**
 * @brief Draw a horizontal line
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param length Line length
 * @param color RGB565 color
 */
void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);

/**
 * @brief Draw a vertical line
 * @param x X coordinate
 * @param y Starting Y coordinate
 * @param length Line length
 * @param color RGB565 color
 */
void LCD_DrawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);

/**
 * @brief Draw a line between two points
 * @param x0 Start X coordinate
 * @param y0 Start Y coordinate
 * @param x1 End X coordinate
 * @param y1 End Y coordinate
 * @param color RGB565 color
 */
void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief Draw a circle
 * @param x0 Center X coordinate
 * @param y0 Center Y coordinate
 * @param radius Circle radius
 * @param color RGB565 color
 */
void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color);

/**
 * @brief Print a string at specified position
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param str Null-terminated string
 * @param fg_color Foreground color (text)
 * @param bg_color Background color
 * @param size Font size (FONT_SMALL, FONT_MEDIUM, FONT_LARGE)
 */
void LCD_PrintString(uint16_t x, uint16_t y, const char* str, 
                     uint16_t fg_color, uint16_t bg_color, LCD_FontSize_t size);

/**
 * @brief Print a number at specified position
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param num Number to print
 * @param fg_color Foreground color
 * @param bg_color Background color
 * @param size Font size
 */
void LCD_PrintNumber(uint16_t x, uint16_t y, int32_t num, 
                     uint16_t fg_color, uint16_t bg_color, LCD_FontSize_t size);

/**
 * @brief Print a floating-point number
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param num Number to print
 * @param decimals Number of decimal places
 * @param fg_color Foreground color
 * @param bg_color Background color
 * @param size Font size
 */
void LCD_PrintFloat(uint16_t x, uint16_t y, float num, uint8_t decimals,
                    uint16_t fg_color, uint16_t bg_color, LCD_FontSize_t size);

/**
 * @brief Convert RGB888 to RGB565
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @return uint16_t RGB565 color value
 */
static inline uint16_t LCD_RGB(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Set backlight brightness (if PWM configured)
 * @param brightness 0-100%
 */
void LCD_SetBacklight(uint8_t brightness);

/**
 * @brief Turn display on/off
 * @param on true = display on, false = display off
 */
void LCD_SetDisplay(bool on);

/**
 * @brief Set display orientation
 * @param rotation 0, 1, 2, or 3 (90° increments)
 */
void LCD_SetRotation(uint8_t rotation);

#endif /* LCD_DRIVER_H_ */