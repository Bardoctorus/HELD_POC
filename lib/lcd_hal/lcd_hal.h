/**
 * @file lcd_hal.h
 * @brief Hardware Abstraction Layer for LCD
 * 
 * This module provides hardware-independent functions for SPI and GPIO.
 * This abstraction allows the display driver to work with different
 * microcontrollers by only changing the HAL implementation.
 */

#ifndef _LCD_HAL_H_
#define _LCD_HAL_H_

#include "../include/lcd_config.h"

/**
 * @brief Initialize all hardware (GPIO + SPI)
 * 
 * This function initializes:
 * - GPIO pins for RST, DC, CS, and BL
 * - SPI1 peripheral for communication
 * 
 * Must be called before using any other HAL functions.
 */
void LCD_HAL_Init(void);

#endif // _LCD_HAL_H_

