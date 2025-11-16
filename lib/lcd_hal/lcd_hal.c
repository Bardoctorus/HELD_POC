/**
 * @file lcd_hal.c
 * @brief Hardware Abstraction Layer Implementation for CH32v003
 * 
 * This file implements SPI and GPIO functions using CH32v003 hardware SPI1.
 * Uses direct register access based on ch32v003_SPI.h patterns.
 * 
 * SPI Configuration:
 * - Mode 3 (CPOL=1, CPHA=1): Clock idle high, sample on second edge
 *   (Matches working Arduino example which uses SPI_MODE3)
 * - 8-bit data frames
 * - MSB first
 * - Software NSS (CS) management via GPIO
 */

#include "lcd_hal.h"
#include "../include/lcd_config.h"

/**
 * @brief Initialize GPIO pins for LCD
 * 
 * Configures RST, DC, CS, and BL pins as outputs.
 * SPI pins (SCK, MOSI) are configured in LCD_HAL_SPI_Init().
 */
void LCD_HAL_GPIO_Init(void)
{
    // Initialize GPIO system (enables clocks for GPIOA, GPIOC, GPIOD)
    funGpioInitAll();
    
    // Configure control pins as push-pull outputs, 10MHz speed
    funPinMode(LCD_RST_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_DC_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_CS_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_BL_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    
    // Set initial states
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // CS high (inactive)
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // DC low (command mode)
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1); // Reset high (not resetting)
    LCD_HAL_DigitalWrite(LCD_BL_PIN, 1);  // Backlight on
}

/**
 * @brief Write a digital value to a GPIO pin
 * 
 * @param Pin   GPIO pin to write (e.g., PD0, PC5)
 * @param Value 0 = LOW, 1 = HIGH
 * 
 * @note If LCD_GPIO_INVERTED is set to 1, the logic is inverted for active-low GPIOs
 */
void LCD_HAL_DigitalWrite(UWORD Pin, UBYTE Value)
{
#if LCD_GPIO_INVERTED
    // Inverted GPIO operation (for active-low GPIOs with inverters on board)
    funDigitalWrite(Pin, Value ? FUN_LOW : FUN_HIGH);
#else
    // Normal (non-inverted) GPIO operation
    funDigitalWrite(Pin, Value ? FUN_HIGH : FUN_LOW);
#endif
}

/**
 * @brief Calculate SPI baud rate prescaler value
 * 
 * @param apb_clock APB2 clock frequency in Hz (typically 48MHz)
 * @param spi_speed Desired SPI speed in Hz
 * @return Prescaler value (0-7) for BR bits in CTLR1
 */
static uint8_t LCD_HAL_SPI_CalcPrescaler(uint32_t apb_clock, uint32_t spi_speed)
{
    uint32_t ratio = apb_clock / spi_speed;
    
    // Find log2(ratio) to get prescaler
    // BR bits: 0=/2, 1=/4, 2=/8, 3=/16, 4=/32, 5=/64, 6=/128, 7=/256
    if (ratio >= 256) return 7;
    if (ratio >= 128) return 6;
    if (ratio >= 64)  return 5;
    if (ratio >= 32)  return 4;
    if (ratio >= 16)  return 3;
    if (ratio >= 8)   return 2;
    if (ratio >= 4)   return 1;
    return 0; // /2
}

/**
 * @brief Initialize SPI1 peripheral for LCD communication
 * 
 * Configures SPI1 in master mode with the following settings:
 * - Mode 3 (CPOL=1, CPHA=1): Clock idle high, sample on second edge
 *   (Matches working Arduino example which uses SPI_MODE3)
 * - 8-bit data frames
 * - MSB first
 * - Software NSS management (CS controlled manually via GPIO)
 * - Clock speed as defined in LCD_SPI_SPEED_HZ
 * 
 * Pin configuration:
 * - PC5 = SPI1_SCK (clock) - alternate function push-pull
 * - PC6 = SPI1_MOSI (data out) - alternate function push-pull
 * 
 * @note The display doesn't use MISO, so it's not configured.
 * @note CS (chip select) is managed via GPIO, not hardware NSS.
 */
void LCD_HAL_SPI_Init(void)
{
    // Enable SPI1 and GPIOC clocks
    RCC->APB2PCENR |= RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOC;
    
    // Disable SPI1 to configure it
    SPI1->CTLR1 &= ~SPI_CTLR1_SPE;
    
    // Reset SPI1 configuration
    SPI1->CTLR1 = 0;
    SPI1->CTLR2 = 0;
    
    // Calculate and set baud rate prescaler
    // APB2 clock is typically FUNCONF_SYSTEM_CORE_CLOCK (48MHz)
    uint32_t apb2_clock = FUNCONF_SYSTEM_CORE_CLOCK;
    uint8_t br_value = LCD_HAL_SPI_CalcPrescaler(apb2_clock, LCD_SPI_SPEED_HZ);
    SPI1->CTLR1 |= (br_value << 3) & SPI_CTLR1_BR;
    
    // CRITICAL: Working Arduino example uses SPI_MODE3 (CPOL=1, CPHA=1)
    // Mode 3: CPOL=1 (clock idle high), CPHA=1 (sample on second edge)
    // STM32 example uses Mode 0, but Arduino (which works) uses Mode 3
    // Setting bits directly: CPOL=bit 1, CPHA=bit 0 in CTLR1
    // SPI_CPOL_High: set bit 1, SPI_CPHA_2Edge: set bit 0
    SPI1->CTLR1 |= (1 << 1) | (1 << 0);  // CPOL=1, CPHA=1 = Mode 3
    
    // Software NSS management (CS controlled via GPIO)
    SPI1->CTLR1 |= SPI_NSS_Soft;
    
    // Master mode
    SPI1->CTLR1 |= SPI_Mode_Master;
    
    // 1-line TX mode (only MOSI, no MISO)
    SPI1->CTLR1 |= SPI_Direction_1Line_Tx;
    
    // Configure SCK pin (PC5) as alternate function push-pull, 50MHz
    GPIOC->CFGLR &= ~(0xf << (4 * 5));  // Clear bits for PC5
    GPIOC->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF) << (4 * 5);
    
    // Configure MOSI pin (PC6) as alternate function push-pull, 50MHz
    GPIOC->CFGLR &= ~(0xf << (4 * 6));  // Clear bits for PC6
    GPIOC->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF) << (4 * 6);
    
    // Enable SPI1 peripheral
    SPI1->CTLR1 |= SPI_CTLR1_SPE;
}

/**
 * @brief Send a single byte over SPI
 * 
 * Waits for transmit buffer to be empty, writes the byte,
 * then waits for transmission to complete.
 * 
 * @param Value Byte to send (0x00 to 0xFF)
 */
void LCD_HAL_SPI_WriteByte(UBYTE Value)
{
    // Wait until transmit buffer is empty (TXE flag = 1)
    // If constants are undefined, use raw bit values:
    // TXE = bit 1, BSY = bit 7 in STATR register
    // Need to wait for TXE before writing, and BSY after writing for reliability
    uint32_t timeout = 100000;  // Prevent infinite hang
    while(!(SPI1->STATR & (1 << 1)) && timeout--) {}  // Check TXE bit
    if(timeout == 0) return;  // SPI not working, timeout
    
    // Write data to SPI data register
    SPI1->DATAR = Value;
    
    // Wait until transmission complete (BSY flag = 0)
    // This ensures byte is fully transmitted before continuing
    timeout = 100000;
    while((SPI1->STATR & (1 << 7)) && timeout--) {}  // Check BSY bit
}

/**
 * @brief Send multiple bytes over SPI
 * 
 * Sends an array of bytes sequentially.
 * 
 * @param pData   Pointer to data buffer
 * @param Length  Number of bytes to send
 */
void LCD_HAL_SPI_WriteBytes(uint8_t *pData, uint32_t Length)
{
    for(uint32_t i = 0; i < Length; i++) {
        LCD_HAL_SPI_WriteByte(pData[i]);
    }
}

/**
 * @brief Delay in milliseconds
 * 
 * @param ms Number of milliseconds to delay
 */
void LCD_HAL_Delay_ms(UDOUBLE ms)
{
    Delay_Ms(ms);
}

/**
 * @brief Delay in microseconds
 * 
 * @param us Number of microseconds to delay
 */
void LCD_HAL_Delay_us(UDOUBLE us)
{
    Delay_Us(us);
}

/**
 * @brief Initialize all hardware (GPIO + SPI)
 * 
 * This is the main initialization function that sets up everything.
 * Call this once at program start before using the display.
 */
void LCD_HAL_Init(void)
{
    LCD_HAL_GPIO_Init();
    LCD_HAL_SPI_Init();
}

