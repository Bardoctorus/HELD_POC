# Timing Debug Instructions (No Logic Analyzer Required)

Since you don't have a logic analyzer available, use DEBUG_MODE 4 which uses GPIO pins as "software indicators" to track initialization timing.

## How to Use DEBUG_MODE 4

1. **Connect a multimeter** to pin **PA2** (or change `DEBUG_HEARTBEAT_PIN` to any unused pin)
2. **Set multimeter to DC voltage mode**
3. **Flash DEBUG_MODE 4** and observe the pin behavior

## What Each Phase Means

The heartbeat pin (PA2) will show different patterns for each initialization phase:

### Phase 1: SystemInit
- **Pattern:** 1 fast blink (50ms on/off)
- **What it means:** System initialization starting

### Phase 2: HAL Init  
- **Pattern:** 2 blinks (100ms each)
- **What it means:** GPIO and SPI peripheral initialization

### Phase 3: Reset Display
- **Pattern:** 3 blinks, then stays HIGH for 60ms, LOW for 60ms
- **What it means:** Hardware reset pulse and 120ms stabilization delay

### Phase 4: Init Registers
- **Pattern:** 4 blinks, then stays HIGH during entire init sequence
- **What it means:** Sending initialization commands via SPI
- **If pin stays HIGH for a long time:** Init commands are being sent (good)
- **If pin goes LOW quickly:** Code might be hanging or SPI not working

### Phase 5: Draw Test
- **Pattern:** 5 blinks, then slow blink (500ms on/off)
- **What it means:** Sending pixel data to display

## What to Check

### ✅ Code is Running
- You should see the phase patterns in sequence
- Pin should toggle through phases 1-5
- If you see all phases: **Code is running correctly**

### ❌ Code Hanging
- Pin stops toggling at a specific phase
- **Phase 2 stop:** HAL initialization problem (SPI config issue?)
- **Phase 4 stop:** SPI communication problem (pins not connected? SPI not working?)
- **Phase 5 stop:** Drawing function problem

### ❌ Wrong Pattern
- Pin doesn't match expected pattern
- Check if code compiled correctly
- Verify pin definitions are correct

## Troubleshooting by Phase

### If Code Hangs at Phase 2 (HAL Init)
- SPI peripheral initialization might be failing
- Check if SPI constants are defined correctly
- Try DEBUG_MODE 5 (minimal init) instead

### If Code Hangs at Phase 4 (Init Registers)
- **Most likely:** SPI communication not working
- **Check:**
  1. SCK (PC5) and MOSI (PC6) are connected correctly
  2. CS (PD2) and DC (PD4) are connected correctly  
  3. Try reducing SPI speed further (change to 1500000 in lcd_config.h)
  4. Check if SPI pins are configured correctly (alternate function mode)

### If Code Reaches Phase 5 But No Display
- SPI communication is working!
- Problem is likely:
  1. Initialization sequence not correct for your display
  2. Display needs different register values
  3. RGB order or pixel format issue
  4. Window coordinates wrong

## Additional Tests

### Try DEBUG_MODE 5 (Minimal Init)
This sends only the absolute minimum commands:
- Just Display ON (0x29)
- Then fills screen
- **If this works:** Your display might not need the full init sequence
- **If this doesn't work:** Try even more basic commands

### Reduce SPI Speed Further
In `include/lcd_config.h`, try:
```c
#define LCD_SPI_SPEED_HZ  1500000  // 1.5MHz - very slow
```
This helps if:
- Wires are long
- There's noise/interference
- Display has strict timing requirements

### Verify SPI Pin Configuration
Make sure PC5 and PC6 are not being used by other peripherals. The CH32v003 has limited pins, so conflicts are possible.

## Expected Timeline

When running DEBUG_MODE 4, here's the approximate timeline:

- **0-200ms:** Phases 1-2 (SystemInit, HAL Init)
- **200-400ms:** Phase 3 (Reset + delay)
- **400-800ms:** Phase 4 (Init commands - depends on delays)
- **800-1000ms:** Phase 5 (Draw pixels)
- **1000ms+:** Slow blink (done)

If phases take much longer or shorter, timing might be an issue.

## Next Steps Based on Results

### ✅ All Phases Complete, Still No Display
- Try DEBUG_MODE 5 (minimal init)
- Try different initialization sequences
- Check if display actually supports GC9A01 commands
- Verify display part number/model

### ❌ Code Hangs at Phase 4
- **SPI communication issue** - check connections
- Reduce SPI speed to 1.5MHz
- Verify CS/DC pins are toggling correctly (use DEBUG_MODE 1 to check)

### ❌ Code Never Starts
- Check if code compiled successfully
- Verify flash was successful
- Try simple LED blink test first

