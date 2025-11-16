/**
 * @file lcd_config.h
 * @brief LCD Hardware Configuration
 * 
 * This file defines all pin assignments and display parameters.
 * Modify these values to match your hardware wiring.
 */

#ifndef _LCD_CONFIG_H_
#define _LCD_CONFIG_H_

#include "ch32fun.h"
#include <stdint.h>

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

typedef uint8_t  UBYTE;
typedef uint16_t UWORD;
typedef uint32_t UDOUBLE;

// ============================================================================
// PIN CONFIGURATION - Modify these to match your hardware wiring
// ============================================================================

// Control pins (can use any available GPIO pins)
// Display board labels: RED (RST), CS, DC, BLK (BL), SDA (MOSI), SCL (SCK)
// NOTE: PD1 is SWIO (programming pin) - DO NOT USE!
#define LCD_RST_PIN   PD0   ///< Reset pin (Display label: RED)
#define LCD_DC_PIN    PD4   ///< Data/Command pin (Display label: DC) - Low=Command, High=Data
#define LCD_CS_PIN    PD2   ///< Chip Select pin (Display label: CS) - active low, managed via GPIO
#define LCD_BL_PIN    PD3   ///< Backlight pin (Display label: BLK) - optional, can be tied to VCC

// SPI pins (must be SPI1 compatible pins on CH32v003)
// PC5 = SPI1_SCK, PC6 = SPI1_MOSI (fixed pins, cannot be changed)
// Note: Display uses SDA/SCL labels (normally I2C), but this is SPI!
#define LCD_SCK_PIN   PC5   ///< SPI1 Clock (Display label: SCL)
#define LCD_MOSI_PIN  PC6   ///< SPI1 MOSI (Display label: SDA - Serial Data)

// ============================================================================
// DISPLAY PARAMETERS
// ============================================================================

/// GC9A01 Display width in pixels (1.28" round, 240x240)
#define LCD_WIDTH    240

/// GC9A01 Display height in pixels
#define LCD_HEIGHT   240

// ============================================================================
// GPIO CONFIGURATION
// ============================================================================

/// GPIO polarity configuration
/// If your board has inverters on GPIO lines between MCU and LCD, set this to 1
/// Set to 0 for normal (non-inverted) GPIO operation
/// 
/// NOTE: LED circuits being active-low (like PC0 debug LED) doesn't mean
/// GPIOs are inverted - that's just circuit wiring. Only enable this if
/// LCD control pins (CS, DC, RST, BL) themselves are inverted by hardware.
#define LCD_GPIO_INVERTED  0  // Change to 1 only if LCD pins have hardware inverters

// ============================================================================
// SPI CONFIGURATION
// ============================================================================

/// SPI clock speed in Hz
/// IMPORTANT: With level translator (SN74LVCC3245A) on display board, use slower speeds
/// Start with 1.5MHz (48MHz / 32) for level translator compatibility
/// Increase to 3MHz or 6MHz if display works well
#define LCD_SPI_SPEED_HZ  1500000  // 1.5MHz - slower for level translator compatibility

// ============================================================================
// FUNCTION PROTOTYPES - Hardware Abstraction Layer
// ============================================================================

// GPIO functions
void LCD_HAL_GPIO_Init(void);
void LCD_HAL_DigitalWrite(UWORD Pin, UBYTE Value);

// SPI functions
void LCD_HAL_SPI_Init(void);
void LCD_HAL_SPI_WriteByte(UBYTE Value);
void LCD_HAL_SPI_WriteBytes(uint8_t *pData, uint32_t Length);

// Delay functions
void LCD_HAL_Delay_ms(UDOUBLE ms);
void LCD_HAL_Delay_us(UDOUBLE us);

#endif // _LCD_CONFIG_H_

