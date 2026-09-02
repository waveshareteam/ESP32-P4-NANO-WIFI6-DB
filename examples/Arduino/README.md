# ESP32-P4-NANO-WIFI6-DB Arduino 示例

这些示例适用于 Waveshare `ESP32-P4-NANO-WIFI6-DB`，并按 Arduino-ESP32 `3.3.11`
完成源代码和板级配置检查。目标芯片是 ESP32-P4；板载 ESP32-C5 无线协处理器由
ESP-Hosted 通过 SDIO 连接，不是 Arduino 的编译目标。

## 使用前

1. 安装 Arduino-ESP32 `3.3.11`，不要把本目录中的板级文件与其他版本混用。
2. 将 `esp32/variants/waveshare_esp32_p4_nano_wifi6_db` 复制到 Arduino-ESP32 的
   `variants` 目录。
3. `esp32/boards.txt` 是基于 Arduino-ESP32 `3.3.11` 的完整 `boards.txt` 快照。
   如果安装目录没有本地修改，可以备份后替换；如果已经添加过其他板卡，建议只将
   `waveshare_esp32_p4_nano_wifi6_db` 条目合并进去，不要覆盖已有板卡定义。
4. 重启 Arduino IDE，选择 `Waveshare ESP32-P4-NANO-WIFI6-DB` 和正确的串口。
5. 板卡条目默认使用 16 MB Flash、启用 PSRAM 和 `default_16MB` 分区方案；除非
   示例 README 另有说明，串口监视器使用 `115200` 波特率。
6. 需要显示、触摸、摄像头或天气功能时，先按照对应示例 README 安装库并选择
   实际连接的 LCD 面板。

## 示例

| 示例 | 作用 | 文档 |
| --- | --- | --- |
| `board_check` | 打印芯片、Flash、PSRAM 和堆内存信息 | [README](./board_check/README.md) |
| `gpio` | 通过串口命令交互式测试 GPIO | [README](./gpio/README.md) |
| `i2c` | 扫描 I2C1 总线上的 7 位地址 | [README](./i2c/README.md) |
| `i2s` | ES8311 单麦克风到单扬声器的 48 kHz 回环 | [README](./i2s/README.md) |
| `sdmmc` | 以 4-bit 模式测试 microSD 文件读写 | [README](./sdmmc/README.md) |
| `mipi_dsi` | MIPI-DSI LCD 色条和背光测试 | [README](./mipi_dsi/README.md) |
| `mipi_csi` | OV5647 摄像头实时显示到 MIPI-DSI LCD | [README](./mipi_csi/README.md) |

## LCD 默认配置

- `mipi_dsi` 和 `mipi_csi` 默认使用 10.1 英寸 JD9365，分辨率为
  800×1280、RGB565、双 DSI lane、1500 Mbps/lane。
- 使用其他 LCD 时，按对应示例 README 修改面板 profile 或 `LCD_PANEL_TYPE`，不能
  直接套用 10.1 英寸 JD9365 的配置。

## I2C 总线使用方式

板上共享 I2C1 总线的 SDA/SCL 是 GPIO7/GPIO8，默认频率为 400 kHz。不同示例的
初始化方式不同，不能同时运行或重复初始化同一总线：

- `i2c` 和 `i2s` 使用 Arduino `Wire1`。
- `mipi_csi` 先使用 `Wire1`，再取得新版 `i2c_master` 总线句柄，供 ESP_Video
  摄像头 SCCB 和 LCD 背光共用。
- `mipi_dsi` 使用 `driver/i2c.h` 的 legacy API。


复杂示例的构建和硬件状态仍需在目标板上单独验证；本目录 README 中的版本和参数
不代表其他 ESP32-P4 板卡或其他 Arduino-ESP32 版本也兼容。
