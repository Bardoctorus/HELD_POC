# GC9A01 LCD Driver - Complete Walkthrough

This document provides a detailed explanation of the GC9A01 LCD driver implementation for the CH32v003F4P6 development board. It explains the architecture, how each component works, and how to bring up the display.

## Table of Contents

1. [Overview](#overview)
2. [Hardware Connections](#hardware-connections)
3. [Project Structure](#project-structure)
4. [Architecture Explanation](#architecture-explanation)
5. [Layer-by-Layer Breakdown](#layer-by-layer-breakdown)
6. [Communication Protocol](#communication-protocol)
7. [Initialization Sequence](#initialization-sequence)
8. [Testing and Debugging](#testing-and-debugging)

---

## Overview

This driver provides a bare-bones implementation to interface a 1.28" round GC9A01 display (240x240 pixels) with the CH32v003F4P6 microcontroller using SPI communication. The driver is structured in layers to allow for easy adaptation to different displays in the future.

**Key Features:**
- Hardware SPI1 communication (not bit-banged)
- Modular architecture (HAL layer + display driver)
- Well-documented code
- Ready for easy extension

---

## Hardware Connections

### Required Connections

Connect the LCD display to your CH32v003F4P6 board as follows:

**Display Pin Labels** (labels on your display board):

| Display Label | Function | CH32v003 Pin | Code Definition | Notes |
|---------------|----------|--------------|-----------------|-------|
| **VIN** | Power Supply | 3.3V or 5V | - | Check display requirements |
| **GND** | Ground | GND | - | Common ground |
| **RED** | Reset | PD0 | `LCD_RST_PIN` | Can be any GPIO |
| **CS** | Chip Select | PD2 | `LCD_CS_PIN` | Active low, managed via GPIO |
| **DC** | Data/Command | PD4 | `LCD_DC_PIN` | Low=Command, High=Data (PD1 is SWIO - avoid!) |
| **SCL** | SPI Clock | PC5 | `LCD_SCK_PIN` | Fixed - must be PC5 for SPI1 |
| **SDA** | SPI Data (MOSI) | PC6 | `LCD_MOSI_PIN` | Fixed - must be PC6 for SPI1 |
| **BLK** | Backlight | PD3 | `LCD_BL_PIN` | Optional, can tie to VCC |

**Important Notes:**
- **"SDA/SCL" labels are misleading!** These are **I2C names**, but this display uses **SPI communication**. The presence of CS and DC pins confirms this is SPI, not I2C.
- **"RED" refers to Reset** (not the red color channel)
- **PD1 is the SWIO (programming/debug) pin** - do not use it for the LCD!

### Pin Configuration

**Default pin assignments** (defined in `include/lcd_config.h`):
```c
#define LCD_RST_PIN   PD0   // Reset pin (Display label: RED)
#define LCD_DC_PIN    PD4   // Data/Command pin (Display label: DC) - Low=Command, High=Data
#define LCD_CS_PIN    PD2   // Chip Select pin (Display label: CS)
#define LCD_BL_PIN    PD3   // Backlight pin (Display label: BLK)
#define LCD_SCK_PIN   PC5   // SPI1 Clock (Display label: SCL) - fixed by hardware
#define LCD_MOSI_PIN  PC6   // SPI1 MOSI (Display label: SDA) - fixed by hardware
```

**Note:** If you need to change the control pins (RST, DC, CS, BL), edit `include/lcd_config.h`. The SPI pins (SCK, MOSI) **cannot** be changed as they are fixed by the CH32v003 hardware.

---

## Project Structure

```
HELD_POC/
├── include/
│   ├── lcd_config.h      # Pin definitions and configuration
│   └── lcd_hal.h         # Hardware abstraction layer interface
├── lib/
│   ├── lcd_hal/
│   │   └── lcd_hal.c     # SPI and GPIO implementation
│   └── gc9a01/
│       ├── gc9a01_driver.h  # GC9A01 driver interface
│       └── gc9a01_driver.c  # GC9A01 initialization and drawing
├── src/
│   └── main.c            # Main program (test stripes)
└── platformio.ini        # PlatformIO configuration
```

---

## Architecture Explanation

The driver is organized into **three layers** for modularity and future adaptability:

```
┌─────────────────────────────────────┐
│   Application Layer (main.c)        │  ← Your code calls this
├─────────────────────────────────────┤
│   Display Driver (gc9a01_driver)    │  ← Display-specific commands
├─────────────────────────────────────┤
│   Hardware Abstraction (lcd_hal)    │  ← Hardware SPI/GPIO
├─────────────────────────────────────┤
│   Hardware (CH32v003 + GC9A01)      │  ← Physical hardware
└─────────────────────────────────────┘
```

### Why This Structure?

1. **Hardware Abstraction Layer (HAL)**: Isolates hardware-specific code (SPI, GPIO). If you switch microcontrollers, only this layer needs to change.

2. **Display Driver Layer**: Contains GC9A01-specific initialization and commands. If you use a different display (e.g., ST7789), create a similar driver file with the same interface.

3. **Application Layer**: Your application code that uses the driver.

---

## Layer-by-Layer Breakdown

### 1. Configuration Layer (`include/lcd_config.h`)

**Purpose:** Central location for all pin assignments and display parameters.

**Key Definitions:**
- Pin assignments (which GPIO pins connect to LCD)
- Display dimensions (240x240 for GC9A01)
- SPI speed (default 6MHz, adjustable)

**Customization:**
To change pins, edit these defines:
```c
#define LCD_RST_PIN   PD0   // Change to your pin
#define LCD_DC_PIN    PD1   // Change to your pin
// etc.
```

### 2. Hardware Abstraction Layer (`lib/lcd_hal/`)

**Purpose:** Provides hardware-independent functions for SPI and GPIO communication.

#### Files:
- `lcd_hal.h` - Interface (function prototypes)
- `lcd_hal.c` - Implementation using CH32v003 hardware

#### Key Functions:

**GPIO Functions:**
- `LCD_HAL_GPIO_Init()` - Sets up control pins (RST, DC, CS, BL) as outputs
- `LCD_HAL_DigitalWrite()` - Write HIGH/LOW to a GPIO pin

**SPI Functions:**
- `LCD_HAL_SPI_Init()` - Configures SPI1 peripheral
- `LCD_HAL_SPI_WriteByte()` - Send single byte over SPI
- `LCD_HAL_SPI_WriteBytes()` - Send multiple bytes

**How SPI1 is Configured:**
```c
// Enable SPI1 and GPIOC clocks
RCC->APB2PCENR |= RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOC;

// Configure as master mode, Mode 0, 8-bit
SPI1->CTLR1 = SPI_Mode_Master | SPI_NSS_Soft | ...;

// Set up PC5 (SCK) and PC6 (MOSI) as alternate functions
GPIOC->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF) << ...
```

**SPI Communication Flow:**
1. Wait for TX buffer empty: `while(!(SPI1->STATR & SPI_STATR_TXE));`
2. Write data: `SPI1->DATAR = value;`
3. Wait for transmission complete: `while(SPI1->STATR & SPI_STATR_BSY);`

### 3. Display Driver Layer (`lib/gc9a01/`)

**Purpose:** GC9A01-specific commands and initialization.

#### Files:
- `gc9a01_driver.h` - Public interface (Init, DrawStripes, etc.)
- `gc9a01_driver.c` - Implementation

#### Communication Functions (Private):

**`GC9A01_SendCommand(UBYTE cmd)`**
- Sends a command byte to the display
- Sets DC low (command mode), pulls CS low, sends byte, CS high
- Used for all register commands (0x00-0xFF)

**`GC9A01_SendData(UBYTE data)`**
- Sends a data byte to the display
- Sets DC high (data mode), pulls CS low, sends byte, CS high
- Used for parameters that follow commands

**`GC9A01_SendDataBulk(uint8_t *pData, uint32_t len)`**
- Sends multiple data bytes efficiently
- Holds CS low for the entire transfer (more efficient than individual sends)

**`GC9A01_Reset()`**
- Hardware reset via RST pin
- Sequence: RST high → low (10ms) → high, then wait 120ms

#### Initialization:

**`GC9A01_InitRegisters()`**
Sends the complete initialization sequence. The sequence includes:

1. **Software Reset & Unlock** (0xFE, 0xEF, 0xEB)
   - Unlocks protected registers

2. **Power Settings** (0x84-0x8F, 0xB6)
   - VCOM voltage, LUT settings, internal pump voltage
   - Optimizes power consumption

3. **Memory Access Control** (0x36)
   - Sets orientation and RGB color order
   - Value 0x08 = normal orientation, RGB order

4. **Pixel Format** (0x3A)
   - Sets color depth to 16-bit RGB565
   - Value 0x05 = 16-bit

5. **Display Function Control** (0x90)
   - Additional display settings

6. **Additional Settings** (0xBD, 0xBC, 0xFF, 0xC3, 0xC4, 0xC9, 0xBE)
   - Various display optimization settings

7. **Gamma Correction** (0xE1, 0xE2)
   - Positive and negative gamma curves
   - Improves color accuracy and display quality

8. **Display On** (0x29)
   - Turns on the display
   - Display is ready after this command

#### Drawing Functions:

**`GC9A01_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)`**
- Sets the drawing area (window) on the display
- Uses commands:
  - 0x2A: Column Address Set (X coordinates)
  - 0x2B: Row Address Set (Y coordinates)
  - 0x2C: Memory Write (ready for pixel data)

**`GC9A01_FillRect()`**
- Fills a rectangular area with a color
- Sets window, then sends pixel data (RGB565 format)
- Uses bulk transfer for efficiency

**`GC9A01_FillScreen()`**
- Fills entire screen with a color
- Convenience wrapper around FillRect

**`GC9A01_DrawStripes()`**
- Test function that draws 8 horizontal stripes
- Alternates white and black
- Verifies display is working

---

## Communication Protocol

### SPI Protocol Details

**Mode:** SPI Mode 0
- CPOL = 0 (clock idle low)
- CPHA = 0 (sample on rising edge)
- 8-bit data frames
- MSB first

**Speed:** 6MHz (48MHz APB2 clock ÷ 8)
- Adjustable in `lcd_config.h` via `LCD_SPI_SPEED_HZ`

**CS Management:** Software controlled
- We manually control CS pin via GPIO (not hardware NSS)
- CS is pulled low before each transmission, high after

### Command vs Data

The display uses a **D/CX (Data/Command)** pin to distinguish between commands and data:

- **D/CX LOW** = Command mode (sending register addresses/commands)
- **D/CX HIGH** = Data mode (sending parameters or pixel data)

**Example:**
```c
// Send command 0x36 (Memory Access Control)
LCD_HAL_DigitalWrite(LCD_DC_PIN, 0);  // Command mode
GC9A01_SendCommand(0x36);

// Send data 0x08 (parameter for 0x36 command)
LCD_HAL_DigitalWrite(LCD_DC_PIN, 1);  // Data mode
GC9A01_SendData(0x08);
```

### Pixel Data Format (RGB565)

Each pixel is **16 bits** sent as **2 bytes** (MSB first):

```
Byte 1 (MSB): RRRRRGGG
Byte 2 (LSB): GGGGBBBB

Where:
- R = 5 bits (bits 15-11)
- G = 6 bits (bits 10-5)
- B = 5 bits (bits 4-0)
```

**Example Colors:**
- White:  0xFFFF = `11111 111111 11111`
- Black:  0x0000 = `00000 000000 00000`
- Red:    0xF800 = `11111 000000 00000`
- Green:  0x07E0 = `00000 111111 00000`
- Blue:   0x001F = `00000 000000 11111`

---

## Initialization Sequence

The complete initialization sequence (`GC9A01_Init()`) performs:

### Step 1: Hardware Reset
```
1. RST pin high
2. Wait 10ms
3. RST pin low (reset)
4. Wait 10ms
5. RST pin high (release)
6. Wait 120ms (display stabilizes)
```

### Step 2: Register Initialization
Sends approximately 70+ bytes of commands and data to configure:
- Power management
- Memory access
- Pixel format
- Display settings
- Gamma curves
- Display on

**Timing:** Total initialization takes ~200ms due to required delays.

---

## Testing and Debugging

### Basic Test Program

The provided `src/main.c` performs a simple test:

```c
int main()
{
    SystemInit();           // Initialize CH32v003 system
    LCD_HAL_Init();        // Initialize SPI and GPIO
    GC9A01_Init();         // Initialize display
    GC9A01_DrawStripes();  // Draw test pattern
    while(1) {}            // Loop forever
}
```

### Expected Result

After uploading and running, you should see:
- **8 horizontal stripes** alternating white and black
- Display should be clearly visible (if backlight is on)

### Troubleshooting

**No display output:**
1. **Check connections** - Verify all pins are connected correctly
2. **Check power** - Ensure display has power (VCC and GND)
3. **Check backlight** - Ensure BL pin is high or display is powered
4. **Verify SPI speed** - Try reducing `LCD_SPI_SPEED_HZ` in `lcd_config.h`
5. **Check pin definitions** - Ensure pins in `lcd_config.h` match your wiring

**Display shows garbled output:**
1. **SPI speed too high** - Reduce `LCD_SPI_SPEED_HZ` (try 3MHz)
2. **Wiring issues** - Check for loose connections
3. **Power supply** - Display may need more current

**Display shows wrong colors:**
1. **RGB order** - Try changing the 0x36 command data (try 0x00, 0x08, 0xC0, 0xC8)
2. **Orientation** - The display might be rotated (can fix in SetWindow)

### Debugging Tools

**SPI Verification:**
- Use a logic analyzer or oscilloscope on SCK/MOSI pins
- Should see clock pulses when sending data
- MOSI should show data bits synchronized to clock

**GPIO Verification:**
- Use multimeter or LED to verify control pins toggle correctly
- RST should pulse low during initialization
- DC should toggle between command and data modes
- CS should pulse low during each SPI transmission

---

## Customization Guide

### Changing Pin Assignments

Edit `include/lcd_config.h`:

```c
// Change these to match your wiring
#define LCD_RST_PIN   PD0   // Your reset pin
#define LCD_DC_PIN    PD1   // Your DC pin
// etc.
```

**Note:** SCK (PC5) and MOSI (PC6) cannot be changed - they are fixed by hardware.

### Adjusting SPI Speed

Edit `include/lcd_config.h`:

```c
// Reduce if display has issues, increase if display works well
#define LCD_SPI_SPEED_HZ  6000000  // 6MHz
```

Common speeds:
- 3MHz - Very reliable, slow
- 6MHz - Good balance (default)
- 12MHz - Fast, may have issues with long wires

### Adding More Colors

Edit `lib/gc9a01/gc9a01_driver.h`:

```c
#define LCD_COLOR_ORANGE  0xFD20  // RGB(31, 41, 0)
// Add more as needed
```

### Creating Custom Drawing Functions

Add to `lib/gc9a01/gc9a01_driver.c`:

```c
void GC9A01_DrawSomething(void)
{
    // Set window
    GC9A01_SetWindow(x0, y0, x1, y1);
    
    // Send pixel data
    // Use GC9A01_SendDataBulk() for efficiency
}
```

---

## Technical Details

### CH32v003 SPI1 Registers

The driver uses direct register access:

- **`SPI1->CTLR1`** - Control Register 1
  - `SPI_CTLR1_SPE` - Enable SPI
  - `SPI_CTLR1_MSTR` - Master mode
  - `SPI_CTLR1_BR` - Baud rate mask
  - `SPI_CPOL_Low`, `SPI_CPHA_1Edge` - Mode 0

- **`SPI1->STATR`** - Status Register
  - `SPI_STATR_TXE` - Transmit buffer empty
  - `SPI_STATR_BSY` - Busy flag

- **`SPI1->DATAR`** - Data Register (write to send)

- **`GPIOC->CFGLR`** - GPIO Configuration Register
  - Configures pins as alternate function push-pull

### Clock Calculation

SPI baud rate is calculated as:
```
SPI_Clock = APB2_Clock / (2^(BR+1))

Where BR (Baud Rate bits) = 0 to 7
BR=0: /2,  BR=1: /4,  BR=2: /8,  BR=3: /16
BR=4: /32, BR=5: /64, BR=6: /128, BR=7: /256

At 48MHz APB2:
BR=2 → 48MHz/8 = 6MHz
BR=3 → 48MHz/16 = 3MHz
```

The driver automatically calculates the correct BR value based on `LCD_SPI_SPEED_HZ`.

---

## Future Enhancements

The modular architecture makes it easy to add:

1. **More display drivers** - Create similar driver files for other displays (ST7789, ILI9341, etc.)
2. **Drawing primitives** - Add functions for lines, circles, text
3. **Bitmaps** - Add functions to display images
4. **Double buffering** - For smooth animations
5. **Rotation support** - Change display orientation

---

## Summary

This driver provides a **complete, working foundation** for driving the GC9A01 display with the CH32v003. The modular structure ensures:

- ✅ Easy to understand and modify
- ✅ Easy to adapt for other displays
- ✅ Well-documented for team collaboration
- ✅ Uses efficient hardware SPI
- ✅ Ready for extension

The code is production-ready for basic display operations and provides a solid base for building more advanced graphics functionality.

