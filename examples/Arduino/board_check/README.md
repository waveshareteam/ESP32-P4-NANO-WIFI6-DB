# board_check

## 作用

读取并打印开发板的芯片型号、芯片版本、CPU 信息、Flash 容量、PSRAM 容量和剩余
堆内存。该示例不访问 LCD、摄像头、音频、SDMMC 或 ESP-Hosted，无需连接其他外设。

## 使用方法

1. 在 Arduino IDE 中选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
2. 打开 `board_check.ino`，编译并上传。
3. 打开串口监视器，波特率设置为 `115200`。
4. 查看开发板信息输出。

## 预期输出

启动后会输出一次类似下面的信息；具体 CPU 频率和内存数值以实际板卡和 Arduino
板卡菜单配置为准：

```text
Waveshare ESP32-P4-NANO-WIFI6-DB
Chip model: ESP32-P4
Chip revision: ...
CPU cores: ...
CPU frequency: ... MHz
Flash size: 16 MB
PSRAM size: ... MB
Free heap: ... bytes
```
