# squareline_wifi_clock

这是基于 SquareLine UI 的 Wi-Fi 时钟示例，适配 Waveshare
`ESP32-P4-NANO-WIFI6-DB` 的 MIPI-DSI LCD、GT911 触摸和 I2C 背光控制器。

当前支持三种项目本地 LCD 驱动：JD9365、HX8394 和 ILI9881C。默认配置为
8 英寸 JD9365（800×1280、RGB565、双 DSI lane、1500 Mbps/lane）。

## 使用方法

1. 使用 Arduino-ESP32 `3.3.11`，开发板选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
2. 安装本 README 中列出的库，并保留本示例目录下的 `libraries/ui` 本地库。
3. 打开本示例根目录的 `esp_panel_drivers_conf.h`，只将一个 LCD 驱动选项设为 `1`：

   - `ESP_PANEL_DRIVERS_LCD_ENABLE_JD9365_LOCAL`
   - `ESP_PANEL_DRIVERS_LCD_ENABLE_HX8394_LOCAL`
   - `ESP_PANEL_DRIVERS_LCD_ENABLE_ILI9881C_LOCAL`

4. 选择 JD9365 时，在同一个配置文件中只将一个屏幕配置设为 `1`：

   - `CONFIG_BSP_LCD_TYPE_800_1280_8_INCH_A`
   - `CONFIG_BSP_LCD_TYPE_720_1280_10_1_INCH_B`
   - `CONFIG_BSP_LCD_TYPE_720_1280_9_INCH_B`
   - `CONFIG_BSP_LCD_TYPE_800_800_3_4_INCH`
   - `CONFIG_BSP_LCD_TYPE_720_720_4_INCH`

   屏幕尺寸、DSI 时序和 JD9365 初始化命令必须属于同一个面板配置。
   当前配置中的 `CONFIG_BSP_LCD_TYPE_720_1280_10_1_INCH_B` 是独立的
   720×1280、1200 Mbps/lane B 型面板 profile；它不是 Nano BSP 板载 10.1 英寸
   JD9365 的 `800×1280`、1500 Mbps/lane A 型 profile，不能直接作为板载 10.1 英寸
   面板的替代配置。默认 8 英寸 JD9365 profile 与 Nano BSP 的 8 英寸面板参数一致。
5. 在 `squareline_wifi_clock.ino` 中填写 `TIMEZONE_OFFSET`。需要天气信息时，
   同时填写 `WEATHER_CITY`、`WEATHER_API_KEY`，并将 `weather_enabled` 改为 `1`；
   默认值为 `0`，只显示网络同步时间，不发送 OpenWeather 请求。
6. 编译并上传，打开 `115200` 波特率串口监视器。

首次启动后的操作流程：打开 Wi-Fi 设置界面，扫描网络，选择 SSID，在触摸键盘中输入
密码并确认。连接成功后会通过 NTP 同步时间；如果启用了天气功能，还会请求
OpenWeather 数据。

需要先验证 LCD 时序时，将 `squareline_wifi_clock.ino` 中的
`LCD_COLOR_BAR_TEST` 改为 `1`。测试图案会在 LVGL 启动前显示，并保持在屏幕上。

## 为什么使用 legacy I2C API

ESP32-P4-NANO-WIFI6-DB 的 I2C1（SDA GPIO7、SCL GPIO8）同时连接 GT911 和 LCD 背光
控制器。示例使用 `driver/i2c.h` 的 legacy API 初始化 I2C1，并直接向背光器件
`0x45` 的 `0x95`、`0x96` 寄存器写入配置和亮度值。

触摸仍使用 `ESP32_Display_Panel` 的 `BusI2C`/`TouchGT911` 接口，但通过
`configI2C_HostSkipInit()` 复用已经启动的 I2C1。这样整条共享总线只有一个初始化
和所有权来源，不会把背光的 legacy API 与另一套 `i2c_master` 初始化混用。

## Wi-Fi 凭据和天气配置

- 成功连接的 SSID 和密码会保存到 NVS，下一次启动时会尝试自动连接。
- `WEATHER_API_KEY` 当前为空占位符，不要把真实 API key 提交到仓库。
- 当前调试代码会把 Wi-Fi SSID 和密码打印到串口；分享日志前必须删除这些内容，
  面向公开发布时建议关闭密码打印。
- 天气接口地址当前使用 HTTP；如果部署环境要求 HTTPS，需要单独调整网络实现，不能
  仅通过 README 中填写 API key 解决。

## 当前适配版本与依赖

| 类型 | 库或组件 | 版本 | 用途 |
| --- | --- | --- | --- |
| Arduino 核心 | Arduino-ESP32 | `3.3.11` | Arduino 框架和 ESP32-P4 板级支持 |
| ESP-IDF | Arduino-ESP32 内置 ESP-IDF | `5.5.5` | MIPI-DSI、legacy I2C、FreeRTOS 和 LCD 底层 API |
| 直接使用 | ESP32_Display_Panel | `1.0.4` | `BusDSI`、LCD、`BusI2C`、GT911 和 LVGL 显示接口 |
| 直接使用 | lvgl | `8.4.0` | LVGL 运行时 |
| 直接使用 | NTPClient | `3.2.1` | 网络时间同步 |
| 直接使用 | ArduinoJson | `6.21.3` | 天气 JSON 解析 |
| 传递依赖 | ESP32_IO_Expander | `1.1.1` | `ESP32_Display_Panel` 的依赖 |
| 传递依赖 | esp-lib-utils | `0.2.3` | Espressif 公共工具依赖 |
| 本地库 | SquareLine `ui` | `1.0` | 本示例生成的 UI、图片和字体 |

HX8394、ILI9881C 和 JD9365 的 MIPI-DSI 控制器实现均位于本示例的
`src/drivers/lcd`，不需要通过 `mipi_csi` 或其他示例目录引用源文件。

## 板级约束

- LCD 没有独立复位 GPIO；DSI 使用两条 data lane，PHY LDO 使用通道 3。
- GT911 的复位和中断脚未连接，示例使用无 GPIO 复位/中断配置。
- 背光由 I2C1 的 `0x45` 设备控制，启动时先执行芯片配置和延时，再打开背光。
- 工程不再使用 `lcd_panel.h` 或独立 adapter 选择屏幕；LCD 驱动和屏幕配置统一
  由 `esp_panel_drivers_conf.h` 选择。
- 触摸复位和中断 GPIO 都是 NC；触摸控制器仍挂在共享 I2C1 上，硬件必须与本板版本
  一致。

SquareLine 的 UI 原始设计尺寸由生成文件决定。更换到不同分辨率的面板时，如需
控件铺满屏幕，请在 SquareLine 工程中同步调整画布尺寸。
