| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

[中文版本](./README_CN.md)

# Video LCD Display

This example is based on the [esp_video](https://github.com/espressif/esp-video-components/tree/master/esp_video) component and demonstrates how to display images from the camera on an LCD screen. The application initializes the ESP32-P4-NANO-WIFI6-DB Nano BSP display, opens the MIPI-CSI video device, uses PPA scale/rotate/mirror processing, acquires a free LCD framebuffer through the BSP ownership API, and submits the complete frame through the LVGL adapter display pipeline.

## ESP-IDF Required

- This example requires ESP-IDF 5.5 or later because the Nano BSP has that minimum requirement.
- The project depends on `esp_video` and the local `esp32_p4_nano_wifi6_db` BSP selected in `main/idf_component.yml`.
- Please follow the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) to set up the development environment. **We highly recommend** you [Build Your First Project](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#build-your-first-project) to get familiar with ESP-IDF and make sure the environment is set up correctly.

### Prerequisites

* An ESP32-P4-NANO-WIFI6-DB development board.
* A supported Waveshare MIPI-DSI panel configured by the Nano BSP. The current project configuration selects the 8-inch 800 x 1280 panel; select another panel in menuconfig when needed.
* A MIPI-CSI camera sensor supported by `esp_video`. The default `sdkconfig.defaults` selects OV5647, MIPI RAW8 `800 x 1280` at 50 FPS.
* A USB-C cable for power supply and programming.
* Connect the LCD to the board's `MIPI_DSI` interface and the camera to its `MIPI_CSI` interface.
* Connect the USB-C cable used for power, programming, and serial output according to the board documentation.

### Configure the Project

Run `idf.py menuconfig` and configure the BSP display, camera sensor, and video pipeline options.

Default ESP32-P4 SCCB/I2C pins for the MIPI-CSI camera:

| Signal | Default GPIO |
| --- | --- |
| SCL | GPIO8 |
| SDA | GPIO7 |

In the `Espressif Camera Sensors` configuration menu, select the camera sensor that matches your hardware. The current defaults are:

```
Component config  --->
    Espressif Camera Sensors Configurations  --->
        [*] OV5647  ---->
            Default format select for MIPI  --->
                (X) RAW8 800x1280 50fps, MIPI input
```

If you use SC2336 or another sensor, update the sensor selection and output format to match the camera module.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output (replace `PORT` with your board's serial port name):

```bash
idf.py set-target esp32p4
idf.py -p PORT flash monitor
```

To exit the serial monitor, type ``Ctrl-]``.

See the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

### Expected Behavior

The display backlight turns on and the LCD shows the live camera image. The log prints the video driver version, device name, bus information, and the detected frame width and height. Camera frames are allocated in PSRAM. Each frame acquires a free LCD framebuffer, renders the complete display-sized output, and submits it with `bsp_display_flush_frame_buffer()`. The LCD uses the triple framebuffer configuration selected by `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3`.

### Display Framebuffer Ownership

The camera-to-LCD path follows this framebuffer ownership sequence:

1. Call `bsp_display_get_free_frame_buffer()` to acquire a writable LCD framebuffer.
2. Render the complete display-sized frame into that buffer with PPA. If the camera image does not cover the whole LCD, clear the target framebuffer before PPA processing so the uncovered area is deterministic.
3. Call `bsp_display_flush_frame_buffer(frame_buffer)` to submit the buffer to the display pipeline. Do not access the buffer again until it is acquired again through `bsp_display_get_free_frame_buffer()`.

This keeps the full LCD frame consistent when the camera image is shorter than the display and prevents residual content or flicker in the uncovered area when triple buffering is enabled. Do not directly index the LCD framebuffers or use `esp_lcd_dpi_panel_get_frame_buffer()` / `esp_lv_adapter_dummy_draw_blit()` in this path.

### Troubleshooting

- Run [06_Displaycolorbar](../06_Displaycolorbar/) first to verify the LCD path.
- Check camera FPC orientation, sensor power, SCCB/I2C pins, and selected sensor model if `video cam open failed` appears.
- Confirm PSRAM is enabled and stable; camera buffers are allocated from PSRAM.
- If the image is cropped, mirrored, or rotated incorrectly, adjust the sensor output format or the PPA operation in `main/main.c`.
