# Board Check

[中文版本](./README_CN.md)

This is the recommended first ESP-IDF example for this repository.

It does not require any external module, display, camera, SD card, network, or
audio codec. It only prints board and runtime information through the serial
monitor, then keeps running so beginners can confirm that the toolchain,
flashing process, and serial monitor are working.

This example reports the detected ESP32-P4 chip revision in the serial output.
Use that information when choosing a target-specific configuration for your
board.

## What You Will Learn

- How to build and flash an ESP-IDF project.
- How to confirm the target is `esp32p4`.
- How to read chip, flash, PSRAM, and heap information from serial output.
- How to confirm the chip revision family before moving to peripheral demos.
- How to keep the serial monitor open and watch periodic logs.

## Hardware Required

- One supported ESP32-P4 board.
- USB cable for flashing and serial monitor.

No other hardware is required.

## Build and Flash

```bash
cd examples/esp_idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port, for example `COM7` on Windows or
`/dev/ttyACM0` on Linux.

## Expected Output

The serial monitor should show output similar to:

```text
========================================
 ESP32-P4 Platform Board Check
========================================
IDF target: esp32p4
CPU cores: 2
Flash size: 16 MB
PSRAM: initialized
Board check is running. Open the serial monitor and confirm this output.
```

The example prints an `alive` log every five seconds. If you can see that log,
your build, flash, and monitor workflow is ready for the other examples.

## Next Steps

- Run [01_i2c_tools](../01_i2c_tools/) to scan the I2C bus.
- Run [02_sdmmc](../02_sdmmc/) to verify the SD card path, if your board has an
  SD card.
- Then choose [03_wifistation](../03_wifistation/),
  [04_ethernetbasic](../04_ethernetbasic/), [05_I2SCodec](../05_I2SCodec/),
  [06_Displaycolorbar](../06_Displaycolorbar/),
  [07_lvgl_demo_v9](../07_lvgl_demo_v9/), [08_eth2ap](../08_eth2ap/),
  [09_simple_video_server](../09_simple_video_server/),
  [10_video_lcd_display](../10_video_lcd_display/),
  [11_usb_extend_screen](../11_usb_extend_screen/), or
  [12_bt_controller_mac_addr](../12_bt_controller_mac_addr/) as needed.
