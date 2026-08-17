# ESP32-P4-NANO-WIFI6-DB

Waveshare `ESP32-P4-NANO-WIFI6-DB` 的 ESP-IDF BSP、Arduino 板卡文件和示例工程。
主控是 ESP32-P4，板载 ESP32-C5 作为无线协处理器，通过 ESP-Hosted/SDIO 与 P4
连接；Arduino 和 ESP-IDF 应用的编译目标都是 `esp32p4`。

## 当前配置

- Flash：16MB
- PSRAM：已由 Arduino 板卡条目启用
- Arduino-ESP32：`3.3.11`
- Arduino-ESP32 内置 ESP32-P4 库：基于 ESP-IDF `v5.5.5`
- ESP-IDF BSP：要求 IDF `>=5.5`，当前组件版本 `0.0.1`
- Arduino 默认分区：`default_16MB`
- 默认串口波特率：`115200`

## 主要硬件资源

| 功能 | 连接与说明 |
| --- | --- |
| I2C1 | SDA `GPIO7`，SCL `GPIO8`，板载 ES8311、GT911、背光和摄像头 SCCB 共用 |
| ES8311 I2S | MCLK/BCLK/WS/DOUT/DIN：`GPIO13/12/10/9/11` |
| 功放使能 | `GPIO53`，高电平打开 |
| SDMMC 4-bit | CLK/CMD：`GPIO43/44`；D0-D3：`GPIO39/40/41/42` |
| SD 卡电源 | `GPIO45`，低电平打开；SDMMC 使用片上 LDO 通道 4 |
| ESP-Hosted SDIO | CLK/CMD/D0-D3/RESET：`GPIO18/19/14/15/16/17/54` |
| LCD 背光 | I2C 地址 `0x45`，亮度寄存器 `0x96`，无独立背光 GPIO |
| LCD 复位 | 未连接；GT911 复位和中断 GPIO 也未连接 |

### MIPI-DSI 面板

Nano BSP 默认使用 10.1 英寸 JD9365。支持的 BSP 面板如下，全部使用两条
DSI data lane：

| 面板 | 控制器 | 分辨率 | Lane 速率 |
| --- | --- | ---: | ---: |
| Waveshare 5-DSI-TOUCH-A | HX8394 | 720×1280 | 700 Mbps |
| Waveshare 7-DSI-TOUCH-A | ILI9881C | 720×1280 | 1000 Mbps |
| Waveshare 8-DSI-TOUCH-A | JD9365 | 800×1280 | 1500 Mbps |
| Waveshare 10.1-DSI-TOUCH-A | JD9365 | 800×1280 | 1500 Mbps |

Arduino 的 `mipi_dsi` 和 `squareline_wifi_clock` 示例默认使用 8 英寸 JD9365；
切换面板时，控制器、分辨率、初始化命令和 DSI 速率必须使用同一套 profile。

## 目录结构

```text
firmware/esp32_p4_nano_wifi6_db/  ESP-IDF BSP 组件
examples/Arduino/                 Arduino 示例、boards.txt 和 Nano variant
examples/esp_idf/                 ESP-IDF 外设和功能示例
```

BSP 的接口、引脚和配置说明见
[`firmware/esp32_p4_nano_wifi6_db/README.md`](./firmware/esp32_p4_nano_wifi6_db/README.md)
以及 [`API.md`](./firmware/esp32_p4_nano_wifi6_db/API.md)。

## Arduino 快速开始

1. 安装 Arduino-ESP32 `3.3.11`。
2. 将
   `examples/Arduino/esp32/variants/waveshare_esp32_p4_nano_wifi6_db`
   复制到 Arduino-ESP32 的 `variants` 目录。
3. 将 `examples/Arduino/esp32/boards.txt` 中以
   `waveshare_esp32_p4_nano_wifi6_db.` 开头的板卡条目合并到同版本的
   `boards.txt`。该文件是完整快照；如果本地已经有其他板卡定义，不要直接覆盖。
4. 重启 Arduino IDE，选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
5. 使用默认 `16M Flash` 分区方案和正确的串口。

详细安装说明和示例索引见
[`examples/Arduino/README.md`](./examples/Arduino/README.md)。可先运行
`board_check`，再按需测试 GPIO、I2C、I2S、SDMMC、MIPI-DSI、MIPI-CSI 或
SquareLine UI 示例。

## ESP-IDF 快速开始

在 ESP-IDF 终端中进入一个示例目录，以最小的板卡检查示例为例：

```bash
cd examples/esp_idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为实际串口，例如 Windows 下的 `COM7`。推荐的验证顺序是：

1. `00_board_check`：确认 P4、Flash、PSRAM 和串口链路。
2. `01_i2c_tools`：确认 GPIO7/GPIO8 和 I2C 总线。
3. `02_sdmmc`：确认 microSD 和 4-bit SDMMC。
4. `06_Displaycolorbar`：在 LVGL 或摄像头示例之前确认 LCD、DSI 时序和背光。
5. `03_wifistation`：确认 ESP-Hosted、C5 无线协处理器、关联和 DHCP。

更多 ESP-IDF 示例位于 [`examples/esp_idf`](./examples/esp_idf)。

## 使用注意

- I2C1 是共享总线。Arduino 示例分别使用 legacy I2C 或新版 `i2c_master`，不要
  同时运行多个会重复初始化 I2C1 的示例。
- GPIO45 和 GPIO53 分别由 SD 卡电源和音频功放使用；对应外设工作时不要将它们
  当作普通 GPIO 驱动。
- 摄像头、LCD、SD 卡和无线协处理器涉及供电、线缆和外部硬件条件。源码或编译
  通过不等于已经完成硬件验证。
- 不要把真实 Wi-Fi 密码、API key 或带凭据的日志提交到仓库或公开分享。
