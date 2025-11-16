/**
 * @file gc9a01_driver.c
 * @brief GC9A01/GC9101 Display Driver Implementation
 * 
 * This file implements the GC9A01/GC9101-specific initialization and drawing functions.
 * 
 * NOTE: Some displays are marked GC9101 but are compatible with GC9A01 commands.
 * If display has a level translator (SN74LVCC3245A), use slower SPI speeds.
 * 
 * Communication Protocol:
 * - Commands: DC low, send command byte
 * - Data: DC high, send data byte(s)
 * - CS must be pulled low before and high after each transmission
 * 
 * Pixel Format: RGB565 (16-bit per pixel)
 * - Each pixel is sent as two bytes (MSB first)
 * - Format: RRRRRGGG GGGGBBBB
 */

#include "gc9a01_driver.h"
#include "../lcd_hal/lcd_hal.h"

// ============================================================================
// PRIVATE FUNCTIONS - Communication Layer
// ============================================================================

/**
 * @brief Send a command byte to the display
 * 
 * CRITICAL: CS stays LOW after command - do NOT set CS high!
 * The working example code shows CS should remain LOW for entire command sequences.
 * Only data bytes toggle CS high after transmission.
 * 
 * @param cmd Command byte to send (0x00 to 0xFF)
 */
static void GC9A01_SendCommand(UBYTE cmd)
{
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);   // CS low = select display
    LCD_HAL_Delay_us(1);  // Small delay for CS to stabilize
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);   // D/C low = command mode
    LCD_HAL_Delay_us(1);  // Small delay for DC to stabilize
    LCD_HAL_SPI_WriteByte(cmd);
    LCD_HAL_Delay_us(1);  // Small delay after SPI transmission
    // NOTE: CS stays LOW - do NOT set CS high here!
    // CS will be set high after data bytes in GC9A01_SendData()
}

/**
 * @brief Send a data byte to the display
 * 
 * Sets DC high (data mode), CS should already be LOW from command,
 * sends data, then CS high.
 * Used for sending parameters that follow commands.
 * 
 * @param data Data byte to send (0x00 to 0xFF)
 */
static void GC9A01_SendData(UBYTE data)
{
    // CS should already be LOW from previous command
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // D/C high = data mode
    LCD_HAL_Delay_us(1);  // Small delay for DC to stabilize
    LCD_HAL_SPI_WriteByte(data);
    LCD_HAL_Delay_us(1);  // Small delay after SPI transmission
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // CS high = deselect
    LCD_HAL_Delay_us(10);  // Small delay between data bytes
}

/**
 * @brief Send multiple data bytes (optimized for bulk transfers)
 * 
 * Sets DC high once, then sends all bytes with CS held low throughout.
 * More efficient than calling GC9A01_SendData() multiple times because
 * it only toggles CS once instead of for each byte.
 * 
 * @param pData Pointer to data buffer
 * @param len   Number of bytes to send
 */
static void GC9A01_SendDataBulk(uint8_t *pData, uint32_t len)
{
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // D/C high = data mode
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);   // CS low = select display
    LCD_HAL_SPI_WriteBytes(pData, len);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // CS high = deselect
}

/**
 * @brief Send command followed by data bytes with CS held low throughout
 * 
 * Alternative CS timing: Keep CS low for entire command+data sequence.
 * Some displays (especially with level translators) prefer this timing.
 * 
 * @param cmd  Command byte
 * @param pData Pointer to data bytes (can be NULL if no data)
 * @param len   Number of data bytes (0 if no data)
 */
static void GC9A01_SendCommandWithData(UBYTE cmd, uint8_t *pData, uint32_t len)
{
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);   // CS low = select display
    LCD_HAL_Delay_us(2);  // Small delay for CS to stabilize
    
    // Send command
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // D/C low = command mode
    LCD_HAL_Delay_us(1);  // Small delay for DC to stabilize
    LCD_HAL_SPI_WriteByte(cmd);
    
    // Send data if any
    if (len > 0 && pData != NULL) {
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // D/C high = data mode
        LCD_HAL_Delay_us(1);  // Small delay for DC to stabilize
        for (uint32_t i = 0; i < len; i++) {
            LCD_HAL_SPI_WriteByte(pData[i]);
        }
    }
    
    LCD_HAL_Delay_us(2);  // Small delay before releasing CS
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // CS high = deselect
    LCD_HAL_Delay_us(10);  // Small delay between commands
}

/**
 * @brief Perform hardware reset of the display
 * 
 * According to GC9A01 datasheet:
 * - RESX is pulled low when module is powered on
 * - RESX should usually be set to 1 (high)
 * - Reset sequence: Pull low, then release high
 * 
 * Reset sequence:
 * 1. Ensure RST is high (not resetting)
 * 2. Pull RST low (reset) - hold for at least 10ms
 * 3. Release RST high - wait at least 120ms for display to stabilize
 */
static void GC9A01_Reset(void)
{
    // CRITICAL: Working example (Arduino) sets CS LOW first, then performs reset
    // STM32 version doesn't manipulate CS during reset - testing Arduino version first
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);  // CS low (Arduino example does this)
    LCD_HAL_Delay_ms(100);
    
    // Pull RESX low to reset
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 0);
    LCD_HAL_Delay_ms(100);  // Hold reset (working example uses 100ms)
    
    // Release RESX high
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    LCD_HAL_Delay_ms(100);  // Wait for display to stabilize (working example uses 100ms)
    // Note: CS remains LOW - do NOT set CS high here!
}

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * @brief Initialize GC9A01 display registers
 * 
 * This function sends the complete initialization sequence to the GC9A01.
 * The sequence includes power settings, memory access control, pixel format,
 * gamma correction, and finally display on commands.
 * 
 * This is the verified GC9A01 initialization sequence based on datasheet
 * and community implementations.
 */
static void GC9A01_InitRegisters(void)
{
    // CRITICAL: Initialization sequence must start with 0xEF, 0xEB, 0x14
    // Then 0xFE, 0xEF, then 0xEB, 0x14 again
    // This is the correct sequence from the working example code
    
    GC9A01_SendCommand(0xEF);
    GC9A01_SendCommand(0xEB);
    GC9A01_SendData(0x14);
    
    GC9A01_SendCommand(0xFE);
    GC9A01_SendCommand(0xEF);
    
    GC9A01_SendCommand(0xEB);
    GC9A01_SendData(0x14);
    
    // VCOM setting
    GC9A01_SendCommand(0x84);
    GC9A01_SendData(0x40);
    
    // LUT (Look-Up Table) settings for power optimization
    GC9A01_SendCommand(0x85);
    GC9A01_SendData(0xFF);
    GC9A01_SendCommand(0x86);
    GC9A01_SendData(0xFF);
    GC9A01_SendCommand(0x87);
    GC9A01_SendData(0xFF);
    GC9A01_SendCommand(0x88);
    GC9A01_SendData(0x0A);
    GC9A01_SendCommand(0x89);
    GC9A01_SendData(0x21);
    GC9A01_SendCommand(0x8A);
    GC9A01_SendData(0x00);
    GC9A01_SendCommand(0x8B);
    GC9A01_SendData(0x80);
    GC9A01_SendCommand(0x8C);
    GC9A01_SendData(0x01);
    GC9A01_SendCommand(0x8D);
    GC9A01_SendData(0x01);
    GC9A01_SendCommand(0x8E);
    GC9A01_SendData(0xFF);
    GC9A01_SendCommand(0x8F);
    GC9A01_SendData(0xFF);
    
    // Internal pump voltage
    GC9A01_SendCommand(0xB6);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x20);
    
    // Memory access control (orientation and RGB order)
    // 0x08 = Normal orientation, RGB order
    GC9A01_SendCommand(0x36);
    GC9A01_SendData(0x08);
    
    // Pixel format: 16-bit/pixel (RGB565)
    // 0x05 = 16-bit color
    GC9A01_SendCommand(0x3A);
    GC9A01_SendData(0x05);
    
    // Display function control
    GC9A01_SendCommand(0x90);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x08);
    
    // Additional display settings
    GC9A01_SendCommand(0xBD);
    GC9A01_SendData(0x06);
    GC9A01_SendCommand(0xBC);
    GC9A01_SendData(0x00);
    GC9A01_SendCommand(0xFF);
    GC9A01_SendData(0x60);
    GC9A01_SendData(0x01);
    GC9A01_SendData(0x04);
    GC9A01_SendCommand(0xC3);
    GC9A01_SendData(0x13);
    GC9A01_SendCommand(0xC4);
    GC9A01_SendData(0x13);
    GC9A01_SendCommand(0xC9);
    GC9A01_SendData(0x22);
    GC9A01_SendCommand(0xBE);
    GC9A01_SendData(0x11);
    
    // Gamma correction (positive polarity) - only 2 bytes in working example
    GC9A01_SendCommand(0xE1);
    GC9A01_SendData(0x10);
    GC9A01_SendData(0x0E);
    
    // Additional display settings from working example
    GC9A01_SendCommand(0xDF);
    GC9A01_SendData(0x21);
    GC9A01_SendData(0x0c);
    GC9A01_SendData(0x02);
    
    GC9A01_SendCommand(0xF0);
    GC9A01_SendData(0x45);
    GC9A01_SendData(0x09);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x26);
    GC9A01_SendData(0x2A);
    
    GC9A01_SendCommand(0xF1);
    GC9A01_SendData(0x43);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x72);
    GC9A01_SendData(0x36);
    GC9A01_SendData(0x37);
    GC9A01_SendData(0x6F);
    
    GC9A01_SendCommand(0xF2);
    GC9A01_SendData(0x45);
    GC9A01_SendData(0x09);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x26);
    GC9A01_SendData(0x2A);
    
    GC9A01_SendCommand(0xF3);
    GC9A01_SendData(0x43);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x72);
    GC9A01_SendData(0x36);
    GC9A01_SendData(0x37);
    GC9A01_SendData(0x6F);
    
    GC9A01_SendCommand(0xED);
    GC9A01_SendData(0x1B);
    GC9A01_SendData(0x0B);
    
    GC9A01_SendCommand(0xAE);
    GC9A01_SendData(0x77);
    
    GC9A01_SendCommand(0xCD);
    GC9A01_SendData(0x63);
    
    GC9A01_SendCommand(0x70);
    GC9A01_SendData(0x07);
    GC9A01_SendData(0x07);
    GC9A01_SendData(0x04);
    GC9A01_SendData(0x0E);
    GC9A01_SendData(0x0F);
    GC9A01_SendData(0x09);
    GC9A01_SendData(0x07);
    GC9A01_SendData(0x08);
    GC9A01_SendData(0x03);
    
    GC9A01_SendCommand(0xE8);
    GC9A01_SendData(0x34);
    
    GC9A01_SendCommand(0x62);
    GC9A01_SendData(0x18);
    GC9A01_SendData(0x0D);
    GC9A01_SendData(0x71);
    GC9A01_SendData(0xED);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x18);
    GC9A01_SendData(0x0F);
    GC9A01_SendData(0x71);
    GC9A01_SendData(0xEF);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x70);
    
    GC9A01_SendCommand(0x63);
    GC9A01_SendData(0x18);
    GC9A01_SendData(0x11);
    GC9A01_SendData(0x71);
    GC9A01_SendData(0xF1);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x18);
    GC9A01_SendData(0x13);
    GC9A01_SendData(0x71);
    GC9A01_SendData(0xF3);
    GC9A01_SendData(0x70);
    GC9A01_SendData(0x70);
    
    GC9A01_SendCommand(0x64);
    GC9A01_SendData(0x28);
    GC9A01_SendData(0x29);
    GC9A01_SendData(0xF1);
    GC9A01_SendData(0x01);
    GC9A01_SendData(0xF1);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x07);
    
    GC9A01_SendCommand(0x66);
    GC9A01_SendData(0x3C);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0xCD);
    GC9A01_SendData(0x67);
    GC9A01_SendData(0x45);
    GC9A01_SendData(0x45);
    GC9A01_SendData(0x10);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x00);
    
    GC9A01_SendCommand(0x67);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x3C);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x01);
    GC9A01_SendData(0x54);
    GC9A01_SendData(0x10);
    GC9A01_SendData(0x32);
    GC9A01_SendData(0x98);
    
    GC9A01_SendCommand(0x74);
    GC9A01_SendData(0x10);
    GC9A01_SendData(0x85);
    GC9A01_SendData(0x80);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x00);
    GC9A01_SendData(0x4E);
    GC9A01_SendData(0x00);
    
    GC9A01_SendCommand(0x98);
    GC9A01_SendData(0x3e);
    GC9A01_SendData(0x07);
    
    GC9A01_SendCommand(0x35);
    GC9A01_SendCommand(0x21);
    
    // Sleep out - exit sleep mode (120ms delay required)
    GC9A01_SendCommand(0x11);
    LCD_HAL_Delay_ms(120);
    
    // Display on
    GC9A01_SendCommand(0x29);
    LCD_HAL_Delay_ms(20);
}

/**
 * @brief Initialize the GC9A01 display
 * 
 * Main initialization function. Performs hardware reset and then sends
 * the initialization sequence to configure the display.
 * 
 * Steps:
 * 1. Hardware reset via RST pin
 * 2. Send initialization register sequence
 * 
 * Must be called before using any other display functions.
 * 
 * @note Takes approximately 200ms due to required delays
 */
void GC9A01_Init(void)
{
    // Step 1: Hardware reset
    GC9A01_Reset();
    
    // Step 2: Initialize display registers
    GC9A01_InitRegisters();
}

// ============================================================================
// DISPLAY CONTROL
// ============================================================================

/**
 * @brief Set the display window (area to write pixels to)
 * 
 * Sets the column and row addresses for pixel writing.
 * After calling this, subsequent pixel data will fill the specified window.
 * 
 * GC9A01 commands:
 * - 0x2A: Column Address Set (X coordinates)
 * - 0x2B: Row Address Set (Y coordinates)
 * - 0x2C: Memory Write (ready to receive pixel data)
 * 
 * @param x0 Left edge (0 to LCD_WIDTH-1)
 * @param y0 Top edge (0 to LCD_HEIGHT-1)
 * @param x1 Right edge (x0+1 to LCD_WIDTH)
 * @param y1 Bottom edge (y0+1 to LCD_HEIGHT)
 */
void GC9A01_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // CRITICAL: Working example uses 8-bit coordinate format: 0x00, X (not 16-bit MSB/LSB)
    // Working code: LCD_SetCursor(0,0,LCD_WIDTH-1,LCD_HEIGHT-1) passes Xend=239, Yend=239
    // But the code sends Xend directly: LCD_WriteData_Byte(Xend) - no -1 in the code itself!
    // The comment says Xend-1 but the code doesn't subtract 1 when sending
    // So if called with (0,0,239,239), it sends Xend=239, Yend=239
    
    // Our FillRect uses exclusive end coordinates (x1,y1), so if called with (0,0,240,240):
    // We want to send Xend=239, Yend=239 (which is x1-1, y1-1) ✓
    
    // Set column address (X coordinates)
    GC9A01_SendCommand(0x2A);
    GC9A01_SendData(0x00);              // High byte (always 0 for 240x240 display)
    GC9A01_SendData(x0 & 0xFF);         // X start (8-bit)
    GC9A01_SendData(0x00);              // High byte
    GC9A01_SendData((x1 - 1) & 0xFF);   // X end (8-bit) - convert from exclusive to inclusive
    
    // Set row address (Y coordinates)
    GC9A01_SendCommand(0x2B);
    GC9A01_SendData(0x00);              // High byte (always 0 for 240x240 display)
    GC9A01_SendData(y0 & 0xFF);         // Y start (8-bit)
    GC9A01_SendData(0x00);              // High byte
    GC9A01_SendData((y1 - 1) & 0xFF);   // Y end (8-bit) - convert from exclusive to inclusive
    
    // Memory write command - ready to receive pixel data
    // After SetCursor: CS was HIGH (from last SendData in 0x2A/0x2B)
    // After 0x2C command: CS is LOW (SendCommand sets CS LOW, doesn't set it high)
    // So CS is LOW and ready for pixel data
    GC9A01_SendCommand(0x2C);
    // CS is now LOW - pixel data will be sent with CS LOW
}

// ============================================================================
// DRAWING FUNCTIONS
// ============================================================================

/**
 * @brief Fill a rectangular area with a single color
 * 
 * Sets the window and fills it with the specified color.
 * Uses bulk data transfer for efficiency.
 * 
 * @param x0 Left edge (0 to LCD_WIDTH-1)
 * @param y0 Top edge (0 to LCD_HEIGHT-1)
 * @param x1 Right edge (exclusive, x0+1 to LCD_WIDTH)
 * @param y1 Bottom edge (exclusive, y0+1 to LCD_HEIGHT)
 * @param color RGB565 color value
 */
void GC9A01_FillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, UWORD color)
{
    // Clamp coordinates to display bounds
    // NOTE: x1, y1 are exclusive (like array indices)
    if (x0 >= LCD_WIDTH || y0 >= LCD_HEIGHT) return;
    if (x1 > LCD_WIDTH) x1 = LCD_WIDTH;
    if (y1 > LCD_HEIGHT) y1 = LCD_HEIGHT;
    if (x0 >= x1 || y0 >= y1) return;
    
    // Set the window to fill
    GC9A01_SetWindow(x0, y0, x1, y1);
    
    // Prepare color bytes (RGB565 format)
    // Working example: LCD_WriteData_Word() sends (da>>8) first (MSB), then (da) (LSB)
    uint8_t color_msb = (color >> 8) & 0xFF;  // MSB (high byte)
    uint8_t color_lsb = color & 0xFF;         // LSB (low byte)
    
    // Calculate dimensions
    uint16_t width = x1 - x0;
    uint16_t height = y1 - y0;
    
    // IMPORTANT: After 0x2C command, CS is LOW and stays LOW
    // Working code's LCD_WriteData_Word() does toggle CS per pixel, but that made rings worse
    // So keep CS LOW for entire stream (more efficient and seems to work better)
    // 
    // Working code loop: for(i=0; i<LCD_WIDTH; i++) for(j=0; j<LCD_HEIGHT; j++)
    // Where i=column (X), j=row (Y) - column-by-column order
    // 
    // But wait - maybe the issue is we're sending 1 pixel too many or too few?
    // Working code sends exactly LCD_WIDTH * LCD_HEIGHT = 240 * 240 = 57,600 pixels
    // Our calculation: (x1-x0) * (y1-y0) = (240-0) * (240-0) = 57,600 pixels ✓
    //
    // Try keeping CS LOW for entire stream (more efficient) and column-by-column order
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // Data mode (DC high) - set once
    // CS is already LOW from SetWindow's 0x2C command - keep it LOW
    
    // Send all pixels with CS LOW (continuous transfer) - COLUMN-BY-COLUMN order
    // Working LCD_Clear: for(i=0; i<LCD_WIDTH; i++) for(j=0; j<LCD_HEIGHT; j++)
    // Where i=column (X), j=row (Y) - sends column-by-column
    // This means: all rows of column 0, then all rows of column 1, etc.
    for (uint16_t col = 0; col < width; col++) {
        for (uint16_t row = 0; row < height; row++) {
            // Send MSB first, then LSB (matches working example: da>>8, then da)
            LCD_HAL_SPI_WriteByte(color_msb);  // MSB first (color >> 8)
            LCD_HAL_SPI_WriteByte(color_lsb);  // LSB second (color & 0xFF)
        }
    }
    
    // CRITICAL: Wait for last byte to complete before CS goes HIGH
    // Otherwise transmission may be cut off
    uint32_t timeout = 100000;
    while((SPI1->STATR & (1 << 7)) && timeout--) {}  // Wait for BSY to clear
    
    // Set CS HIGH after all pixels are sent and transmission complete
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // CS HIGH after entire stream
}

/**
 * @brief Fill entire screen with a color
 * 
 * Convenience function to fill the entire display.
 * 
 * @param color RGB565 color value
 */
void GC9A01_FillScreen(UWORD color)
{
    // Working example: LCD_SetCursor(0,0,LCD_WIDTH-1,LCD_HEIGHT-1) with 240x240 loop
    // We need to send exactly LCD_WIDTH * LCD_HEIGHT pixels (240*240 = 57,600)
    // Our FillRect uses exclusive end coordinates, so pass LCD_WIDTH, LCD_HEIGHT
    GC9A01_FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

/**
 * @brief Draw horizontal stripes across the screen
 * 
 * Creates alternating color stripes for testing display functionality.
 * Useful for verifying communication and display operation.
 * 
 * Draws 8 horizontal stripes, alternating between white and black.
 */
void GC9A01_DrawStripes(void)
{
    uint16_t stripe_height = LCD_HEIGHT / 8;
    
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t y0 = i * stripe_height;
        uint16_t y1 = (i + 2) * stripe_height;
        
        // Alternate colors: even = white, odd = black
        UWORD color = (i % 2 == 0) ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
        GC9A01_FillRect(0, y0, LCD_WIDTH, y1, color);
    }
}

