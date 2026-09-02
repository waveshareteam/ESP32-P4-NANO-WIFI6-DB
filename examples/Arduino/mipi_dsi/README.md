# mipi_dsi

本示例使用项目本地 LCD 驱动初始化 MIPI-DSI 面板，循环显示水平/垂直色条，并通过
I2C 背光控制器验证 LCD 背光。它不使用摄像头、LVGL 或 GT911 触摸。

## 使用方法

1. 使用 Arduino-ESP32 `3.3.11`，选择开发板 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
2. 安装或确认以下 Arduino 库及版本：

   - `ESP32_Display_Panel` `1.0.4`
   - `ESP32_IO_Expander` `1.1.1`
   - `esp-lib-utils` `0.2.3`

   后两个库是 `ESP32_Display_Panel` 的依赖；Arduino-ESP32 `3.3.11` 已包含本示例
   使用的 ESP-IDF `5.5.5` 底层头文件和库。

3. 打开
   `esp_panel_drivers_conf.h`，只选择一个本地 LCD 驱动：

   - `ESP_PANEL_DRIVERS_LCD_ENABLE_JD9365_LOCAL`
   - `ESP_PANEL_DRIVERS_LCD_ENABLE_HX8394_LOCAL`
   - `ESP_PANEL_DRIVERS_LCD_ENABLE_ILI9881C_LOCAL`

4. 如果选择 JD9365，在同一个配置文件中选择与实际面板匹配的屏幕 profile。例如当前默认
   10.1 英寸面板：

   ```cpp
   #define CONFIG_BSP_LCD_TYPE_800_1280_10_1_INCH_A (1)
   #define CONFIG_BSP_LCD_TYPE_800_1280_8_INCH_A    (0)
   #define CONFIG_BSP_LCD_TYPE_720_1280_10_1_INCH_B (0)
   ```

   `CONFIG_BSP_LCD_TYPE_800_1280_8_INCH_A` 与当前 10.1 英寸 A 型 profile 使用相同的
   800×1280、1500 Mbps/lane DSI 参数；`CONFIG_BSP_LCD_TYPE_720_1280_10_1_INCH_B`
   是独立的 720×1280、1200 Mbps/lane 旧 B 型 profile，不能与 Nano BSP 板载 10.1
   英寸面板混用。实际面板必须与寄存器表和 DSI 参数匹配。
5. 编译并上传，打开 `115200` 波特率串口监视器。本示例启动后循环显示 MIPI-DSI
   色条，用于验证 LCD、DSI 时序和背光。

当前默认配置为 10.1 英寸 JD9365：800×1280、RGB565、双通道、1500 Mbps/lane。

如果选择 HX8394 或 ILI9881C，示例会使用对应驱动中定义的固定分辨率和 DSI 时序；
JD9365 的屏幕尺寸宏只在选择 JD9365 时生效。

## 为什么使用 legacy I2C API

板上的背光控制器地址为 `0x45`，连接在 I2C1 的 GPIO7/GPIO8 总线上。示例使用
`driver/i2c.h` 提供的 legacy I2C API，直接调用 `i2c_param_config()`、
`i2c_driver_install()` 和 `i2c_master_write_to_device()` 控制背光。

背光初始化会使用寄存器 `0x95` 设置工作模式，并使用寄存器 `0x96` 设置亮度。

这样做是为了与现有 Waveshare 示例和同一 I2C1 总线上的旧驱动保持一致，避免同时使用
Arduino `Wire1` 或另一套 I2C 总线管理方式造成重复初始化或总线所有权冲突。这不是因为
`ESP32_Display_Panel` 版本过旧；如果改用新的 I2C API，需要同时重新设计 I2C1 的总线
初始化和设备共享方式。

## 当前适配版本与依赖

| 类型 | 库或组件 | 当前版本 | 用途 |
| --- | --- | --- | --- |
| Arduino 核心 | Arduino-ESP32 | `3.3.11` | Arduino 框架、ESP32-P4 板级支持 |
| ESP-IDF | Arduino-ESP32 内置 ESP-IDF | `5.5.5` | MIPI-DSI、I2C、FreeRTOS 和 LCD 底层 API |
| 直接使用 | ESP32_Display_Panel | `1.0.4` | `BusDSI`、`LCD` 接口和 DSI 面板生命周期 |
| 库依赖 | ESP32_IO_Expander | `1.1.1` | `ESP32_Display_Panel` 的依赖，本示例不创建 IO Expander 实例 |
| 库依赖 | esp-lib-utils | `0.2.3` | `ESP32_Display_Panel` 和 `ESP32_IO_Expander` 的公共工具依赖 |

HX8394、ILI9881C 和 JD9365 的控制器实现是本示例 `src/drivers/lcd` 下的项目本地驱动，
不对应额外安装的 LCD 驱动库版本。

本示例不使用 LVGL、ESP_Video、GT911 触摸驱动，也不依赖 `mipi_csi` 目录中的源文件。

## 故障定位

- 看到 `LCD: start complete` 后仍无图像时，先核对 LCD 控制器、分辨率、DSI lane
  速率和实际面板是否匹配。
- 背光初始化失败时，确认 I2C1 上的 `0x45` 设备可响应，并确认 PHY LDO 通道 3
  的供电条件满足面板要求。
- 不要同时运行 `i2c`、`mipi_csi` 或 SquareLine 示例；它们对共享 I2C1 的所有权和
  初始化方式不同。
