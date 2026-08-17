# Arduino-ESP32 本地板级文件说明

本目录提供 `ESP32-P4-NANO-WIFI6-DB` 在 Arduino IDE 中的板卡注册和 variant 文件，
不是对 Arduino-ESP32 官方核心源码的修改。

## 版本和板卡默认值

- Arduino-ESP32：`3.3.11`
- Arduino-ESP32 内置 ESP32-P4 预编译包：`tools/esp32-arduino-libs/esp32p4`
- 板卡条目：`Waveshare ESP32-P4-NANO-WIFI6-DB`
- 编译目标：`esp32p4`
- Flash：`16MB`
- PSRAM：由板卡条目自动定义 `BOARD_HAS_PSRAM`
- 默认分区：`default_16MB`（每个 OTA APP 6.25MB、3.43MB SPIFFS）

`boards.txt` 是基于 Arduino-ESP32 `3.3.11` 的完整文件，不是只包含一个板卡的
小型片段。安装时请先备份 Arduino-ESP32 的原文件：

1. 若安装目录没有本地板卡修改，可用本目录的 `boards.txt` 替换同版本文件。
2. 若安装目录已经包含其他板卡或本地修改，只合并以
   `waveshare_esp32_p4_nano_wifi6_db.` 开头的板卡条目，避免覆盖其他定义。
   本仓库当前不再注册 `waveshare_esp32_p4_wifi6_db`；如果安装目录已有该旧条目，
   可以为其他硬件保留，但 Nano 必须使用新的 `waveshare_esp32_p4_nano_wifi6_db` 条目。
3. 将 `variants/waveshare_esp32_p4_nano_wifi6_db` 整个目录复制到同版本 Arduino-ESP32
   的 `variants` 目录。
4. 重启 Arduino IDE，选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。

建议使用板卡菜单的 `UART0 / Hardware CDC` 上传模式；上传完成后使用 `115200`
波特率打开串口监视器。使用 USB CDC、其他分区或其他上传速度时，应确认对应硬件
连接和示例的内存需求。

## 板级差异

`pins_arduino.h` 按 `firmware/esp32_p4_nano_wifi6_db` BSP、活动的
ESP-Hosted 配置和该工程原理图适配：

- I2C1：SDA=`GPIO7`，SCL=`GPIO8`，默认 400 kHz；普通 I2C/音频示例使用 `Wire1`。
- ES8311 I2S：MCLK/BCLK/WS/DOUT/DIN=`GPIO13/12/10/9/11`，功放使能=`GPIO53`；板载为单麦克风、单扬声器。
- SDMMC：CLK/CMD/D0~D3=`GPIO43/44/39/40/41/42`，卡电源使能=`GPIO45`（低有效）。
- ESP-Hosted SDIO：CLK/CMD/D0~D3/复位=`GPIO18/19/14/15/16/17/54`。
- MIPI-DSI LCD 没有 LCD 复位 GPIO；背光控制器位于共享 I2C 总线 `0x45` 的寄存器 `0x96`。
- GT911 复位和中断脚均未连接，不能把 7C 的 GPIO23、GPIO30、GPIO31 映射带到本板。

当前 Nano BSP 默认的 LCD 配置是 10.1 英寸 JD9365（800×1280）；本板也支持
8 英寸 JD9365、7 英寸 ILI9881C 和 5 英寸 HX8394。Arduino 示例中的 LCD
分辨率、面板控制器和 DSI 速率必须与实际连接的面板一致。

## 与示例的关系

`examples/Arduino` 中的通用 GPIO、I2C、SDMMC 和板卡信息示例使用本 variant。
`mipi_dsi`、`squareline_wifi_clock` 和 `mipi_csi` 仍需按实际 LCD 面板选择对应的
控制器参数；其中 `mipi_csi` 当前默认是 ILI9881C，不应继续使用 7C 的
EK79007/1024×600/GPIO30/31 配置。

I2C 初始化方式也按示例区分：`mipi_dsi` 和 SquareLine 使用 legacy I2C API，
`mipi_csi` 使用 `Wire1` 建立新版总线后共享 `i2c_master` 句柄；不要把一个示例的
I2C 初始化代码复制到另一个示例中。
