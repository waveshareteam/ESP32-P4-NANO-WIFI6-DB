# ESP32-P4-NANO-WIFI6-DB

A development board based on ESP32-P4 and ESP32-C5 for multimedia, networking, and edge applications.

English · [中文](./README_CN.md) · [🌐 Product Page](https://www.waveshare.net/shop/ESP32-P4-NANO-WIFI6-DB.htm) · [📚 Official Documentation](https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/) · [🧩 ESP-IDF Examples](./examples/esp_idf/) · [🔧 Arduino Examples](./examples/Arduino/) · [📦 Firmware](./firmware/)

---

![ESP32-P4-NANO-WIFI6-DB](./assert/ESP32-P4-NANO-WIFI6-DB.jpg)

Waveshare ESP32-P4-NANO-WIFI6-DB is a development board for multimedia, networking, and edge applications. This repository provides the matching ESP-IDF BSP, Arduino board files, peripheral examples, Brookesia firmware source, and precompiled firmware.

The board uses ESP32-P4 as the main processor and connects an ESP32-C5 wireless co-processor through ESP-Hosted/SDIO. It provides interfaces for external MIPI-DSI displays, MIPI-CSI cameras, audio, microSD, Ethernet, USB Host, and GPIO expansion. The display and camera are external modules; always follow the hardware configuration and the README of the selected example.

## ✨ Overview

This repository includes:

- A local ESP-IDF BSP component targeting **esp32p4**.
- Board definitions and examples based on Arduino-ESP32 **3.3.11**.
- ESP-IDF examples for I2C, I2S audio, SDMMC, MIPI-DSI, MIPI-CSI, Wi-Fi, Ethernet, USB, and NVS.
- Brookesia application firmware source under **firmware/brookesia**.
- Precompiled firmware for the P4 main processor and the C5 wireless co-processor.
- The board schematic and BSP API documentation.

Useful entry points:

| Resource | Description |
| --- | --- |
| [ESP-IDF BSP](./components/esp32_p4_nano_wifi6_db/) | Board initialization, display, touch, audio, storage, camera, and USB Host support |
| [BSP Guide](./components/esp32_p4_nano_wifi6_db/README.md) | Pin assignments, panel profiles, and basic usage |
| [BSP API](./components/esp32_p4_nano_wifi6_db/API.md) | BSP macros and public functions |
| [ESP-IDF Examples](./examples/esp_idf/) | ESP-IDF peripheral and feature examples |
| [Arduino Examples](./examples/Arduino/) | Arduino projects, board files, and examples |
| [Hardware Schematic](./hardware/schematics/ESP32-P4-NANO-WIFI6-DB.pdf) | Development-board schematic |
| [Official Documentation](https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/) | Waveshare hardware and software documentation |

## 🖥️ Hardware Overview

| Feature | Device / Interface |
| --- | --- |
| Main processor | ESP32-P4; the ESP-IDF and Arduino target is **esp32p4** |
| Wireless | ESP32-C5 wireless co-processor connected to ESP32-P4 through ESP-Hosted/SDIO |
| Memory | 16 MB Flash; PSRAM is enabled by the Arduino board definition |
| Display | External MIPI-DSI LCD with two DSI data lanes |
| Touch | GT911 capacitive touch controller on the shared I2C bus, using polling |
| Camera | MIPI-CSI interface; the Arduino camera example uses OV5647 |
| Audio | ES8311 codec, one analog microphone input, and speaker output |
| Storage expansion | microSD through 4-bit SDMMC |
| Network | ESP32-C5 Wi-Fi co-processor and onboard Ethernet interface |
| USB | USB Host |
| Expansion | GPIO, I2C, UART, and CAN/TWAI expansion interfaces |

### Software Versions and Configuration

| Item | Configuration |
| --- | --- |
| ESP-IDF BSP | Component version **0.0.1**; the manifest requires ESP-IDF **>=5.5** |
| Arduino-ESP32 | **3.3.11** |
| Arduino board | **Waveshare ESP32-P4-NANO-WIFI6-DB** |
| Arduino Flash | 16 MB; default partition scheme **default_16MB** |
| Arduino serial monitor | **115200** by default |

### Key Hardware Resources

| Function | Pins / Parameters | Notes |
| --- | --- | --- |
| Shared I2C | SDA **GPIO7**, SCL **GPIO8** | Shared by ES8311, GT911, LCD backlight, and camera SCCB |
| ES8311 I2S | MCLK/BCLK/WS/DOUT/DIN: **GPIO13/12/10/9/11** | Audio input and output |
| Amplifier enable | **GPIO53**, active high | Audio power-amplifier enable |
| 4-bit SDMMC | CLK/CMD: **GPIO43/44**; D0-D3: **GPIO39/40/41/42** | SD-card power control is **GPIO45**, active low |
| ESP-Hosted SDIO | CLK/CMD/D0-D3/RESET: **GPIO18/19/14/15/16/17/54** | Connected to the ESP32-C5 co-processor |
| LCD backlight | I2C address **0x45**, register **0x96** | No dedicated backlight GPIO |
| LCD reset | Not connected | Handled by the panel initialization flow |
| GT911 reset / interrupt | Not connected | Touch data is polled over I2C |

GPIO45 and GPIO53 are also used by the BSP for SD-card power and audio-amplifier control. Do not drive them as general-purpose GPIOs while the corresponding peripherals are active.

### MIPI-DSI Panels

The Nano BSP supports the following external Waveshare DSI touch panels. The default profile is the 10.1-inch JD9365. When switching panels, keep the controller, resolution, initialization commands, and DSI rate from the same profile.

| Panel option | Controller | Resolution | DSI lane rate |
| --- | --- | ---: | ---: |
| Waveshare 5-DSI-TOUCH-A | HX8394 | 720 × 1280 | 700 Mbps |
| Waveshare 7-DSI-TOUCH-A | ILI9881C | 720 × 1280 | 1000 Mbps |
| Waveshare 8-DSI-TOUCH-A | JD9365 | 800 × 1280 | 1500 Mbps |
| Waveshare 10.1-DSI-TOUCH-A | JD9365 | 800 × 1280 | 1500 Mbps |

## 🚀 Quick Start

### ESP-IDF

Install an ESP-IDF environment that satisfies the BSP requirement, then open an ESP-IDF terminal and enter an example directory. Start with the board-check example, which does not require an external display, camera, SD card, network, or audio codec:

~~~bash
cd examples/esp_idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
~~~

Replace **PORT** with the serial port of the board, such as **COM7** on Windows. The recommended bring-up order is:

1. **00_board_check**: verify the chip, Flash, PSRAM, and serial link.
2. **01_i2c_tools**: verify the shared I2C bus on GPIO7/GPIO8.
3. **02_sdmmc**: verify the microSD card and 4-bit SDMMC.
4. **06_Displaycolorbar**: verify the actual LCD panel, MIPI-DSI timing, and backlight.
5. **05_I2SCodec**: verify ES8311 audio input and output.
6. **03_wifistation**: verify ESP-Hosted, the ESP32-C5 co-processor, Wi-Fi association, and DHCP.

### Arduino

The Arduino examples are based on Arduino-ESP32 **3.3.11** and target ESP32-P4. The ESP32-C5 is managed as a wireless co-processor by ESP-Hosted; it is not the compilation target of the Arduino project.

1. Install Arduino-ESP32 **3.3.11**.
2. Copy the directory **examples/Arduino/esp32/variants/waveshare_esp32_p4_nano_wifi6_db** to the **variants** directory of the matching Arduino-ESP32 installation.
3. **examples/Arduino/esp32/boards.txt** is a complete boards.txt snapshot. Back up the original file first, then merge the entries beginning with **waveshare_esp32_p4_nano_wifi6_db.** instead of overwriting other board definitions.
4. Restart Arduino IDE and select **Waveshare ESP32-P4-NANO-WIFI6-DB**.
5. Select 16 MB Flash, the **default_16MB** partition scheme, PSRAM enabled, and the correct serial port.
6. Select the panel profile that matches the connected LCD for display examples.

## 🧪 Examples

### ESP-IDF

All examples are under [examples/esp_idf](./examples/esp_idf/). Each example has its own project configuration and README; some examples provide both English and Chinese documentation.

| Example | Main function |
| --- | --- |
| [00_board_check](./examples/esp_idf/00_board_check/) | First-run check of the chip, Flash, PSRAM, and serial output |
| [01_i2c_tools](./examples/esp_idf/01_i2c_tools/) | I2C bus scanning and interactive diagnostics |
| [02_sdmmc](./examples/esp_idf/02_sdmmc/) | 4-bit SDMMC read/write test for microSD |
| [03_wifistation](./examples/esp_idf/03_wifistation/) | Wi-Fi connection through the ESP32-C5 and ESP-Hosted |
| [04_ethernetbasic](./examples/esp_idf/04_ethernetbasic/) | Ethernet link and IP initialization |
| [05_I2SCodec](./examples/esp_idf/05_I2SCodec/) | ES8311 audio playback, capture, and loopback |
| [06_Displaycolorbar](./examples/esp_idf/06_Displaycolorbar/) | MIPI-DSI LCD color-bar and backlight test |
| [07_lvgl_demo_v9](./examples/esp_idf/07_lvgl_demo_v9/) | LVGL 9 graphical-interface demo |
| [08_eth2ap](./examples/esp_idf/08_eth2ap/) | Ethernet-to-Wi-Fi access-point example |
| [09_simple_video_server](./examples/esp_idf/09_simple_video_server/) | Camera streaming, image capture, and Web control |
| [10_video_lcd_display](./examples/esp_idf/10_video_lcd_display/) | Camera video output to an LCD |
| [11_usb_extend_screen](./examples/esp_idf/11_usb_extend_screen/) | USB extended-screen example |
| [12_nvs_counter](./examples/esp_idf/12_nvs_counter/) | Boot-counter persistence with NVS |

### Arduino

| Example | Main function |
| --- | --- |
| [board_check](./examples/Arduino/board_check/) | Print chip, Flash, PSRAM, and heap information |
| [gpio](./examples/Arduino/gpio/) | Interactive GPIO test through serial commands |
| [i2c](./examples/Arduino/i2c/) | Scan addresses on the shared I2C1 bus |
| [i2s](./examples/Arduino/i2s/) | ES8311 single-microphone to single-speaker audio loopback |
| [sdmmc](./examples/Arduino/sdmmc/) | 4-bit microSD file read/write test |
| [mipi_dsi](./examples/Arduino/mipi_dsi/) | MIPI-DSI LCD color-bar and backlight test |
| [mipi_csi](./examples/Arduino/mipi_csi/) | OV5647 camera live display on an MIPI-DSI LCD |

mipi_dsi and mipi_csi default to the 10.1-inch JD9365 profile: 800 × 1280, RGB565, and two DSI lanes. For another LCD, select the matching JD9365, HX8394, or ILI9881C profile instead of reusing the default configuration.

I2C1 is a shared bus. Different examples may use Arduino Wire1, the legacy I2C API, or the newer i2c_master API. Do not initialize the same bus more than once in one application; see the README of the selected example for details.

## 📦 Firmware

### Brookesia Firmware Source

[firmware/brookesia](./firmware/brookesia/) contains the ESP-Brookesia application firmware source, including the GUI, settings, music player, video player, camera, and other application components.

### Precompiled Firmware

| File | Purpose |
| --- | --- |
| [ESP32-P4-NANO-WIFI6-DB-FactoryOnly-260623.bin](./firmware/bin/ESP32-P4-NANO-WIFI6-DB-FactoryOnly-260623.bin) | Factory firmware for the ESP32-P4 main processor |
| [esp32-c5-slave-260623.bin](./firmware/bin/ESP32-C5/esp32-c5-slave-260623.bin) | Slave firmware for the ESP32-C5 wireless co-processor |

The P4 and C5 firmware target different chips. Confirm the target chip, flash address, and firmware version before flashing. When rebuilding or replacing the C5 firmware, also confirm compatibility with the ESP-Hosted and remote Wi-Fi component versions used by the P4 project.

## 🗂️ Repository Layout

| Path | Purpose |
| --- | --- |
| [assert/](./assert/) | Product image used by the README |
| [examples/esp_idf/](./examples/esp_idf/) | ESP-IDF peripheral and feature examples |
| [examples/Arduino/](./examples/Arduino/) | Arduino examples, board files, and variant |
| [components/esp32_p4_nano_wifi6_db/](./components/esp32_p4_nano_wifi6_db/) | Local Nano BSP component |
| [firmware/brookesia/](./firmware/brookesia/) | Brookesia application firmware source |
| [firmware/bin/](./firmware/bin/) | Precompiled P4 and C5 firmware |
| [hardware/schematics/](./hardware/schematics/) | Board schematic |

Generated files such as **build/**, **managed_components/**, and local **sdkconfig** files are not source files and should not be committed.

## 📚 Documentation

- [Documentation Index](./docs/README.md)
- [Hardware Guide](./docs/en/hw-reference/user-guide-esp32-p4-nano-wifi6-db.md)
- [BSP Guide](./components/esp32_p4_nano_wifi6_db/README.md)
- [BSP API](./components/esp32_p4_nano_wifi6_db/API.md)
- [Arduino Board Files](./examples/Arduino/esp32/README.md)
- [Arduino Examples](./examples/Arduino/README.md)
- [ESP-IDF Board Check README](./examples/esp_idf/00_board_check/README.md)
- [Arduino MIPI-CSI README](./examples/Arduino/mipi_csi/README.md)
- [Board Schematic](./hardware/schematics/ESP32-P4-NANO-WIFI6-DB.pdf)

## ⚠️ Usage Notes

- Display, touch, and camera functions depend on the external module, power supply, cables, and matching panel profile. A source or build result is not hardware validation.
- I2C1 is shared by ES8311, GT911, the LCD backlight, and camera SCCB. For camera examples, also verify camera power and SCCB wiring.
- GPIO45, GPIO53, SDIO, and MIPI pins have board-level multiplexing. Check the pin table and schematic before using expansion headers.
- ESP32-P4 Wi-Fi depends on the ESP32-C5 slave firmware and the ESP-Hosted/SDIO link. Check compatibility when changing software on either side.
- Do not commit real Wi-Fi passwords, API keys, or serial logs containing credentials.

## 📄 License

This repository is licensed under the Apache License 2.0; see [LICENSE](./LICENSE). Some components and examples carry their own license or copyright notices, which apply to those files and must be followed when using or redistributing them.
