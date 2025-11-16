/**
 * @file gc9a01_driver.h
 * @brief GC9A01 Display Driver Interface
 * 
 * This module provides functions specific to the GC9A01 display controller.
 * For a different display, create a similar driver file with the same
 * interface pattern.
 * 
 * @note Colors are in RGB565 format (16-bit):
 *       - R: 5 bits (bits 15-11)
 *       - G: 6 bits (bits 10-5)
 *       - B: 5 bits (bits 4-0)
 */

#ifndef _GC9A01_DRIVER_H_
#define _GC9A01_DRIVER_H_

#include "../include/lcd_config.h"

// ============================================================================
// COLOR DEFINITIONS (RGB565 format)
// ============================================================================

#define LCD_COLOR_BLACK    0x0000  ///< RGB(0, 0, 0)
#define LCD_COLOR_WHITE    0xFFFF  ///< RGB(31, 63, 31)
#define LCD_COLOR_RED      0xF800  ///< RGB(31, 0, 0)
#define LCD_COLOR_GREEN    0x07E0  ///< RGB(0, 63, 0)
#define LCD_COLOR_BLUE     0x001F  ///< RGB(0, 0, 31)
#define LCD_COLOR_YELLOW   0xFFE0  ///< RGB(31, 63, 0)
#define LCD_COLOR_CYAN     0x07FF  ///< RGB(0, 63, 31)
#define LCD_COLOR_MAGENTA  0xF81F  ///< RGB(31, 0, 31)

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * @brief Initialize the GC9A01 display
 * 
 * Performs hardware reset and sends initialization sequence.
 * Must be called before using any other display functions.
 * 
 * @note Takes approximately 200ms due to required delays.
 */
void GC9A01_Init(void);

/**
 * @brief Set the display window (area to write pixels to)
 * 
 * Sets the column and row addresses for pixel writing.
 * After calling this, subsequent pixel data will fill the specified window.
 * 
 * @param x0 Left edge (0 to LCD_WIDTH-1)
 * @param y0 Top edge (0 to LCD_HEIGHT-1)
 * @param x1 Right edge (x0+1 to LCD_WIDTH)
 * @param y1 Bottom edge (y0+1 to LCD_HEIGHT)
 */
void GC9A01_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief Fill a rectangular area with a single color
 * 
 * Sets the window and fills it with the specified color.
 * 
 * @param x0 Left edge (0 to LCD_WIDTH-1)
 * @param y0 Top edge (0 to LCD_HEIGHT-1)
 * @param x1 Right edge (exclusive, x0+1 to LCD_WIDTH)
 * @param y1 Bottom edge (exclusive, y0+1 to LCD_HEIGHT)
 * @param color RGB565 color value
 */
void GC9A01_FillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, UWORD color);

/**
 * @brief Fill entire screen with a color
 * 
 * Convenience function to fill the entire display.
 * 
 * @param color RGB565 color value
 */
void GC9A01_FillScreen(UWORD color);

/**
 * @brief Draw horizontal stripes across the screen
 * 
 * Creates alternating color stripes for testing display functionality.
 * Useful for verifying communication and display operation.
 */
void GC9A01_DrawStripes(void);

#endif // _GC9A01_DRIVER_H_

