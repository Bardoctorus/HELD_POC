/**
 * @file main.c
 * @brief Main program - GC9A01 LCD Debugging and Test
 * 
 * This program initializes the GC9A01 display with debugging capabilities.
 * 
 * Hardware Required:
 * - CH32v003F4P6 development board
 * - 1.28" GC9A01/GC9101 round LCD display (240x240)
 *   NOTE: Some displays are marked GC9101 but use GC9A01-compatible commands
 *   NOTE: Display board contains SN74LVCC3245A level translator - use slower SPI speeds
 * 
 * Connections (Display pin labels in parentheses):
 * - RED (Reset):    PD0
 * - CS:             PD2
 * - DC:             PD4 (Note: PD1 is SWIO/programming pin, avoid!)
 * - SCL (Clock):    PC5 (SPI1_SCK - fixed pin)
 * - SDA (Data):     PC6 (SPI1_MOSI - fixed pin)
 * - BLK (Backlight): PD3 (optional, can be tied to VCC)
 * - VIN:            Power (3.3V or 5V)
 * - GND:            Ground
 * 
 * Note: Display uses "SDA/SCL" labels (normally I2C), but this is SPI communication.
 * 
 * DEBUGGING MODES:
 * Set DEBUG_MODE to one of the following:
 * 0 = Normal operation (stripes test)
 * 1 = GPIO pin toggle test (verify hardware connections)
 * 2 = Backlight blink test (verify backlight control)
 * 3 = Fill screen with single color (simplest display test)
 * 4 = Step-by-step initialization with GPIO heartbeat (timing debug)
 * 5 = Minimal init test (basic commands only)
 * 6 = SPI communication test (verify SPI is working)
 * 7 = SPI register verification test (check SPI is configured correctly)
 * 8 = Alternative init test (tries different register values)
 * 9 = Comprehensive test with slower SPI, CS timing, and alternative init sequences
 */

#include "ch32fun.h"
#include "lcd_hal.h"
#include "gc9a01_driver.h"

// ============================================================================
// DEBUGGING MODE SELECTION
// ============================================================================
#define DEBUG_MODE 3   // Change this to enable different test modes

// GPIO pin for heartbeat indicator (use multimeter to check)
#define DEBUG_HEARTBEAT_PIN PC0  // Change this to any unused pin

#if DEBUG_MODE == 0
// Normal operation: Draw stripes
void run_normal_test(void)
{
    // Step 1: System initialization
    SystemInit();
    
    // Step 2: Initialize hardware abstraction layer
    LCD_HAL_Init();
    
    // Step 3: Initialize GC9A01 display
    GC9A01_Init();
    
    // Step 4: Draw test stripes
    GC9A01_DrawStripes();
    
    // Main loop - stripes are drawn
    while(1) {}
}

#elif DEBUG_MODE == 1
// GPIO Pin Toggle Test
// This will make pins toggle so you can verify with multimeter/scope
void run_gpio_test(void)
{
    SystemInit();
    funGpioInitAll();
    
    // Configure all LCD pins as outputs
    funPinMode(LCD_RST_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_DC_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_CS_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_BL_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    
    while(1)
    {
        // Toggle each pin in sequence with delays
        // Use multimeter or scope to verify each pin toggles
        
        funDigitalWrite(LCD_RST_PIN, FUN_HIGH);
        Delay_Ms(500);
        funDigitalWrite(LCD_RST_PIN, FUN_LOW);
        Delay_Ms(500);
        
        funDigitalWrite(LCD_DC_PIN, FUN_HIGH);
        Delay_Ms(500);
        funDigitalWrite(LCD_DC_PIN, FUN_LOW);
        Delay_Ms(500);
        
        funDigitalWrite(LCD_CS_PIN, FUN_HIGH);
        Delay_Ms(500);
        funDigitalWrite(LCD_CS_PIN, FUN_LOW);
        Delay_Ms(500);
        
        funDigitalWrite(LCD_BL_PIN, FUN_HIGH);
        Delay_Ms(500);
        funDigitalWrite(LCD_BL_PIN, FUN_LOW);
        Delay_Ms(500);
    }
}

#elif DEBUG_MODE == 2
// Backlight Blink Test
// If backlight blinks, hardware connections are likely OK
void run_backlight_test(void)
{
    SystemInit();
    LCD_HAL_Init();
    funPinMode(DEBUG_HEARTBEAT_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);

    while(1)
    {
        LCD_HAL_DigitalWrite(LCD_BL_PIN, 1);  // Backlight on
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        LCD_HAL_Delay_ms(5000);
        LCD_HAL_DigitalWrite(LCD_BL_PIN, 0);  // Backlight off
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);

        LCD_HAL_Delay_ms(500);
    }
}

#elif DEBUG_MODE == 3
// Simple Fill Screen Test
// Just fills the entire screen with red (most visible)
// This uses the FULL GC9A01 initialization sequence
void run_fill_screen_test(void)
{
    SystemInit();
    LCD_HAL_Init();
    
    // Longer delays everywhere for stability
    LCD_HAL_Delay_ms(200);
    
    // Full initialization sequence
    GC9A01_Init();
    
    // Extra delay after init
    LCD_HAL_Delay_ms(200);
    
    // Fill screen with black first to clear it
    GC9A01_FillScreen(LCD_COLOR_BLACK);
    LCD_HAL_Delay_ms(1000);
    
    // Test 1: Fill screen with bright red (most visible)
    GC9A01_FillScreen(LCD_COLOR_RED);
    LCD_HAL_Delay_ms(3000);
    
    // Test 2: Try a small green 10x10 square in the corner to verify coordinates
    // This will help us see if coordinate addressing is working correctly
    // Fill screen black first, then draw square at (10, 10) to (20, 20)
    GC9A01_FillScreen(LCD_COLOR_BLACK);
    LCD_HAL_Delay_ms(500);
    GC9A01_FillRect(10, 10, 20, 20, LCD_COLOR_GREEN);  // 10x10 square starting at (10,10)
    LCD_HAL_Delay_ms(5000);
    
    // Test 3: Try another square in different position to verify coordinates
    GC9A01_FillScreen(LCD_COLOR_BLACK);
    LCD_HAL_Delay_ms(500);
    GC9A01_FillRect(100, 100, 110, 110, LCD_COLOR_GREEN);  // Square in center area
    LCD_HAL_Delay_ms(5000);
    
    // Test 4: Try a larger square to see scaling
    GC9A01_FillScreen(LCD_COLOR_BLACK);
    LCD_HAL_Delay_ms(500);
    GC9A01_FillRect(50, 50, 100, 100, LCD_COLOR_GREEN);  // 50x50 square
    LCD_HAL_Delay_ms(5000);
    
    // Test 5: Fill with blue to test another color
    GC9A01_FillScreen(LCD_COLOR_BLUE);
    LCD_HAL_Delay_ms(3000);
    
    // Test 6: Fill with green
    GC9A01_FillScreen(LCD_COLOR_GREEN);
    LCD_HAL_Delay_ms(3000);
    
    // Test 7: Fill with white
    GC9A01_FillScreen(LCD_COLOR_WHITE);
    LCD_HAL_Delay_ms(3000);
    
    // Final: Fill with black and leave it
    GC9A01_FillScreen(LCD_COLOR_BLACK);
    
    while(1) {}
}

#elif DEBUG_MODE == 4
// Step-by-step initialization with GPIO heartbeat
// Use multimeter on DEBUG_HEARTBEAT_PIN to see which phase code is in
void run_step_by_step_debug(void)
{

    SystemInit();
    funGpioInitAll();

    funPinMode(DEBUG_HEARTBEAT_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);

    //idiot check
    // funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
    // Delay_Ms(1000);
    // funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
    // Delay_Ms(1000);
    
    // Phase 1: SystemInit (fast blink) - delays x4 for visibility
    // NOTE: Swapped FUN_HIGH/FUN_LOW for active-low GPIOs
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
    Delay_Ms(200);  // was 50
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
    Delay_Ms(2000);  // was 50
    
    
    // Phase 2: HAL Init (2 blinks) - delays x4
    for(int i = 0; i < 2; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(400);  // was 100
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(400);  // was 100
    }
    Delay_Ms(2000);  // Extra delay before next phase
    LCD_HAL_Init();
    
    // Phase 3: Reset display (3 blinks, then longer delay) - delays x4
    for(int i = 0; i < 3; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(400);  // was 100
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(400);  // was 100
    }
    
    // Hardware reset - delays x4
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 0);
    LCD_HAL_Delay_ms(40);  // was 10
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    
    // Heartbeat during 480ms delay (was 120ms) - delays x4
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
    LCD_HAL_Delay_ms(240);  // was 60
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
    LCD_HAL_Delay_ms(240);  // was 60
    
    // Phase 4: Init registers (4 blinks, then continuous low during init) - delays x4
    for(int i = 0; i < 4; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(400);  // was 100
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(400);  // was 100
    }
    
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // Stay low during init (LED on for active-low)
    
    // Minimal init: Only essential commands - delays x4
    // Software reset unlock
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // Command
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0xFE);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0xEF);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    // Register unlock
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0xEB);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // Data
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x14);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    // Memory access control
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x36);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x08);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    // Pixel format: 16-bit
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x3A);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x05);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(40);  // was 10
    
    // Display on
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x29);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(200);  // was 50
    
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
    
    // Phase 5: Draw test (5 blinks) - delays x4
    for(int i = 0; i < 5; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(400);  // was 100
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(400);  // was 100
    }
    
    // Simple window set and fill - delays x4
    // Column address set (0x2A)
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x2A);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(4);  // was 1
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0xEF);  // 239 (240-1)
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(4);  // was 1
    
    // Row address set (0x2B)
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x2B);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(4);  // was 1
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0xEF);  // 239
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(4);  // was 1
    
    // Memory write (0x2C) - send red pixels
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x2C);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(4);  // was 1
    
    // Send pixel data (red = 0xF800 = 11111 000000 00000)
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    // Send only 100 pixels to start (faster test)
    for(int i = 0; i < 100; i++) {
        LCD_HAL_SPI_WriteByte(0xF8);  // High byte of red
        LCD_HAL_SPI_WriteByte(0x00);  // Low byte of red
    }
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    
    // Done - slow blink (delays x4)
    while(1) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(2000);  // was 500
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(3000);  // was 500
    }
}

#elif DEBUG_MODE == 5
// Minimal init test - absolute minimum commands
void run_minimal_init(void)
{
    SystemInit();
    LCD_HAL_Init();
    
    // Longer delays everywhere
    LCD_HAL_Delay_ms(200);
    
    // Reset
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    LCD_HAL_Delay_ms(20);
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 0);
    LCD_HAL_Delay_ms(20);
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    LCD_HAL_Delay_ms(200);  // Extra long delay
    
    // Only send Display On
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x29);  // Display ON
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(100);
    
    // Try to fill screen
    GC9A01_FillScreen(LCD_COLOR_RED);
    
    while(1) {}
}

#elif DEBUG_MODE == 6
// SPI Communication Test
// Each phase has a unique LED pattern for easy identification:
// Phase 1: 3 SHORT blinks (200ms) - CS pin test
// Phase 2: 4 SHORT blinks (200ms) - DC pin test  
// Phase 3: STEADY ON for 3 seconds - SPI transmission test
// Phase 4: 2 LONG blinks (1000ms) - Reset phase
// Phase 5: 5 FAST blinks (100ms) - Display command phase
// Phase 6: SLOW pulse (500ms on/off, 3 times) - Drawing phase
// Final: VERY SLOW blink (2000ms) - Complete
void run_spi_test(void)
{
    SystemInit();
    funGpioInitAll();
    funPinMode(DEBUG_HEARTBEAT_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    
    // Long pause to see LED starts
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
    Delay_Ms(2000);
    
    // Initialize HAL (SPI + GPIO)
    LCD_HAL_Init();
    
    // ========================================================================
    // PHASE 1: CS PIN TEST - 3 SHORT BLINKS (200ms on/off)
    // ========================================================================
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(1500);
    for(int i = 0; i < 3; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);  // CS low (select)
        Delay_Ms(200);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // CS high (deselect)
        Delay_Ms(200);
    }
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(1500);
    
    // ========================================================================
    // PHASE 2: DC PIN TEST - 4 SHORT BLINKS (200ms on/off)
    // ========================================================================
    for(int i = 0; i < 4; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // DC low (command)
        Delay_Ms(200);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // DC high (data)
        Delay_Ms(200);
    }
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(1500);
    
    // ========================================================================
    // PHASE 3: SPI TRANSMISSION TEST - STEADY ON for 3 seconds
    // ========================================================================
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // LED on - steady
    Delay_Ms(500);  // Brief pause
    
    // Send test pattern: 0xAA (10101010)
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // Command mode
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);  // Select
    for(int i = 0; i < 50; i++) {
        LCD_HAL_SPI_WriteByte(0xAA);  // Test pattern
    }
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // Deselect
    Delay_Ms(100);
    
    // Send another pattern: 0x55 (01010101)
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);  // Select
    for(int i = 0; i < 50; i++) {
        LCD_HAL_SPI_WriteByte(0x55);  // Test pattern
    }
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // Deselect
    
    Delay_Ms(2000);  // Keep LED on for visibility
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(1500);
    
    // ========================================================================
    // PHASE 4: RESET PHASE - 2 LONG BLINKS (1000ms on/off)
    // ========================================================================
    for(int i = 0; i < 2; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on - long
        Delay_Ms(1000);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - long
        Delay_Ms(1000);
    }
    
    // Do actual reset
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on during reset
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    Delay_Ms(50);
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 0);
    Delay_Ms(50);
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    Delay_Ms(500);  // Stabilization delay
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(1500);
    
    // ========================================================================
    // PHASE 5: DISPLAY COMMAND PHASE - 5 FAST BLINKS (100ms on/off)
    // ========================================================================
    for(int i = 0; i < 5; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on - fast
        Delay_Ms(100);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - fast
        Delay_Ms(100);
    }
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(500);
    
    // Send Display ON command
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on during command
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // Command
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);  // Select
    LCD_HAL_SPI_WriteByte(0x29);  // Display ON
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);  // Deselect
    Delay_Ms(100);
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(1500);
    
    // ========================================================================
    // PHASE 6: DRAWING PHASE - SLOW PULSE (500ms on/off, 3 times)
    // ========================================================================
    for(int i = 0; i < 3; i++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on - slow
        Delay_Ms(500);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - slow
        Delay_Ms(500);
    }
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - pause
    Delay_Ms(500);
    
    // Set window (0,0 to 239,239)
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on during drawing
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // Command
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x2A);  // Column address
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    Delay_Ms(5);
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // Data
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0xEF);  // 239
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    Delay_Ms(5);
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // Command
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x2B);  // Row address
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    Delay_Ms(5);
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // Data
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0x00);
    LCD_HAL_SPI_WriteByte(0xEF);  // 239
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    Delay_Ms(5);
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // Command
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x2C);  // Memory write
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    Delay_Ms(5);
    
    // Send red pixels (1000 pixels for test)
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // Data
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    for(int i = 0; i < 1000; i++) {
        LCD_HAL_SPI_WriteByte(0xF8);  // Red high byte
        LCD_HAL_SPI_WriteByte(0x00);  // Red low byte
    }
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    Delay_Ms(500);
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
    
    // ========================================================================
    // FINAL: VERY SLOW BLINK (2000ms) = Test complete
    // ========================================================================
    Delay_Ms(2000);
    while(1) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(2000);  // Very slow
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(2000);  // Very slow
    }
}

#elif DEBUG_MODE == 7
// SPI Register Verification Test
// Uses LED patterns to indicate SPI register state
// Pattern: 10 blinks if SPI is configured, 1 blink if not
void run_spi_register_test(void)
{
    SystemInit();
    funGpioInitAll();
    funPinMode(DEBUG_HEARTBEAT_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    
    // Initialize HAL
    LCD_HAL_Init();
    
    Delay_Ms(2000);
    
    // Check SPI1 register state
    // If SPI is configured, CTLR1 should have SPE bit set (bit 6)
    // STATR TXE should be 1 when ready (bit 1)
    
    uint32_t ctlr1_value = SPI1->CTLR1;
    uint32_t statr_value = SPI1->STATR;
    
    // Blink pattern indicates register state
    // 10 blinks = SPI looks configured
    // 1 blink = SPI not configured
    
    if((ctlr1_value & (1 << 6)) != 0) {  // SPE bit set?
        // SPI enabled - blink 10 times (200ms each)
        for(int i = 0; i < 10; i++) {
            funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
            Delay_Ms(200);
            funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
            Delay_Ms(200);
        }
    } else {
        // SPI not enabled - 1 long blink (1000ms)
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(1000);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
    }
    
    Delay_Ms(2000);
    
    // Try sending a byte and check if TXE clears
    SPI1->DATAR = 0xAA;
    Delay_Ms(10);
    
    // Check TXE bit (should clear when data sent)
    uint32_t statr_after = SPI1->STATR;
    
    // Blink pattern: 5 blinks if TXE changed, 2 blinks if not
    if((statr_value & (1 << 1)) != (statr_after & (1 << 1))) {
        // TXE changed - SPI might be working
        for(int i = 0; i < 5; i++) {
            funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
            Delay_Ms(200);
            funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
            Delay_Ms(200);
        }
    } else {
        // TXE didn't change - SPI not working
        for(int i = 0; i < 2; i++) {
            funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
            Delay_Ms(500);
            funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
            Delay_Ms(500);
        }
    }
    
    // Final: Very slow blink
    while(1) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(2000);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(2000);
    }
}

#elif DEBUG_MODE == 8
// Alternative Initialization Test
// Tries different memory access control values and simpler init sequences
void run_alternative_init_test(void)
{
    SystemInit();
    funGpioInitAll();
    funPinMode(DEBUG_HEARTBEAT_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    
    LCD_HAL_Init();
    LCD_HAL_Delay_ms(200);
    
    // Reset
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // LED on = reset
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    LCD_HAL_Delay_ms(50);
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 0);
    LCD_HAL_Delay_ms(20);
    LCD_HAL_DigitalWrite(LCD_RST_PIN, 1);
    LCD_HAL_Delay_ms(150);
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
    LCD_HAL_Delay_ms(1000);
    
    // Try minimal init with different register values
    // Test 1: Basic unlock and simple settings
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // LED on = init
    LCD_HAL_Delay_ms(500);
    
    // Software reset unlock
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0xFE);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(10);
    
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0xEF);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(10);
    
    // Register unlock
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0xEB);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(10);
    
    LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
    LCD_HAL_SPI_WriteByte(0x14);
    LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
    LCD_HAL_Delay_ms(10);
    
    // Memory access control - try different values
    // Some displays need 0x00, 0x08, 0xC0, or 0xC8
    uint8_t mac_values[] = {0x08, 0x00, 0xC0, 0xC8};
    
    for(int test = 0; test < 4; test++) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
        LCD_HAL_Delay_ms(1000);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);   // LED on for test
        LCD_HAL_Delay_ms(500);
        
        // Memory access control
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x36);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(5);
        
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(mac_values[test]);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(5);
        
        // Pixel format: 16-bit
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x3A);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(5);
        
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x05);  // 16-bit
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(5);
        
        // Display on
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x29);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(50);
        
        // Try to fill screen with red
        // Set window
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x2A);  // Column
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(1);
        
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x00);
        LCD_HAL_SPI_WriteByte(0x00);
        LCD_HAL_SPI_WriteByte(0x00);
        LCD_HAL_SPI_WriteByte(0xEF);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(1);
        
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x2B);  // Row
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(1);
        
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x00);
        LCD_HAL_SPI_WriteByte(0x00);
        LCD_HAL_SPI_WriteByte(0x00);
        LCD_HAL_SPI_WriteByte(0xEF);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(1);
        
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        LCD_HAL_SPI_WriteByte(0x2C);  // Memory write
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        LCD_HAL_Delay_ms(1);
        
        // Send red pixels (full screen - 240x240 = 57,600 pixels)
        LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 0);
        for(int i = 0; i < 57600; i++) {
            LCD_HAL_SPI_WriteByte(0xF8);  // Red MSB
            LCD_HAL_SPI_WriteByte(0x00);  // Red LSB
        }
        LCD_HAL_DigitalWrite(LCD_CS_PIN, 1);
        
        // Wait to see if display shows anything
        LCD_HAL_Delay_ms(2000);
    }
    
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off - done
    
    // Very slow blink
    while(1) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(2000);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(2000);
    }
}

#elif DEBUG_MODE == 9
// Comprehensive Timing and Initialization Test
// Tests multiple variations: slower SPI, CS timing, alternative init sequences
void run_comprehensive_timing_test(void)
{
    SystemInit();
    funGpioInitAll();
    funPinMode(DEBUG_HEARTBEAT_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    
    // Initialize GPIO only (SPI will be initialized separately for each test)
    funGpioInitAll();
    funPinMode(LCD_RST_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_DC_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_CS_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(LCD_BL_PIN,  GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    
    // Set initial states
    funDigitalWrite(LCD_CS_PIN, FUN_HIGH);  // CS high (inactive)
    funDigitalWrite(LCD_DC_PIN, FUN_LOW);   // DC low (command mode)
    funDigitalWrite(LCD_RST_PIN, FUN_HIGH); // Reset high (not resetting)
    funDigitalWrite(LCD_BL_PIN, FUN_HIGH);  // Backlight on
    
    // Helper function to reinitialize SPI at different speed
    void init_spi_at_speed(uint32_t speed_hz) {
        // Enable SPI1 and GPIOC clocks
        RCC->APB2PCENR |= RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOC;
        
        // Disable SPI1 to configure it
        SPI1->CTLR1 &= ~SPI_CTLR1_SPE;
        SPI1->CTLR1 = 0;
        SPI1->CTLR2 = 0;
        
        // Calculate prescaler
        uint32_t apb2_clock = FUNCONF_SYSTEM_CORE_CLOCK; // 48MHz
        uint32_t ratio = apb2_clock / speed_hz;
        uint8_t br_value = 0;
        if (ratio >= 256) br_value = 7;
        else if (ratio >= 128) br_value = 6;
        else if (ratio >= 64) br_value = 5;
        else if (ratio >= 32) br_value = 4;
        else if (ratio >= 16) br_value = 3;
        else if (ratio >= 8) br_value = 2;
        else if (ratio >= 4) br_value = 1;
        
        SPI1->CTLR1 |= (br_value << 3) & 0x38; // BR bits
        SPI1->CTLR1 |= (SPI_CPOL_Low | SPI_CPHA_1Edge);
        SPI1->CTLR1 |= SPI_NSS_Soft;
        SPI1->CTLR1 |= SPI_Mode_Master;
        SPI1->CTLR1 |= SPI_Direction_1Line_Tx;
        
        // Configure pins as alternate function
        GPIOC->CFGLR &= ~(0xf << (4 * 5));  // Clear PC5
        GPIOC->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF) << (4 * 5);
        GPIOC->CFGLR &= ~(0xf << (4 * 6));  // Clear PC6
        GPIOC->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF) << (4 * 6);
        
        SPI1->CTLR1 |= SPI_CTLR1_SPE;
    }
    
    // Helper: Send command with data, CS held low throughout
    void send_cmd_with_data_cs_low(uint8_t cmd, uint8_t *data, uint8_t len) {
        funDigitalWrite(LCD_CS_PIN, FUN_LOW);   // CS low
        Delay_Us(5);  // Delay for CS
        
        // Send command
        funDigitalWrite(LCD_DC_PIN, FUN_LOW);   // DC low = command
        Delay_Us(2);
        uint32_t timeout = 100000;
        while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
        SPI1->DATAR = cmd;
        timeout = 100000;
        while((SPI1->STATR & (1 << 7)) && timeout--) {}
        
        // Send data if any
        if (len > 0 && data != NULL) {
            funDigitalWrite(LCD_DC_PIN, FUN_HIGH);  // DC high = data
            Delay_Us(2);
            for (uint8_t i = 0; i < len; i++) {
                timeout = 100000;
                while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
                SPI1->DATAR = data[i];
                timeout = 100000;
                while((SPI1->STATR & (1 << 7)) && timeout--) {}
            }
        }
        
        Delay_Us(5);
        funDigitalWrite(LCD_CS_PIN, FUN_HIGH);  // CS high
        Delay_Ms(5);  // Delay between commands
    }
    
    LCD_HAL_Delay_ms(200);
    
    // Perform reset (all tests use same reset)
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // LED on = reset
    funDigitalWrite(LCD_RST_PIN, FUN_HIGH);
    LCD_HAL_Delay_ms(50);
    funDigitalWrite(LCD_RST_PIN, FUN_LOW);
    LCD_HAL_Delay_ms(20);
    funDigitalWrite(LCD_RST_PIN, FUN_HIGH);
    LCD_HAL_Delay_ms(150);
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
    LCD_HAL_Delay_ms(1000);
    
    // TEST 1: 750kHz SPI with CS held low for sequences
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // LED on = test 1
    LCD_HAL_Delay_ms(500);
    init_spi_at_speed(750000);  // 750kHz - very slow for level translator
    LCD_HAL_Delay_ms(50);
    
    // Minimal init with CS held low
    uint8_t unlock[] = {0x14};
    send_cmd_with_data_cs_low(0xFE, NULL, 0);
    send_cmd_with_data_cs_low(0xEF, NULL, 0);
    send_cmd_with_data_cs_low(0xEB, unlock, 1);
    
    uint8_t mac[] = {0x08};
    send_cmd_with_data_cs_low(0x36, mac, 1);
    
    uint8_t pixfmt[] = {0x05};
    send_cmd_with_data_cs_low(0x3A, pixfmt, 1);
    
    send_cmd_with_data_cs_low(0x29, NULL, 0);  // Display on
    LCD_HAL_Delay_ms(50);
    
    // Try to fill screen - set window
    uint8_t win_x[] = {0x00, 0x00, 0x00, 0xEF};
    uint8_t win_y[] = {0x00, 0x00, 0x00, 0xEF};
    send_cmd_with_data_cs_low(0x2A, win_x, 4);
    send_cmd_with_data_cs_low(0x2B, win_y, 4);
    send_cmd_with_data_cs_low(0x2C, NULL, 0);
    
    // Send red pixels with CS held low
    funDigitalWrite(LCD_CS_PIN, FUN_LOW);
    funDigitalWrite(LCD_DC_PIN, FUN_HIGH);  // Data mode
    Delay_Us(5);
    for(int i = 0; i < 57600; i++) {
        uint32_t timeout = 100000;
        while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
        SPI1->DATAR = 0xF8;  // Red MSB
        timeout = 100000;
        while((SPI1->STATR & (1 << 7)) && timeout--) {}
        timeout = 100000;
        while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
        SPI1->DATAR = 0x00;  // Red LSB
        timeout = 100000;
        while((SPI1->STATR & (1 << 7)) && timeout--) {}
    }
    funDigitalWrite(LCD_CS_PIN, FUN_HIGH);
    
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);  // LED off
    LCD_HAL_Delay_ms(3000);  // Wait to see result
    
    // TEST 2: 1MHz SPI with different MAC value (0x00)
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // LED on = test 2
    LCD_HAL_Delay_ms(500);
    init_spi_at_speed(1000000);  // 1MHz
    LCD_HAL_Delay_ms(50);
    
    send_cmd_with_data_cs_low(0xFE, NULL, 0);
    send_cmd_with_data_cs_low(0xEF, NULL, 0);
    send_cmd_with_data_cs_low(0xEB, unlock, 1);
    
    uint8_t mac2[] = {0x00};  // Different MAC
    send_cmd_with_data_cs_low(0x36, mac2, 1);
    send_cmd_with_data_cs_low(0x3A, pixfmt, 1);
    send_cmd_with_data_cs_low(0x29, NULL, 0);
    LCD_HAL_Delay_ms(50);
    
    send_cmd_with_data_cs_low(0x2A, win_x, 4);
    send_cmd_with_data_cs_low(0x2B, win_y, 4);
    send_cmd_with_data_cs_low(0x2C, NULL, 0);
    
    funDigitalWrite(LCD_CS_PIN, FUN_LOW);
    funDigitalWrite(LCD_DC_PIN, FUN_HIGH);
    Delay_Us(5);
    for(int i = 0; i < 57600; i++) {
        uint32_t timeout = 100000;
        while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
        SPI1->DATAR = 0xF8;
        timeout = 100000;
        while((SPI1->STATR & (1 << 7)) && timeout--) {}
        timeout = 100000;
        while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
        SPI1->DATAR = 0x00;
        timeout = 100000;
        while((SPI1->STATR & (1 << 7)) && timeout--) {}
    }
    funDigitalWrite(LCD_CS_PIN, FUN_HIGH);
    
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
    LCD_HAL_Delay_ms(3000);
    
    // TEST 3: 500kHz SPI - very slow
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);  // LED on = test 3
    LCD_HAL_Delay_ms(500);
    init_spi_at_speed(500000);  // 500kHz - extremely slow
    LCD_HAL_Delay_ms(50);
    
    send_cmd_with_data_cs_low(0xFE, NULL, 0);
    send_cmd_with_data_cs_low(0xEF, NULL, 0);
    send_cmd_with_data_cs_low(0xEB, unlock, 1);
    send_cmd_with_data_cs_low(0x36, mac, 1);
    send_cmd_with_data_cs_low(0x3A, pixfmt, 1);
    send_cmd_with_data_cs_low(0x29, NULL, 0);
    LCD_HAL_Delay_ms(50);
    
    send_cmd_with_data_cs_low(0x2A, win_x, 4);
    send_cmd_with_data_cs_low(0x2B, win_y, 4);
    send_cmd_with_data_cs_low(0x2C, NULL, 0);
    
    funDigitalWrite(LCD_CS_PIN, FUN_LOW);
    funDigitalWrite(LCD_DC_PIN, FUN_HIGH);
    Delay_Us(5);
    for(int i = 0; i < 57600; i++) {
        uint32_t timeout = 100000;
        while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
        SPI1->DATAR = 0xF8;
        timeout = 100000;
        while((SPI1->STATR & (1 << 7)) && timeout--) {}
        timeout = 100000;
        while(!(SPI1->STATR & (1 << 1)) && timeout--) {}
        SPI1->DATAR = 0x00;
        timeout = 100000;
        while((SPI1->STATR & (1 << 7)) && timeout--) {}
    }
    funDigitalWrite(LCD_CS_PIN, FUN_HIGH);
    
    funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
    LCD_HAL_Delay_ms(3000);
    
    // Done - slow blink
    while(1) {
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_LOW);
        Delay_Ms(2000);
        funDigitalWrite(DEBUG_HEARTBEAT_PIN, FUN_HIGH);
        Delay_Ms(2000);
    }
}

#else
#error "Invalid DEBUG_MODE value"
#endif

/**
 * @brief Main program entry point
 */
int main()
{
#if DEBUG_MODE == 0
    run_normal_test();
#elif DEBUG_MODE == 1
    run_gpio_test();
#elif DEBUG_MODE == 2
    run_backlight_test();
#elif DEBUG_MODE == 3
    run_fill_screen_test();
#elif DEBUG_MODE == 4
    run_step_by_step_debug();
#elif DEBUG_MODE == 5
    run_minimal_init();
#elif DEBUG_MODE == 6
    run_spi_test();
#elif DEBUG_MODE == 7
    run_spi_register_test();
#elif DEBUG_MODE == 8
    run_alternative_init_test();
#elif DEBUG_MODE == 9
    run_comprehensive_timing_test();
#endif
}

