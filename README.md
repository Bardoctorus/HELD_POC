
# Just don't even bother this is a mess and it doesn't work


I don't know if the little round display is broken, I don't understand the way the ch32 does SPI, my attempt to get AI to compare what I did to working arduino code was foolish, or all of the above. This whole repository probably needs to go the way of the project. Away, to be reborn as something that isn't 9 hours of no progress. aaaaaah



How to build PlatformIO based project
=====================================

See <https://github.com/cnlohr/ch32v003fun/tree/master/examples/i2c_oled>

1. [Install PlatformIO Core](https://docs.platformio.org/page/core.html)
2. Download [development platform with examples](https://github.com/Community-PIO-CH32V/platform-ch32v/archive/develop.zip)
3. Extract ZIP archive
4. Run these commands:

```shell
# Change directory to example
$ cd platform-ch32v/examples/ch32v003fun-oled

# Build project
$ pio run

# Upload firmware
$ pio run --target upload

# Upload firmware for the specific environment
$ pio run -e ch32v003f4p6_evt_r0 --target upload

# Clean build files
$ pio run --target clean
```
