# 开发板检查

[English Version](./README.md)

这是本仓库推荐的第一个 ESP-IDF 示例。

它不需要外接模块、显示屏、摄像头、SD 卡、网络或音频 codec。示例只会通过串口监视器打印开发板和运行时信息，然后持续运行，方便初学者确认工具链、烧录流程和串口监视器都工作正常。

该示例会在串口输出中报告检测到的 ESP32-P4 芯片版本。后续请根据该信息选择适合开发板的目标配置。

## 你将学到什么

- 如何构建和烧录 ESP-IDF 工程。
- 如何确认目标芯片是 `esp32p4`。
- 如何从串口输出读取芯片、Flash、PSRAM 和堆信息。
- 如何在进入外设示例前确认芯片版本系列。
- 如何保持串口监视器打开并观察周期日志。

## 硬件要求

- 一块受支持的 ESP32-P4 开发板。
- 用于烧录和串口监视器的 USB 线。

不需要其他硬件。

## 构建和烧录

```bash
cd examples/esp_idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为你的串口，例如 Windows 上的 `COM7` 或 Linux 上的 `/dev/ttyACM0`。

## 预期输出

串口监视器应显示类似输出：

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

示例每五秒打印一次 `alive` 日志。如果能看到该日志，说明你的构建、烧录和监视流程已经可以用于其他示例。

## 下一步

- 运行 [01_i2c_tools](../01_i2c_tools/)，扫描 I2C 总线。
- 如果你的开发板带 SD 卡，运行 [02_sdmmc](../02_sdmmc/) 验证 SD 卡路径。
- 然后根据需要选择 [03_wifistation](../03_wifistation/)、
  [04_ethernetbasic](../04_ethernetbasic/)、[05_I2SCodec](../05_I2SCodec/)、
  [06_Displaycolorbar](../06_Displaycolorbar/)、
  [07_lvgl_demo_v9](../07_lvgl_demo_v9/)、[08_eth2ap](../08_eth2ap/)、
  [09_simple_video_server](../09_simple_video_server/)、
  [10_video_lcd_display](../10_video_lcd_display/)、
  [11_usb_extend_screen](../11_usb_extend_screen/) 或
  [12_bt_controller_mac_addr](../12_bt_controller_mac_addr/)。
