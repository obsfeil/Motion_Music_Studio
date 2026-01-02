/**
 * @file lcd_driver.c
 * @brief ST7735S LCD Driver Implementation - FIXED VERSION
 * @details Complete driver for CFAF128128B-0145T (128x128 TFT)
 *
 * FIXES APPLIED:
 * 1. Removed all duplicate SPI sends
 * 2. Proper delays for LCD initialization
 * 3. Correct RAMWR command handling
 */

#include "lcd_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ti_msp_dl_config.h"
#include "main.h"

//=============================================================================
// ST7735S COMMANDS
//=============================================================================
#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID 0x04
#define ST7735_RDDST 0x09

#define ST7735_SLPIN 0x10
#define ST7735_SLPOUT 0x11
#define ST7735_PTLON 0x12
#define ST7735_NORON 0x13

#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_RAMRD 0x2E

#define ST7735_PTLAR 0x30
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_RDID1 0xDA
#define ST7735_RDID2 0xDB
#define ST7735_RDID3 0xDC
#define ST7735_RDID4 0xDD

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

//=============================================================================
// INTERNAL HELPER MACROS - CORRECTED WITH INDIVIDUAL PORT NAMES
//=============================================================================

// RST pin (on PORTB pin 17)
#define LCD_RST_LOW()    DL_GPIO_clearPins(GPIO_LCD_RST_PORT, GPIO_LCD_RST_PIN)
#define LCD_RST_HIGH()   DL_GPIO_setPins(GPIO_LCD_RST_PORT, GPIO_LCD_RST_PIN)

// DC pin (on PORTA pin 13)
#define LCD_DC_COMMAND() DL_GPIO_clearPins(GPIO_LCD_DC_PORT, GPIO_LCD_DC_PIN)
#define LCD_DC_DATA()    DL_GPIO_setPins(GPIO_LCD_DC_PORT, GPIO_LCD_DC_PIN)

// CS pin (on PORTA pin 2)
#define LCD_CS_LOW()     DL_GPIO_clearPins(LCD_CS_PIN_PORT, LCD_CS_PIN_LCD_CS_PIN)
#define LCD_CS_HIGH()    DL_GPIO_setPins(LCD_CS_PIN_PORT, LCD_CS_PIN_LCD_CS_PIN)

#define LCD_BL_ON()   DL_GPIO_setPins(LCD_BL_PORT, LCD_BL_GIPO_LCD_BACKLIGHT_PIN)
#define LCD_BL_OFF()  DL_GPIO_clearPins(LCD_BL_PORT, LCD_BL_GIPO_LCD_BACKLIGHT_PIN)

// Delay macro (80 MHz clock: 80,000 cycles = 1 ms)
#define LCD_DELAY_MS(ms) DL_Common_delayCycles((uint32_t)(ms * 80000UL))

//=============================================================================
// SIMPLE 6x8 FONT (ASCII 32-127)
//=============================================================================
static const uint8_t font_6x8[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sp (32)
    0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, 0x00, // "
    0x14, 0x7f, 0x14, 0x7f, 0x14, 0x00, // #
    0x24, 0x2a, 0x7f, 0x2a, 0x12, 0x00, // $
    0x23, 0x13, 0x08, 0x64, 0x62, 0x00, // %
    0x36, 0x49, 0x55, 0x22, 0x50, 0x00, // &
    0x00, 0x05, 0x03, 0x00, 0x00, 0x00, // '
    0x00, 0x1c, 0x22, 0x41, 0x00, 0x00, // (
    0x00, 0x41, 0x22, 0x1c, 0x00, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, 0x00, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, // +
    0x00, 0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x00, // -
    0x00, 0x60, 0x60, 0x00, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, 0x00, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, 0x00, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, 0x00, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, 0x00, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, 0x00, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, 0x00, // =
    0x00, 0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, 0x00, // ?
    0x32, 0x49, 0x59, 0x51, 0x3E, 0x00, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, 0x00, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, // R
    0x46, 0x49, 0x49, 0x49, 0x31, 0x00, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00, // W
    0x63, 0x14, 0x08, 0x14, 0x63, 0x00, // X
    0x07, 0x08, 0x70, 0x08, 0x07, 0x00, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, 0x00, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00, // backslash
    0x00, 0x41, 0x41, 0x7F, 0x00, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, 0x00, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00, // _
    0x00, 0x01, 0x02, 0x04, 0x00, 0x00, // '
    0x20, 0x54, 0x54, 0x54, 0x78, 0x00, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, 0x00, // b
    0x38, 0x44, 0x44, 0x44, 0x20, 0x00, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, 0x00, // d
    0x38, 0x54, 0x54, 0x54, 0x18, 0x00, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, 0x00, // f
    0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, 0x00, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, 0x00, // n
    0x38, 0x44, 0x44, 0x44, 0x38, 0x00, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, 0x00, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, 0x00, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, 0x00, // r
    0x48, 0x54, 0x54, 0x54, 0x20, 0x00, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, 0x00, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00, // w
    0x44, 0x28, 0x10, 0x28, 0x44, 0x00, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, // z
};

//=============================================================================
// INTERNAL FUNCTIONS - FIXED: NO DUPLICATES!
//=============================================================================

/**
 * @brief Write a command byte to LCD
 * FIXED: Only sends byte ONCE!
 */
static void LCD_WriteCommand(uint8_t cmd) {
    LCD_CS_LOW();
    LCD_DC_COMMAND();
    
    // Send command (ONLY ONCE!)
    DL_SPI_transmitData8(SPI_LCD_INST, cmd);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    
    LCD_CS_HIGH();
}

/**
 * @brief Write a data byte to LCD
 * FIXED: Only sends byte ONCE!
 */
static void LCD_WriteData(uint8_t data) {
    LCD_CS_LOW();
    LCD_DC_DATA();
    
    // Send data (ONLY ONCE!)
    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    
    LCD_CS_HIGH();
}

/**
 * @brief Write multiple data bytes to LCD
 * FIXED: Only sends each byte ONCE!
 */
static void LCD_WriteDataBuffer(const uint8_t *buffer, uint32_t length) {
    LCD_CS_LOW();
    LCD_DC_DATA();
    
    for (uint32_t i = 0; i < length; i++) {
        // Send each byte ONLY ONCE!
        DL_SPI_transmitData8(SPI_LCD_INST, buffer[i]);
        while (DL_SPI_isBusy(SPI_LCD_INST));
    }
    
    LCD_CS_HIGH();
}

/**
 * @brief Set address window for pixel drawing
 */
static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address set
    LCD_WriteCommand(ST7735_CASET);
    LCD_WriteData(0x00);
    LCD_WriteData(x0);
    LCD_WriteData(0x00);
    LCD_WriteData(x1);

    // Row address set
    LCD_WriteCommand(ST7735_RASET);
    LCD_WriteData(0x00);
    LCD_WriteData(y0);
    LCD_WriteData(0x00);
    LCD_WriteData(y1);

    // Memory write command
    LCD_WriteCommand(ST7735_RAMWR);
}

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void LCD_Init(void) {
    // Hardware reset with proper delays
    LCD_RST_HIGH();
    LCD_DELAY_MS(10);
    LCD_RST_LOW();
    LCD_DELAY_MS(10);
    LCD_RST_HIGH();
    LCD_DELAY_MS(200);  // CRITICAL: Wait for LCD to power up
    
    LCD_BL_ON();

    // Software reset
    LCD_WriteCommand(ST7735_SWRESET);
    LCD_DELAY_MS(150);  // CRITICAL: Wait for reset to complete

    // Out of sleep mode
    LCD_WriteCommand(ST7735_SLPOUT);
    LCD_DELAY_MS(200);  // CRITICAL: Wait for sleep out

    // Frame rate control - normal mode
    LCD_WriteCommand(ST7735_FRMCTR1);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    // Frame rate control - idle mode
    LCD_WriteCommand(ST7735_FRMCTR2);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    // Frame rate control - partial mode
    LCD_WriteCommand(ST7735_FRMCTR3);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    // Display inversion control
    LCD_WriteCommand(ST7735_INVCTR);
    LCD_WriteData(0x07);

    // Power control
    LCD_WriteCommand(ST7735_PWCTR1);
    LCD_WriteData(0xA2);
    LCD_WriteData(0x02);
    LCD_WriteData(0x84);

    LCD_WriteCommand(ST7735_PWCTR2);
    LCD_WriteData(0xC5);

    LCD_WriteCommand(ST7735_PWCTR3);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x00);

    LCD_WriteCommand(ST7735_PWCTR4);
    LCD_WriteData(0x8A);
    LCD_WriteData(0x2A);

    LCD_WriteCommand(ST7735_PWCTR5);
    LCD_WriteData(0x8A);
    LCD_WriteData(0xEE);

    // VCOM control
    LCD_WriteCommand(ST7735_VMCTR1);
    LCD_WriteData(0x0E);

    // Display settings
    LCD_WriteCommand(ST7735_INVOFF);

    // Memory access control (rotation/mirror)
    LCD_WriteCommand(ST7735_MADCTL);
    LCD_WriteData(0xC8); // RGB order, row/col exchange

    // Color mode: 16-bit color (RGB565)
    LCD_WriteCommand(ST7735_COLMOD);
    LCD_WriteData(0x05);

    // Gamma correction
    LCD_WriteCommand(ST7735_GMCTRP1);
    LCD_WriteData(0x02);
    LCD_WriteData(0x1c);
    LCD_WriteData(0x07);
    LCD_WriteData(0x12);
    LCD_WriteData(0x37);
    LCD_WriteData(0x32);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2d);
    LCD_WriteData(0x29);
    LCD_WriteData(0x25);
    LCD_WriteData(0x2B);
    LCD_WriteData(0x39);
    LCD_WriteData(0x00);
    LCD_WriteData(0x01);
    LCD_WriteData(0x03);
    LCD_WriteData(0x10);

    LCD_WriteCommand(ST7735_GMCTRN1);
    LCD_WriteData(0x03);
    LCD_WriteData(0x1d);
    LCD_WriteData(0x07);
    LCD_WriteData(0x06);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x37);
    LCD_WriteData(0x3F);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x02);
    LCD_WriteData(0x10);

    // Normal display mode
    LCD_WriteCommand(ST7735_NORON);
    LCD_DELAY_MS(10);

    // Display on
    LCD_WriteCommand(ST7735_DISPON);
    LCD_DELAY_MS(100);  // CRITICAL: Wait for display to turn on

    // Clear screen
    LCD_FillScreen(LCD_COLOR_BLACK);
}

void LCD_FillScreen(uint16_t color) {
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;

    LCD_CS_LOW();
    LCD_DC_DATA();
    
    // FIXED: Send each byte ONLY ONCE!
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        DL_SPI_transmitData8(SPI_LCD_INST, color_high);
        while (DL_SPI_isBusy(SPI_LCD_INST));
        DL_SPI_transmitData8(SPI_LCD_INST, color_low);
        while (DL_SPI_isBusy(SPI_LCD_INST));
    }
    
    LCD_CS_HIGH();
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;

    LCD_SetWindow(x, y, x, y);
    
    // FIXED: Send color bytes ONLY ONCE!
    LCD_WriteData(color >> 8);
    LCD_WriteData(color & 0xFF);
}

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
    
    LCD_SetWindow(x, y, x + width - 1, y + height - 1);

    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;

    LCD_CS_LOW();
    LCD_DC_DATA();
    
    // FIXED: Send each pixel ONLY ONCE (2 bytes per pixel)!
    for (uint32_t i = 0; i < width * height; i++) {
        DL_SPI_transmitData8(SPI_LCD_INST, color_high);
        while (DL_SPI_isBusy(SPI_LCD_INST));
        DL_SPI_transmitData8(SPI_LCD_INST, color_low);
        while (DL_SPI_isBusy(SPI_LCD_INST));
    }
    
    LCD_CS_HIGH();
}

void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    LCD_DrawRect(x, y, length, 1, color);
}

void LCD_DrawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    LCD_DrawRect(x, y, 1, length, color);
}

void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = -abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;

    while (1) {
        LCD_DrawPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color) {
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    LCD_DrawPixel(x0, y0 + radius, color);
    LCD_DrawPixel(x0, y0 - radius, color);
    LCD_DrawPixel(x0 + radius, y0, color);
    LCD_DrawPixel(x0 - radius, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        LCD_DrawPixel(x0 + x, y0 + y, color);
        LCD_DrawPixel(x0 - x, y0 + y, color);
        LCD_DrawPixel(x0 + x, y0 - y, color);
        LCD_DrawPixel(x0 - x, y0 - y, color);
        LCD_DrawPixel(x0 + y, y0 + x, color);
        LCD_DrawPixel(x0 - y, y0 + x, color);
        LCD_DrawPixel(x0 + y, y0 - x, color);
        LCD_DrawPixel(x0 - y, y0 - x, color);
    }
}

void LCD_PrintString(uint16_t x, uint16_t y, const char *str, uint16_t fg_color,
                     uint16_t bg_color, LCD_FontSize_t size) {
    uint16_t char_width = 6 * size;
    uint16_t char_height = 8 * size;

    while (*str) {
        uint8_t c = *str++;
        if (c < 32 || c > 127)
            c = 32; // Replace invalid chars with space

        // Get font data for this character
        const uint8_t *font_char = &font_6x8[(c - 32) * 6];

        // Draw each column of the character
        for (uint8_t col = 0; col < 6; col++) {
            uint8_t col_data = font_char[col];

            // Draw each pixel in this column
            for (uint8_t row = 0; row < 8; row++) {
                uint16_t color = (col_data & (1 << row)) ? fg_color : bg_color;

                // Scale pixel if size > 1
                for (uint8_t sx = 0; sx < size; sx++) {
                    for (uint8_t sy = 0; sy < size; sy++) {
                        LCD_DrawPixel(x + col * size + sx, y + row * size + sy, color);
                    }
                }
            }
        }

        x += char_width;
        if (x + char_width > LCD_WIDTH)
            break; // Stop if text goes off screen
    }
}

void LCD_PrintNumber(uint16_t x, uint16_t y, int32_t num, uint16_t fg_color,
                     uint16_t bg_color, LCD_FontSize_t size) {
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%ld", (long)num);
    LCD_PrintString(x, y, buffer, fg_color, bg_color, size);
}

void LCD_PrintFloat(uint16_t x, uint16_t y, float num, uint8_t decimals,
                    uint16_t fg_color, uint16_t bg_color, LCD_FontSize_t size) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", (int)num);
    LCD_PrintString(x, y, buffer, fg_color, bg_color, size);
}

void LCD_SetBacklight(uint8_t brightness) {
    // Placeholder for PWM backlight control
    (void)brightness;
}

void LCD_SetDisplay(bool on) {
    LCD_WriteCommand(on ? ST7735_DISPON : ST7735_DISPOFF);
}

void LCD_SetRotation(uint8_t rotation) {
    LCD_WriteCommand(ST7735_MADCTL);
    switch (rotation % 4) {
    case 0:
        LCD_WriteData(0xC8); // 0° (default)
        break;
    case 1:
        LCD_WriteData(0x68); // 90°
        break;
    case 2:
        LCD_WriteData(0x08); // 180°
        break;
    case 3:
        LCD_WriteData(0xA8); // 270°
        break;
    }
}