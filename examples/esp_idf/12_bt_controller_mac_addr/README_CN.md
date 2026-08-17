# BT Controller MAC Address

[English Version](./README.md)

通过 ESP-Hosted 验证 ESP32-P4 的蓝牙控制器访问能力。

该示例会连接 ESP32-P4 主机和无线协处理器，打印协处理器固件和芯片信息，读取 BT Controller MAC 地址，在控制器初始化前可选更新该 MAC 地址，并启动名为 `P4_GATTS_DEMO` 的可连接 BLE GATT Server。

## 难度

中级。

## 硬件要求

- 已配置 ESP-Hosted 无线协处理器访问路径的 ESP32-P4 开发板。
- 支持 Bluetooth Controller 的兼容 ESP32 系列协处理器固件。
- 用于验证的 BLE Scanner 应用，例如 nRF Connect。

ESP32-P4 本身没有原生蓝牙射频。该示例中的 Bluetooth Controller 运行在协处理器上，ESP32-P4 通过 Hosted VHCI 运行 Bluedroid Host。

## 构建和烧录

请先使用 ESP-IDF 终端。如果编辑器插件无法构建该示例，先用命令行流程验证，再调试编辑器配置。

```bash
cd examples/esp_idf/12_bt_controller_mac_addr
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

在 `menuconfig` 中打开 **Example Configuration**：

- 保持 **Update BT Controller MAC address before init** 关闭时，只读取当前 BT MAC 地址。
- 打开该选项后，会把 **BT Controller MAC address** 中的地址临时写入控制器。地址必须使用 `XX:XX:XX:XX:XX:XX` 格式。

MAC 更新必须发生在 `esp_hosted_bt_controller_init()` 之前。该修改是临时的，复位后会恢复。

## 预期输出

串口日志应显示：

- ESP-Hosted 协处理器连接成功。
- 协处理器固件和芯片信息。
- 当前 BT Controller MAC 地址。
- Hosted BT Controller 初始化成功。
- `BLE advertising started as P4_GATTS_DEMO`。
- `GATTS app registered`。
- `GATT service 0x00FF started, char 0xFF01`。

广播启动后，BLE Scanner 应能扫描到 `P4_GATTS_DEMO`。

## BLE 交互

使用 nRF Connect 或其他 BLE Scanner：

1. 扫描并连接 `P4_GATTS_DEMO`。
2. 发现服务 `0x00FF`。
3. 读取 characteristic `0xFF01`，默认值为 `hello from esp32-p4`。
4. 向 characteristic `0xFF01` 写入一段短数据。
5. 对同一个 characteristic 打开 Notify 后再次写入，设备会把写入内容作为 notification 回显。
6. 断开连接后，设备会自动重新开始广播。

在 Windows 上，请使用 **Bluetooth LE Explorer** 或其他 BLE GATT Client，不要用系统“蓝牙和设备”页面作为主要测试工具。Bluetooth LE Explorer 可能会在 GATT 发现前先发起配对流程；该示例会接受 Just Works bonding。如果之前配对失败过，请先在 Windows 蓝牙设置里删除缓存的 `P4_GATTS_DEMO` 设备，再重新扫描连接。

## 排障

- 确认协处理器固件已启用 ESP-Hosted Bluetooth 支持。
- 确认 ESP-Hosted 传输引脚和复位 GPIO 与开发板匹配。
- 如果读取 BT MAC 失败，检查 ESP-Hosted 启动日志中协处理器是否报告蓝牙能力。
- 如果广播无法启动，检查 `sdkconfig` 中是否启用了 `CONFIG_BT_ENABLED`、`CONFIG_BT_CONTROLLER_DISABLED` 和 Hosted Bluedroid VHCI。
- 如果能看到设备但无法连接，确认 BLE Scanner 连接的是新的 `P4_GATTS_DEMO` 可连接广播，而不是缓存的旧扫描结果。
- 如果 Windows 提示配对失败，先在 Windows 蓝牙设置中删除该设备，重启示例后再从 Bluetooth LE Explorer 重新连接。
