/**
 * @file lcd_driver.c
 * @brief ST7735 LCD Driver Implementation - CRITICAL FIX
 * @version 1.4 - DC PIN BUG FIXED
 * 
 * CRITICAL FIX:
 * - LCD_DC_Command() and LCD_DC_Data() were using GPIO_LCD_RST_PIN instead of GPIO_LCD_DC_PIN
 * - This caused LCD to never receive proper command/data selection
 * - Result: White screen, no display
 */

#include "ti_msp_dl_config.h"
#include "lcd_driver.h"
#include <string.h>
#include "../main.h"

//=============================================================================
// SINE LOOKUP TABLE (256 entries, scaled to -1000 to +1000)
//=============================================================================
static const int16_t sine_table[256] = {
    0, 25, 49, 74, 98, 122, 147, 171, 195, 219, 243, 267, 290, 314, 337, 360,
    383, 405, 428, 450, 471, 493, 514, 535, 555, 575, 595, 614, 633, 652, 670, 687,
    704, 721, 737, 753, 768, 783, 797, 811, 824, 837, 849, 860, 871, 882, 892, 901,
    910, 918, 926, 933, 939, 945, 951, 955, 960, 963, 966, 969, 971, 972, 973, 974,
    974, 973, 972, 971, 969, 966, 963, 960, 955, 951, 945, 939, 933, 926, 918, 910,
    901, 892, 882, 871, 860, 849, 837, 824, 811, 797, 783, 768, 753, 737, 721, 704,
    687, 670, 652, 633, 614, 595, 575, 555, 535, 514, 493, 471, 450, 428, 405, 383,
    360, 337, 314, 290, 267, 243, 219, 195, 171, 147, 122, 98, 74, 49, 25, 0,
    -25, -49, -74, -98, -122, -147, -171, -195, -219, -243, -267, -290, -314, -337, -360, -383,
    -405, -428, -450, -471, -493, -514, -535, -555, -575, -595, -614, -633, -652, -670, -687, -704,
    -721, -737, -753, -768, -783, -797, -811, -824, -837, -849, -860, -871, -882, -892, -901, -910,
    -918, -926, -933, -939, -945, -951, -955, -960, -963, -966, -969, -971, -972, -973, -974, -974,
    -973, -972, -971, -969, -966, -963, -960, -955, -951, -945, -939, -933, -926, -918, -910, -901,
    -892, -882, -871, -860, -849, -837, -824, -811, -797, -783, -768, -753, -737, -721, -704, -687,
    -670, -652, -633, -614, -595, -575, -555, -535, -514, -493, -471, -450, -428, -405, -383, -360,
    -337, -314, -290, -267, -243, -219, -195, -171, -147, -122, -98, -74, -49, -25
};

//=============================================================================
// ST7735 Commands
//=============================================================================
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

//=============================================================================
// FONT TABLE
//=============================================================================
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space (32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // ! (33)
    {0x00, 0x07, 0x00, 0x07, 0x00}, // " (34)
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // # (35)
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $ (36)
    {0x23, 0x13, 0x08, 0x64, 0x62}, // % (37)
    {0x36, 0x49, 0x55, 0x22, 0x50}, // & (38)
    {0x00, 0x05, 0x03, 0x00, 0x00}, // ' (39)
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // ( (40)
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ) (41)
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // * (42)
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // + (43)
    {0x00, 0x50, 0x30, 0x00, 0x00}, // , (44)
    {0x08, 0x08, 0x08, 0x08, 0x08}, // - (45)
    {0x00, 0x60, 0x60, 0x00, 0x00}, // . (46)
    {0x20, 0x10, 0x08, 0x04, 0x02}, // / (47)
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0 (48)
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1 (49)
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2 (50)
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3 (51)
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4 (52)
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5 (53)
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6 (54)
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7 (55)
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8 (56)
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9 (57)
    {0x00, 0x36, 0x36, 0x00, 0x00}, // : (58)
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ; (59)
    {0x08, 0x14, 0x22, 0x41, 0x00}, // < (60)
    {0x14, 0x14, 0x14, 0x14, 0x14}, // = (61)
    {0x00, 0x41, 0x22, 0x14, 0x08}, // > (62)
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ? (63)
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @ (64)
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A (65)
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B (66)
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C (67)
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D (68)
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E (69)
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F (70)
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G (71)
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H (72)
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I (73)
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J (74)
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K (75)
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L (76)
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M (77)
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N (78)
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O (79)
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P (80)
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q (81)
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R (82)
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S (83)
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T (84)
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U (85)
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V (86)
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W (87)
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X (88)
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y (89)
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z (90)
};

//=============================================================================
// PRIVATE FUNCTIONS - CRITICAL FIX HERE!
//=============================================================================

/**
 * @brief Set DC pin LOW for command mode
 * FIXED: Was using RST_PIN, now correctly uses DC_PIN
 */
static inline void LCD_DC_Command(void) {
    DL_GPIO_clearPins(GPIO_LCD_PORT, GPIO_LCD_DC_PIN);  // ✅ FIXED!
}

/**
 * @brief Set DC pin HIGH for data mode
 * FIXED: Was using RST_PIN, now correctly uses DC_PIN
 */
static inline void LCD_DC_Data(void) {
    DL_GPIO_setPins(GPIO_LCD_PORT, GPIO_LCD_DC_PIN);    // ✅ FIXED!
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
    // Hardware reset sequence
    DL_GPIO_clearPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN);  // Reset LOW
    delay_ms(20); 
    DL_GPIO_setPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN);    // Reset HIGH
    delay_ms(120);

    // Software reset
    LCD_WriteCommand(ST7735_SWRESET);
    delay_ms(150);
    
    // Sleep out
    LCD_WriteCommand(ST7735_SLPOUT); 
    delay_ms(120);

    // Frame rate control
    LCD_WriteCommand(ST7735_FRMCTR1);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    // Color mode: 16-bit/pixel
    LCD_WriteCommand(ST7735_COLMOD);
    LCD_WriteData(0x05);

    // Memory data access control
    LCD_WriteCommand(ST7735_MADCTL);
    LCD_WriteData(0xC8); 

    // Normal display mode
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
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t x = x0, y = y0;
    
    while (1) {
        LCD_DrawPixel((uint8_t)x, (uint8_t)y, color);
        if (x == x1 && y == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
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
    int16_t x = r, y = 0, err = 0;
    
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
    LCD_DrawRect(x-1, y-1, w+2, h+2, COLOR_CYAN);
    LCD_FillRect(x, y, w, h, COLOR_BLACK);
    
    uint8_t cy = y + h/2;
    int16_t amplitude = h/2 - 2;
    
    for (uint8_t i = 0; i < w; i++) {
        int16_t sample = 0;
        uint8_t phase_idx = (uint8_t)((i * 256UL) / w);
        
        switch (waveform) {
            case 0: // Sine - lookup table
                sample = (sine_table[phase_idx] * amplitude) / 1000;
                break;
            case 1: // Square
                sample = (i < w/2) ? amplitude : -amplitude;
                break;
            case 2: // Sawtooth
                sample = (int16_t)(((i * 2 * amplitude) / w) - amplitude);
                break;
            case 3: // Triangle
                if (i < w/2) {
                    sample = (int16_t)((i * 2 * amplitude) / w);
                } else {
                    sample = (int16_t)(amplitude - ((i - w/2) * 2 * amplitude) / w);
                }
                break;
        }
        
        LCD_DrawPixel(x + i, cy - sample, COLOR_GREEN);
    }
}

void LCD_DrawSpectrum(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const float* fft_data, uint8_t bins) {
    LCD_DrawRect(x-1, y-1, w+2, h+2, COLOR_CYAN);
    LCD_FillRect(x, y, w, h, COLOR_BLACK);
    
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