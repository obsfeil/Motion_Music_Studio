/**
 * @file lcd_driver.c
 * @brief ST7735 LCD Driver Implementation for MSPM0G3507 BoosterPack MKII
 */

#include "lcd_driver.h"
#include <string.h>

//=============================================================================
// ST7735 COMMANDS
//=============================================================================
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_NORON   0x13
#define ST7735_INVOFF  0x20
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

//=============================================================================
// GPIO MACROS
//=============================================================================
#define LCD_CS_HIGH()  DL_GPIO_setPins(GPIO_SPI_LCD_CS0_PORT, GPIO_SPI_LCD_CS0_PIN)
#define LCD_CS_LOW()   DL_GPIO_clearPins(GPIO_SPI_LCD_CS0_PORT, GPIO_SPI_LCD_CS0_PIN)
#define LCD_DC_LOW()   DL_GPIO_clearPins(GPIO_LCD_PORT, GPIO_LCD_DC_PIN)
#define LCD_DC_HIGH()  DL_GPIO_setPins(GPIO_LCD_PORT, GPIO_LCD_DC_PIN)
#define LCD_RST_LOW()  DL_GPIO_clearPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN)
#define LCD_RST_HIGH() DL_GPIO_setPins(GPIO_LCD_PORT, GPIO_LCD_RST_PIN)


//=============================================================================
// FONT (Simple 5x7 font)
//=============================================================================
static const uint8_t font5x7[][5] = {
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

static void LCD_WriteCommand(uint8_t cmd) {
    LCD_DC_LOW();   // Command mode
    LCD_CS_LOW();
    DL_SPI_transmitData8(SPI_LCD_INST, cmd);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    LCD_CS_HIGH();
}

static void LCD_WriteData(uint8_t data) {
    LCD_DC_HIGH();  // Data mode
    LCD_CS_LOW();
    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    LCD_CS_HIGH();
}

static void LCD_WriteData16(uint16_t data) {
    LCD_WriteData(data >> 8);    // High byte
    LCD_WriteData(data & 0xFF);  // Low byte
}

static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address
    LCD_WriteCommand(ST7735_CASET);
    LCD_WriteData16(x0);
    LCD_WriteData16(x1);
    
    // Row address
    LCD_WriteCommand(ST7735_RASET);
    LCD_WriteData16(y0);
    LCD_WriteData16(y1);
    
    // Write to RAM
    LCD_WriteCommand(ST7735_RAMWR);
}

static void delay_ms(uint32_t ms) {
    DL_Common_delayCycles(80000000 / 1000 * ms);
}

//=============================================================================
// PUBLIC FUNCTIONS
//=============================================================================

void LCD_Init(void) {
    // Hardware reset
    LCD_RST_HIGH();
    delay_ms(10);
    LCD_RST_LOW();
    delay_ms(10);
    LCD_RST_HIGH();
    delay_ms(120);
    
    // Software reset
    LCD_WriteCommand(ST7735_SWRESET);
    delay_ms(150);
    
    // Sleep out
    LCD_WriteCommand(ST7735_SLPOUT);
    delay_ms(120);
    
    // Color mode: 16-bit/pixel (RGB565)
    LCD_WriteCommand(ST7735_COLMOD);
    LCD_WriteData(0x05);
    
    // Memory access control (rotation, etc)
    LCD_WriteCommand(ST7735_MADCTL);
    LCD_WriteData(0xC0);  // RGB order, portrait mode
    
    // Normal display mode
    LCD_WriteCommand(ST7735_NORON);
    delay_ms(10);
    
    // Display on
    LCD_WriteCommand(ST7735_DISPON);
    delay_ms(10);
    
    // Clear screen
    LCD_Clear(COLOR_BLACK);
}

void LCD_Clear(Color_t color) {
    LCD_FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void LCD_DrawPixel(uint16_t x, uint16_t y, Color_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    LCD_SetWindow(x, y, x, y);
    LCD_WriteData16(color);
}

void LCD_DrawChar(uint16_t x, uint16_t y, char c, Color_t color) {
    if (c < ' ' || c > 'Z') c = ' ';
    
    const uint8_t* glyph = font5x7[c - ' '];
    
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (uint8_t j = 0; j < 7; j++) {
            if (line & (1 << j)) {
                LCD_DrawPixel(x + i, y + j, color);
            }
        }
    }
}

void LCD_DrawString(uint16_t x, uint16_t y, const char* str, Color_t color) {
    while (*str) {
        LCD_DrawChar(x, y, *str++, color);
        x += 6;  // Character width + spacing
        if (x > LCD_WIDTH - 6) break;
    }
}

void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color_t color) {
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int16_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;
    
    while (1) {
        LCD_DrawPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color_t color) {
    LCD_DrawLine(x, y, x + w - 1, y, color);           // Top
    LCD_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
    LCD_DrawLine(x, y, x, y + h - 1, color);           // Left
    LCD_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    LCD_SetWindow(x, y, x + w - 1, y + h - 1);
    
    LCD_DC_HIGH();
    LCD_CS_LOW();
    
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        DL_SPI_transmitData8(SPI_LCD_INST, color >> 8);
        while (DL_SPI_isBusy(SPI_LCD_INST));
        DL_SPI_transmitData8(SPI_LCD_INST, color & 0xFF);
        while (DL_SPI_isBusy(SPI_LCD_INST));
    }
    
    LCD_CS_HIGH();
}

void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, Color_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    LCD_DrawPixel(x0, y0 + r, color);
    LCD_DrawPixel(x0, y0 - r, color);
    LCD_DrawPixel(x0 + r, y0, color);
    LCD_DrawPixel(x0 - r, y0, color);
    
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

void LCD_FillCircle(uint16_t x0, uint16_t y0, uint16_t r, Color_t color) {
    for (int16_t y = -r; y <= r; y++) {
        for (int16_t x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                LCD_DrawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}
