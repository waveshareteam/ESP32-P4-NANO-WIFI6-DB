# mipi_csi

## 作用

初始化 ESP32-P4-NANO-WIFI6-DB 的 OV5647 MIPI-CSI 摄像头和 MIPI-DSI LCD，
将摄像头实时画面显示到 LCD。LCD 入口支持 JD9365、HX8394 和 ILI9881C，当前默认使用
10.1 英寸 JD9365 的 RGB565 配置。

## 前置条件

- 使用 Arduino-ESP32 `3.3.11` 和其对应的 ESP32-P4 预编译库。
- `ESP_Video` 必须启用 MIPI-CSI video device：
  `CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE=y`。仓库中的 `ci.yml` 已声明
  该要求；如果使用自定义 Arduino-ESP32 构建，也必须保留这个配置。
- 连接 OV5647 摄像头和实际 LCD 面板，并确认摄像头的供电、MIPI 线缆及共享 I2C/SCCB
  连接正确。

## 使用方法

1. 将 OV5647 摄像头和实际 LCD 面板连接到开发板。
2. 使用 Arduino-ESP32 `3.3.11`，选择 `Waveshare ESP32-P4-NANO-WIFI6-DB`。
3. 在 `lcd_panel.h` 中选择实际面板：
   `LCD_PANEL_TYPE` 可设为 `lcd_panel_type_jd9365`、
   `lcd_panel_type_hx8394` 或 `lcd_panel_type_ili9881c`。
4. 打开 `mipi_csi.ino`，编译并上传。
5. 打开串口监视器，波特率设置为 `115200`。

## LCD 与 I2C

当前本地 Arduino-ESP32 版本为 `3.3.11`，其 ESP32-P4 预编译包基于
ESP-IDF `v5.5.5`。`ESP32_Display_Panel 1.0.4` 使用旧版 I2C 驱动，而
`ESP_Video 3.3.11` 使用新版 `i2c_master` 驱动；两者同时进入同一个程序时，
即使没有调用 LCD 的 I2C 功能，也可能触发：

```text
CONFLICT! driver_ng is not allowed to be used with this old driver
```

因此本示例不使用 `ESP32_Display_Panel`，而是维护本地的 `lcd_panel.cpp/.h`
和三个面板驱动文件，直接调用原生 `esp_lcd` MIPI-DSI API。开发板的 LCD
没有复位 GPIO，背光通过共享 I2C 总线 GPIO7/GPIO8 的设备 `0x45`、寄存器
`0x96` 控制。

摄像头和背光共用由 Arduino `Wire1` 管理的新版 `i2c_master` 总线句柄，不使用
旧版 I2C API。这样不需要修改 Arduino-ESP32 或官方库源码；后续若官方库
提供兼容新旧 I2C 驱动的实现，再评估是否切回通用 LCD 库。

## 摄像头库版本与日志

本示例使用 Arduino-ESP32 `3.3.11` 内置的 `ESP_Video 3.3.11`，底层预编译
组件版本为 `esp_video 2.3.0`、`esp_cam_sensor 2.3.0` 和
`esp_sccb_intf 0.0.8`。这些版本共同完成 MIPI-CSI 初始化、SCCB 通信、
传感器驱动注册和视频设备访问。

当前 `esp_video` 会在初始化时依次尝试已编译进来的多个传感器驱动。因此
可能看到 `imx500`、`os04c10`、`ov2710` 等驱动的探测失败日志；这只是自动
探测过程。最后出现 OV5647 的 `PID=0x5647` 识别日志并输出
`OV5647 direct display started`，才表示本示例使用的摄像头已经启动。

## 板级参数

- I2C/SCCB：SDA=`GPIO7`，SCL=`GPIO8`，I2C 控制器 `1`。
- LCD：JD9365 为 800×1280、2 lanes、1500 Mbps/lane；HX8394 为 720×1280、
  2 lanes、700 Mbps/lane；ILI9881C 为 720×1280、2 lanes、1000 Mbps/lane。
- LCD：三种面板均使用 RGB565、PHY LDO3/2500 mV；当前帧缓冲数量为 3。
- LCD：无独立复位 GPIO；背光为 I2C `0x45`/寄存器 `0x96`。
- 摄像头：OV5647 MIPI-CSI；XCLK 和复位脚均为 NC。当前 Arduino 路径不产生或控制
  外部 XCLK/复位信号，传感器探测和 CSI 视频设备初始化由 `ESP_Video` 完成。

## 运行日志

初始化时可能看到其他内置传感器驱动探测失败的日志，这些日志来自自动探测过程，
不一定表示 OV5647 失败。成功启动的关键日志是：

```text
OV5647 direct display started
```

如果没有该日志，按顺序检查 I2C/SCCB、LCD 初始化、MIPI-CSI 配置和摄像头捕获设备。
本示例不使用 `ESP32_Display_Panel`；它通过 `Wire1` 建立新版 `i2c_master` 总线，
再把总线句柄共享给摄像头和 LCD 背光。
