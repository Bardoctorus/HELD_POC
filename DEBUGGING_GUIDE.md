# Debugging Guide - No Display Output

If your display isn't showing anything, follow these steps systematically to isolate the problem.

## Quick Debugging Checklist

### 1. **Power Check** ⚡
- [ ] Is VIN connected to 3.3V or 5V?
- [ ] Is GND connected to common ground?
- [ ] Check with multimeter: Display should have power
- [ ] Try powering from a separate USB supply if MCU power is weak

### 2. **Hardware Connections** 🔌
- [ ] Verify all pins are connected correctly (see pin mapping below)
- [ ] Check for loose connections or cold solder joints
- [ ] Verify no shorts between adjacent pins
- [ ] Double-check RED pin = Reset (PD0), not a color channel!

### 3. **Test GPIO Pins** 📍

Change `DEBUG_MODE` in `src/main.c` to **1** and flash:

```c
#define DEBUG_MODE 1   // GPIO pin toggle test
```

**What to check:**
- Use a multimeter (DC voltage mode) on each pin
- Each pin should toggle between ~0V (LOW) and ~3.3V (HIGH) every 500ms
- Verify these pins toggle: PD0 (RED/RST), PD2 (CS), PD4 (DC), PD3 (BLK)
- **If pins don't toggle:** GPIO configuration issue or wrong pin definitions

### 4. **Test Backlight** 💡

Change `DEBUG_MODE` to **2**:

```c
#define DEBUG_MODE 2   // Backlight blink test
```

**What to check:**
- Backlight should blink on/off every 500ms
- **If backlight doesn't blink:** Check BLK pin connection or display may need external backlight control
- **If backlight blinks but still no display:** Display initialization or SPI communication issue

### 5. **Test Simple Display** 🎨

Change `DEBUG_MODE` to **3**:

```c
#define DEBUG_MODE 3   // Fill screen with red
```

**What to check:**
- Screen should fill with bright red color
- **If you see red:** Display is working! Problem was in the drawing function
- **If no display:** Likely SPI communication or initialization issue

### 6. **SPI Communication Verification** 📡

If you have an oscilloscope or logic analyzer:

**Check SCK (PC5):**
- Should see clock pulses during initialization and drawing
- Frequency should be ~6MHz (LCD_SPI_SPEED_HZ / 2)
- Clock should idle LOW (Mode 0)

**Check MOSI (PC6):**
- Should see data pulses synchronized with SCK
- Data should change when sending commands/data
- Should be stable during each clock edge

**Check CS (PD2):**
- Should pulse LOW during each SPI transaction
- Should be HIGH when idle

**Check DC (PD4):**
- Should be LOW when sending commands
- Should be HIGH when sending data

### 7. **Reduce SPI Speed** 🐌

If SPI seems problematic, try reducing speed. Edit `include/lcd_config.h`:

```c
#define LCD_SPI_SPEED_HZ  3000000  // Change from 6MHz to 3MHz
```

Then rebuild and flash. Sometimes slower speed helps with:
- Long wires
- Power supply issues
- Noise/interference

### 8. **Check Initialization Sequence** 🔄

The initialization sequence sends many bytes. Verify:
- Reset pulse occurs (RED pin goes LOW then HIGH)
- 120ms delay after reset (display needs time to stabilize)
- All initialization commands are sent

You can add delays after `GC9A01_Init()` to verify timing:

```c
GC9A01_Init();
LCD_HAL_Delay_ms(500);  // Extra delay
GC9A01_FillScreen(LCD_COLOR_RED);
```

### 9. **Common Issues and Solutions**

| Symptom | Possible Cause | Solution |
|---------|---------------|----------|
| No backlight | BLK not connected/powered | Connect BLK to PD3 or VCC |
| Backlight but no display | SPI not working | Check SCK/MOSI connections |
| Flickering | Power supply insufficient | Use separate 5V supply for display |
| Wrong colors | RGB order wrong | Try different 0x36 command value |
| Display upside down | Orientation wrong | Modify memory access control |
| Only some pixels | Partial connection | Check all pins, especially power |

### 10. **Verify Pin Definitions** 📋

Double-check your pin assignments in `include/lcd_config.h` match your actual wiring:

```c
#define LCD_RST_PIN   PD0   // Display RED pin → PD0
#define LCD_CS_PIN    PD2   // Display CS pin → PD2
#define LCD_DC_PIN    PD4   // Display DC pin → PD4 (NOT PD1!)
#define LCD_BL_PIN    PD3   // Display BLK pin → PD3
// SCK and MOSI are fixed: PC5 and PC6
```

**Common mistake:** Using PD1 for DC pin - this is the programming pin!

### 11. **Check Display Type** 📺

Verify you have a GC9A01 controller. Some similar displays use:
- ST7789 (different init sequence)
- ILI9341 (different init sequence)
- Other variants

If your display module part number doesn't match, initialization sequence may need changes.

### 12. **Software Debugging** 🐛

Add a simple LED blink to verify code is running:

```c
// In main(), after SystemInit()
funPinMode(PA2, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
while(1) {
    funDigitalWrite(PA2, FUN_HIGH);
    Delay_Ms(100);
    funDigitalWrite(PA2, FUN_LOW);
    Delay_Ms(100);
}
```

If LED blinks, code is running. If not, check:
- Flash successful?
- Reset button pressed?
- Code actually uploaded?

## Systematic Debugging Order

Follow this order:

1. ✅ **DEBUG_MODE 1** - Verify GPIO pins toggle
2. ✅ **DEBUG_MODE 2** - Verify backlight control
3. ✅ Check power connections with multimeter
4. ✅ Verify pin wiring matches config
5. ✅ **DEBUG_MODE 3** - Test simple screen fill
6. ✅ **DEBUG_MODE 0** - Test full stripes
7. ✅ Use scope/analyzer if available (SPI verification)
8. ✅ Try reducing SPI speed

## Still Not Working?

If none of the above works:

1. **Check display datasheet** - Verify it's actually GC9A01
2. **Try different initialization sequence** - Some displays need variant sequences
3. **Check for hardware issues** - Display might be damaged
4. **Verify MCU is working** - Try a simple blink LED program first
5. **Check power requirements** - Display might need more current than MCU can provide

## Getting Help

When asking for help, provide:
- Which DEBUG_MODE you tried and what happened
- Multimeter/scope readings (if available)
- Pin wiring confirmation
- Display model number/part number
- Any error messages during compilation

