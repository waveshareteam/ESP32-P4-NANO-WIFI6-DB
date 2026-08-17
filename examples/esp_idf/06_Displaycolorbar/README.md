# Display Color Bar

[中文版本](./README_CN.md)

Display color bars on a supported LCD panel. This is the simplest ESP-IDF
display bring-up example in the repository.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4-NANO-WIFI6-DB development board.
- A supported MIPI-DSI LCD panel configured by the Nano BSP.

This example uses the local `esp32_p4_nano_wifi6_db` BSP selected by
`main/idf_component.yml`. It is a good first display test for the Nano board
because the application does not create an LVGL UI; it only initializes the
configured panel and asks the DPI panel driver to generate a hardware vertical
color-bar pattern.

## Build and Flash

```bash
cd examples/esp_idf/06_Displaycolorbar
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Expected Behavior

The serial log should show:

```text
Initialize LCD device
Show color bar pattern drawn by hardware
```

The LCD should turn on its backlight and show vertical color bars.

## Troubleshooting

- Confirm the display panel model and interface.
- Confirm PSRAM is enabled when the panel path needs it.
- Check backlight control if logs look correct but the screen is dark.
- Verify reset, power, and panel initialization settings.
- Run this example before [07_lvgl_demo_v9](../07_lvgl_demo_v9/) or
  [10_video_lcd_display](../10_video_lcd_display/) so panel bring-up is tested
  separately from LVGL and camera processing.
