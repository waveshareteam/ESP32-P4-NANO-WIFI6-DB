## USB Extended Display Example

Build this repository-local adaptation from the example directory. It uses the Nano BSP override declared in `main/idf_component.yml`.

The USB extended display example allows the ESP32-P4-NANO-WIFI6-DB board to function as a secondary display for Windows. It supports the following features:

* Supports a maximum landscape USB input of **1024×576@60FPS**.
* Rotates the input into the Nano's native portrait MIPI panel and centers it in the available area.
* Keeps unused panel area black; for the default 800×1280 panel, the active content is 576×1024.
* Supports up to **five-point touch input**.
* Supports **audio input and output**.

## Required Hardware

### ESP32-P4-NANO-WIFI6-DB

1. ESP32-P4-NANO-WIFI6-DB development board.
2. A supported Waveshare MIPI-DSI touch display configured by the Nano BSP.
3. A speaker if USB audio output is enabled.

## Hardware Connection

1. Connect the high-speed USB port on the development board to the PC.

## Compilation and Flashing

### Device Side

Build the project, flash it to the board, and run the monitor tool to check the serial output:

1. Run `. ./export.sh` to set up the IDF environment.
2. Run `idf.py set-target esp32p4` to set the target chip.
3. If you encounter any errors in the previous step, run `pip install "idf-component-manager~=1.1.4"` to upgrade the component manager.
4. Run `idf.py -p PORT flash monitor` to build, flash, and monitor the project.

(To exit the serial monitor, press `Ctrl-]`.)

Refer to the **Getting Started Guide** for full instructions on configuring and using ESP-IDF to build projects.

### PC Side

For preparation, refer to [windows_driver](./windows_driver/README_cn.md).

![Demo](https://dl.espressif.com/AE/esp-iot-solution/p4_usb_extern_screen.gif)

## Troubleshooting

### The touchscreen controls the wrong display

1. Open **Control Panel** and select **Tablet PC Settings**.
2. Under the **Display** section, select **Setup**.
3. Follow the on-screen instructions to choose the correct extended display.

### Adjusting JPEG Image Quality

* Modify `CONFIG_USB_EXYEEND_SCREEN_JEPG_QUALITY`. The symbol keeps the existing spelling for compatibility.

### Changing the Secondary Screen Resolution

* Keep `CONFIG_USB_EXTEND_SCREEN_HEIGHT=1024` and `CONFIG_USB_EXTEND_SCREEN_WIDTH=576` for the maximum landscape input.
* The Nano panel itself is portrait-oriented. The firmware rotates the image in software, centers it, and leaves the remaining area black.

### Adjusting Image Output Frame Rate

* Modify `CONFIG_USB_EXTEND_SCREEN_MAX_FPS`. Lowering this value effectively reduces USB bandwidth usage. If USB audio stuttering occurs, consider decreasing this value.

### Modifying the Maximum Frame Size

* Modify `CONFIG_USB_EXTEND_SCREEN_FRAME_LIMIT_B` to limit the maximum image size received from the PC driver.

### Disabling Touchscreen Functionality

* Set `CONFIG_HID_TOUCH_ENABLE` to `n`.

### Disabling Audio Functionality

* Set `CONFIG_UAC_AUDIO_ENABLE` to `n`.

**Note:** If only the secondary screen function is enabled, change the PID to `0x2987`.
