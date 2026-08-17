# Wi-Fi Station / SoftAP

[English Version](./README.md)

默认将 ESP32-P4 连接到现有 Wi-Fi 接入点。也可以在 menuconfig 中打开 SoftAP
模式，使用手机搜索并连接设备创建的热点，进行 Wi-Fi 信号质量测试。

## 难度

中级。

## 硬件要求

- 带 Wi-Fi 支持的开发板或配置。
- 2.4 GHz Wi-Fi 接入点，或支持 Wi-Fi 的手机。

某些 ESP32-P4 开发板需要外部或伴随 Wi-Fi 路径。使用此示例前请查看你的开发板文档。

## 构建和烧录

请先使用 ESP-IDF 终端。如果你的编辑器插件无法构建该示例，先用命令行流程验证，再调试编辑器配置。

```bash
cd examples/esp_idf/03_wifistation
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

在 `menuconfig` 中打开 **Example Connection Configuration** 或 **Example Configuration** 并设置：

- Wi-Fi SSID 和密码。
- Station 模式下的最大重试次数和认证模式。
- SoftAP 模式下的信道和最大连接数。

打开 `Run in SoftAP mode` 后，源码会打印 SSID、密码、信道、最大连接数和 AP IP
地址；默认关闭时仍保留 Station 连接和网关 Ping 功能。不要在共享串口日志中使用生产凭据。

## 预期输出

SoftAP 模式下，串口日志应显示热点配置和 AP IP 地址。手机连接后，串口会打印
手机的 MAC 地址和连接事件。

```text
SoftAP ready
SSID: myssid
Password: mypassword
Channel: 1, max connections: 4
AP IP address: 192.168.4.1
Station connected: XX:XX:XX:XX:XX:XX, aid=1
```

## 排障

- 确认开发板具备 Wi-Fi 支持。
- 确认手机已打开 Wi-Fi，并搜索配置的 SSID。
- 确认手机使用 2.4 GHz Wi-Fi。
- 如果手机无法连接，检查密码长度是否至少为 8 个字符。
- 如果在编辑器扩展中构建失败，请改用 ESP-IDF 命令行 shell 重试，确保托管组件和目标相关工具已显式初始化。
