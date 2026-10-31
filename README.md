<div align="center">
  <h1>ESP32-P4-NANO-WIFI6-DB</h1>
  <p><strong>ESP32-P4 development board with an ESP32-C5 Wi-Fi 6 co-processor and MIPI-DSI / MIPI-CSI interfaces</strong></p>
  <p>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-NANO-WIFI6-DB"></a>
  </p>
  <p>
    <a href="README_CN.md">中文</a> ·
    <a href="https://www.waveshare.net/shop/ESP32-P4-NANO-WIFI6-DB.htm">🌐 Product Page</a> ·
    <a href="https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/">📚 Documentation</a> ·
    <a href="firmware/">📦 Firmware</a> ·
    <a href="examples/esp_idf/">🧩 ESP-IDF Examples</a> ·
    <a href="examples/Arduino/">🔧 Arduino Examples</a>
  </p>
  <img src="https://www.waveshare.net/photo/development-board/ESP32-P4-NANO-WIFI6-DB/ESP32-P4-NANO-WIFI6-DB-details-1.jpg" alt="Waveshare ESP32-P4-NANO-WIFI6-DB" width="720">
</div>

---

## ✨ Overview

This repository provides the ESP-IDF BSP, ESP-IDF and Arduino examples, ESP-Brookesia firmware source, factory firmware, and schematic for the Waveshare ESP32-P4-NANO-WIFI6-DB development board.

The board uses ESP32-P4 as its main processor and connects an ESP32-C5 wireless co-processor through ESP-Hosted over SDIO. It provides MIPI-DSI display, MIPI-CSI camera, ES8311 audio, microSD, Ethernet, USB Host, and GPIO expansion interfaces. Displays and cameras are external modules; see the [product page](https://www.waveshare.net/shop/ESP32-P4-NANO-WIFI6-DB.htm) for purchasing information.

## 🖥️ Hardware Overview

| Feature | Device / Interface |
| --- | --- |
| Main processor | ESP32-P4NRW32X, dual-core HP RISC-V up to 360 MHz plus a low-power core |
| Wireless | ESP32-C5 co-processor; Wi-Fi 6 and Bluetooth through ESP-Hosted over SDIO |
| Memory | 16 MB Flash; PSRAM enabled by the Arduino board definition |
| Display | Dual-lane MIPI-DSI; supports external Waveshare 5-, 7-, 8-, and 10.1-inch panels |
| Touch | GT911 capacitive touch controller, polled over the shared I2C bus |
| Camera | MIPI-CSI; the camera example is configured for OV5647 |
| Audio | ES8311 codec, one analog microphone input, and speaker output |
| Storage and expansion | microSD (4-bit SDMMC), USB Host, Ethernet, and GPIO expansion |
| Board support | Local ESP-IDF BSP component `esp32_p4_nano_wifi6_db`, version `0.0.1` |
| Schematic | [hardware/schematics/ESP32-P4-NANO-WIFI6-DB.pdf](hardware/schematics/ESP32-P4-NANO-WIFI6-DB.pdf) |

For complete pin assignments, interface details, and usage notes, see the [Hardware Guide](docs/en/hw-reference/user-guide.md).

### ESP-IDF LCD Selection

| Panel | Controller | Resolution |
| --- | --- | --- |
| [Waveshare 5-DSI-TOUCH-A](https://www.waveshare.net/shop/5-DSI-TOUCH-A.htm) | HX8394, 5-inch | 720 × 1280 |
| [Waveshare 7-DSI-TOUCH-A](https://www.waveshare.net/shop/7-DSI-TOUCH-A.htm) | ILI9881C, 7-inch | 720 × 1280 |
| [Waveshare 8-DSI-TOUCH-A](https://www.waveshare.net/shop/8-DSI-TOUCH-A.htm) | JD9365, 8-inch | 800 × 1280 |
| [Waveshare 10.1-DSI-TOUCH-A](https://www.waveshare.net/shop/10.1-DSI-TOUCH-A.htm) | JD9365, 10.1-inch | 800 × 1280 |

The BSP defaults to the **10.1-inch JD9365**. Before building, confirm which panel is connected and use its matching controller, resolution, and initialization commands.

## 🚀 Quick Start

Install ESP-IDF (the BSP manifest requires **>=5.5**) and open a configured terminal. Start by building the board-check example:

```powershell
cd examples\esp_idf\00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with the board's serial port. After the board check passes, choose an example from the tables below. Before building a display example, configure the LCD profile that matches the connected panel. See the [Arduino setup instructions](examples/Arduino/README.md) to install the Arduino board package.

`>=5.5` is the version constraint in the component manifest; it does not mean that every example has been verified with all newer versions. Dependencies may also impose additional requirements.

## 🧪 ESP-IDF Examples

| Example | Purpose |
| --- | --- |
| [00_board_check](examples/esp_idf/00_board_check) | Print chip, Flash, PSRAM, and serial information |
| [01_i2c_tools](examples/esp_idf/01_i2c_tools) | Scan and interactively diagnose the shared I2C bus |
| [02_sdmmc](examples/esp_idf/02_sdmmc) | 4-bit SDMMC read/write test for microSD |
| [03_wifistation](examples/esp_idf/03_wifistation) | Connect to Wi-Fi through ESP32-C5/ESP-Hosted |
| [04_ethernetbasic](examples/esp_idf/04_ethernetbasic) | Ethernet link and IP initialization |
| [05_I2SCodec](examples/esp_idf/05_I2SCodec) | ES8311 audio playback, capture, and loopback |
| [06_Displaycolorbar](examples/esp_idf/06_Displaycolorbar) | MIPI-DSI LCD color-bar and backlight test |
| [07_lvgl_demo_v9](examples/esp_idf/07_lvgl_demo_v9) | LVGL 9 graphical interface demo |
| [08_eth2ap](examples/esp_idf/08_eth2ap) | Ethernet-to-Wi-Fi access-point example |
| [09_simple_video_server](examples/esp_idf/09_simple_video_server) | Camera streaming, image capture, and Web control |
| [10_video_lcd_display](examples/esp_idf/10_video_lcd_display) | Camera video output to an LCD |
| [11_usb_extend_screen](examples/esp_idf/11_usb_extend_screen) | USB extended-screen example |
| [12_nvs_counter](examples/esp_idf/12_nvs_counter) | Save a boot counter using NVS |

See each example's README and configuration for wiring and operation instructions.

## 🧪 Arduino Examples

The local board package targets Arduino-ESP32 **3.3.11**. Follow the [Arduino setup instructions](examples/Arduino/README.md) to install the variant and board entries, then select `Waveshare ESP32-P4-NANO-WIFI6-DB`. The board defaults to 16 MB Flash and the `default_16MB` partition scheme; see each example's README for its configuration.

| Example | Purpose |
| --- | --- |
| [board_check](examples/Arduino/board_check) | Chip and memory information |
| [gpio](examples/Arduino/gpio) | Interactive GPIO test over serial commands |
| [i2c](examples/Arduino/i2c) | Scan I2C1 addresses |
| [i2s](examples/Arduino/i2s) | ES8311 single-microphone to single-speaker audio loopback |
| [sdmmc](examples/Arduino/sdmmc) | 4-bit microSD file read/write test |
| [mipi_dsi](examples/Arduino/mipi_dsi) | LCD color-bar and backlight test |
| [mipi_csi](examples/Arduino/mipi_csi) | OV5647 camera preview on an LCD |

Arduino `mipi_dsi` and `mipi_csi` default to the 10.1-inch JD9365 profile at 800 × 1280, RGB565, with two DSI lanes. For another LCD, select the matching JD9365, HX8394, or ILI9881C profile in the example configuration. I2C1 is shared by multiple peripherals; do not initialize the bus more than once in the same application.

## 📡 ESP32-P4 and ESP32-C5 Wireless

ESP32-P4 does not include an RF transceiver. Wi-Fi and Bluetooth are provided by the ESP32-C5 co-processor through ESP-Hosted over SDIO. The ESP32-C5 requires compatible slave firmware; flashing an ESP32-P4 example does not update the C5. Before changing or reflashing the co-processor, confirm that its firmware is compatible with the ESP-Hosted and remote Wi-Fi component versions used by the P4 project.

## 📦 Firmware

[`firmware/brookesia/`](firmware/brookesia/) contains the ESP-Brookesia application firmware source, including the GUI, settings, music player, video player, camera, and other application components.

[`firmware/bin/`](firmware/bin/) provides precompiled firmware for the ESP32-P4 main processor and ESP32-C5 wireless co-processor. They target different chips and use different flash addresses; check the corresponding instructions and firmware versions before flashing. The presence of a firmware file in the repository does not guarantee compatibility with newly resolved dependency versions.

## 🗂️ Repository Layout

| Path | Contents |
| --- | --- |
| [`components/esp32_p4_nano_wifi6_db/`](components/esp32_p4_nano_wifi6_db/) | ESP-IDF BSP, headers, Kconfig, and component manifest |
| [`examples/esp_idf/`](examples/esp_idf/) | ESP-IDF peripheral and feature examples |
| [`examples/Arduino/`](examples/Arduino/) | Arduino examples and local board definitions |
| [`firmware/brookesia/`](firmware/brookesia/) | ESP-Brookesia application firmware source |
| [`firmware/bin/`](firmware/bin/) | Precompiled ESP32-P4 and ESP32-C5 firmware |
| [`hardware/schematics/`](hardware/schematics/) | Board schematic |
| [`docs/`](docs/) | Hardware reference and project documentation |
| [`assert/`](assert/) | Product image used in the documentation |

## 📚 Documentation

- [Product Page](https://www.waveshare.net/shop/ESP32-P4-NANO-WIFI6-DB.htm)
- [Official Wiki](https://docs.waveshare.net/ESP32-P4-NANO-WIFI6-DB/)
- [Hardware Guide](docs/en/hw-reference/user-guide.md)
- [BSP README](components/esp32_p4_nano_wifi6_db/README.md)
- [BSP API Reference](components/esp32_p4_nano_wifi6_db/API.md)
- [Arduino Examples](examples/Arduino/README.md)
- [Brookesia Firmware](firmware/brookesia/)
- [Schematic](hardware/schematics/ESP32-P4-NANO-WIFI6-DB.pdf)

## 🤝 Support and Contributions

Contributions and reproducible issue reports are welcome. Include the board version, connected panel, example path, ESP-IDF or Arduino-ESP32 version, reproduction steps, expected and actual behavior, and relevant serial logs. Do not include logs containing Wi-Fi passwords or API keys.

- [Open an Issue](https://github.com/waveshareteam/ESP32-P4-NANO-WIFI6-DB/issues/new)

## 📄 License

This repository is licensed under the Apache License 2.0; see [LICENSE](LICENSE). Some components and example files carry their own license or copyright notices; follow those notices when using or redistributing the files.
