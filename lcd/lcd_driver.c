/**
 * @file lcd_driver.c
 * @brief ST7735 LCD Driver Implementation
 */

#include "ti_msp_dl_config.h"  // SysConfig generated
#include "lcd_driver.h"         // LCD interface
#include <string.h>
#include <math.h>
#include "../main.h"            // Parent directory


//=============================================================================
// PRIVATE CONSTANTS
//=============================================================================

// ST7735 Commands
#define ST7735_NOP          0x00
#define ST7735_SWRESET      0x01
#define ST7735_SLPOUT       0x11
#define ST7735_NORON        0x13
#define ST7735_INVOFF       0x20
#define ST7735_DISPON       0x29
#define ST7735_CASET        0x2A
#define ST7735_RASET        0x2B
#define ST7735_RAMWR        0x2C
#define ST7735_COLMOD       0x3A
#define ST7735_MADCTL       0x36
#define ST7735_FRMCTR1      0xB1

//=======================================================================
// SIMPLE 5x7 FONT
//=============================================================================

static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

//=============================================================================
// PRIVATE FUNCTIONS
//=============================================================================

static inline void LCD_DC_Command(void) {
    DL_GPIO_clearPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN);
}

static inline void LCD_DC_Data(void) {
    DL_GPIO_setPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN);
}

static void LCD_WriteCommand(uint8_t cmd) {
    LCD_DC_Command();
    DL_SPI_transmitData8(SPI_LCD_INST, cmd);
    while (DL_SPI_isBusy(SPI_LCD_INST));
}

static void LCD_WriteData(uint8_t data) {
    LCD_DC_Data();
    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST));
}

static void LCD_WriteData16(uint16_t data) {
    LCD_WriteData(data >> 8);
    LCD_WriteData(data & 0xFF);
}

static void LCD_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    LCD_WriteCommand(ST7735_CASET);
    LCD_WriteData16(x0);
    LCD_WriteData16(x1);
    
    LCD_WriteCommand(ST7735_RASET);
    LCD_WriteData16(y0);
    LCD_WriteData16(y1);
    
    LCD_WriteCommand(ST7735_RAMWR);
}

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void LCD_Init(void) {
    // Reset
    DL_GPIO_clearPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN);
    delay_ms(20);
    DL_GPIO_setPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN);
    delay_ms(120);
    
    // Software reset
    LCD_WriteCommand(ST7735_SWRESET);
    delay_ms(150);
    
    // Sleep out
    LCD_WriteCommand(ST7735_SLPOUT);
    delay_ms(120);
    
    // Frame rate
    LCD_WriteCommand(ST7735_FRMCTR1);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    
    // Color mode (16-bit)
    LCD_WriteCommand(ST7735_COLMOD);
    LCD_WriteData(0x05);
    
    // Memory access
    LCD_WriteCommand(ST7735_MADCTL);
    LCD_WriteData(0xC8);
    
    // Normal display
    LCD_WriteCommand(ST7735_NORON);
    delay_ms(10);
    
    // Display on
    LCD_WriteCommand(ST7735_DISPON);
    delay_ms(10);
}

void LCD_Clear(uint16_t color) {
    LCD_SetWindow(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);
    
    for (uint16_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        LCD_WriteData16(color);
    }
}

void LCD_DrawPixel(uint8_t x, uint8_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    LCD_SetWindow(x, y, x, y);
    LCD_WriteData16(color);
}

void LCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int16_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int16_t steps = (dx > dy) ? dx : dy;
    
    float x_inc = (float)(x1 - x0) / steps;
    float y_inc = (float)(y1 - y0) / steps;
    
    float x = x0;
    float y = y0;
    
    for (int i = 0; i <= steps; i++) {
        LCD_DrawPixel((uint8_t)x, (uint8_t)y, color);
        x += x_inc;
        y += y_inc;
    }
}

void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    LCD_DrawLine(x, y, x+w-1, y, color);
    LCD_DrawLine(x+w-1, y, x+w-1, y+h-1, color);
    LCD_DrawLine(x+w-1, y+h-1, x, y+h-1, color);
    LCD_DrawLine(x, y+h-1, x, y, color);
}

void LCD_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    LCD_SetWindow(x, y, x+w-1, y+h-1);
    
    for (uint16_t i = 0; i < w * h; i++) {
        LCD_WriteData16(color);
    }
}

void LCD_DrawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        LCD_DrawPixel(x0 + x, y0 + y, color);
        LCD_DrawPixel(x0 + y, y0 + x, color);
        LCD_DrawPixel(x0 - y, y0 + x, color);
        LCD_DrawPixel(x0 - x, y0 + y, color);
        LCD_DrawPixel(x0 - x, y0 - y, color);
        LCD_DrawPixel(x0 - y, y0 - x, color);
        LCD_DrawPixel(x0 + y, y0 - x, color);
        LCD_DrawPixel(x0 + x, y0 - y, color);
        
        y++;
        err += 1 + 2*y;
        if (2*(err-x) + 1 > 0) {
            x--;
            err += 1 - 2*x;
        }
    }
}

void LCD_FillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
    for (int16_t y = -r; y <= r; y++) {
        for (int16_t x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                LCD_DrawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}

void LCD_DrawChar(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg) {
    if (c < ' ' || c > 'Z') c = ' ';
    
    const uint8_t* glyph = font_5x7[c - ' '];
    
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (uint8_t row = 0; row < 7; row++) {
            if (line & 0x01) {
                LCD_DrawPixel(x + col, y + row, color);
            } else if (bg != color) {
                LCD_DrawPixel(x + col, y + row, bg);
            }
            line >>= 1;
        }
    }
}

void LCD_DrawString(uint8_t x, uint8_t y, const char* str, uint16_t color) {
    while (*str) {
        LCD_DrawChar(x, y, *str++, color, COLOR_BLACK);
        x += 6;
        if (x > LCD_WIDTH - 6) break;
    }
}

void LCD_DrawWaveform(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t waveform) {
    // Draw border
    LCD_DrawRect(x-1, y-1, w+2, h+2, COLOR_CYAN);
    
    // Clear inside
    LCD_FillRect(x, y, w, h, COLOR_BLACK);
    
    // Draw waveform
    uint8_t cy = y + h/2;
    
    for (uint8_t i = 0; i < w; i++) {
        float phase = (float)i / w * 2.0f * 3.14159f;
        int16_t sample = 0;
        
        switch (waveform) {
            case 0: // Sine
                sample = (int16_t)(sinf(phase) * (h/2 - 2));
                break;
            case 1: // Square
                sample = (phase < 3.14159f) ? (h/2 - 2) : -(h/2 - 2);
                break;
            case 2: // Sawtooth
                sample = (int16_t)((phase / (2.0f * 3.14159f) - 0.5f) * (h - 4));
                break;
            case 3: // Triangle
                if (phase < 3.14159f) {
                    sample = (int16_t)((phase / 3.14159f - 0.5f) * (h - 4));
                } else {
                    sample = (int16_t)((1.5f - phase / 3.14159f) * (h - 4));
                }
                break;
        }
        
        LCD_DrawPixel(x + i, cy - sample, COLOR_GREEN);
    }
}

void LCD_DrawSpectrum(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const float* fft_data, uint8_t bins) {
    // Draw border
    LCD_DrawRect(x-1, y-1, w+2, h+2, COLOR_CYAN);
    
    // Clear inside
    LCD_FillRect(x, y, w, h, COLOR_BLACK);
    
    // Draw bars
    uint8_t bar_width = w / bins;
    
    for (uint8_t i = 0; i < bins; i++) {
        uint8_t bar_height = (uint8_t)(fft_data[i] * h);
        if (bar_height > h) bar_height = h;
        
        uint16_t bar_color = COLOR_GREEN;
        if (bar_height > h * 2/3) bar_color = COLOR_RED;
        else if (bar_height > h/3) bar_color = COLOR_YELLOW;
        
        LCD_FillRect(x + i * bar_width, y + h - bar_height, 
                     bar_width - 1, bar_height, bar_color);
    }
}